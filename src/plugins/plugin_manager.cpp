#include "plugin_manager.hpp"
#include <filesystem>
#include <algorithm>
#include <iostream>
#include <optional>

namespace fs = std::filesystem;

namespace desktop_tui {

// ============================================================================
// PluginContextImpl Implementation
// ============================================================================

PluginContextImpl::PluginContextImpl(PluginManager& manager) 
    : manager_(manager) {
}

void PluginContextImpl::register_callback(const std::string& name, 
                                         std::function<void()> callback) {
    manager_.register_global_callback(name, std::move(callback));
}

std::any PluginContextImpl::get_config(const std::string& key) const {
    auto result = manager_.get_config(key);
    if (result.has_value()) {
        return result.value();
    }
    return std::any();
}

void PluginContextImpl::log_info(const std::string& message) {
    // std::cout << "[PLUGIN INFO] " << message << std::endl;
}

void PluginContextImpl::log_warning(const std::string& message) {
    // std::cerr << "[PLUGIN WARNING] " << message << std::endl;
}

void PluginContextImpl::log_error(const std::string& message) {
    std::cerr << "[PLUGIN ERROR] " << message << std::endl;
}

void PluginContextImpl::register_widget(const std::string& name, void* widget_factory) {
    // TODO: Implementar registro de widgets en el sistema UI
    (void)name;
    (void)widget_factory;
}

std::string PluginContextImpl::get_data_path() const {
    // Retornar ruta de datos del plugin
    auto home = std::getenv("HOME");
    if (!home) {
        return "./data";
    }
    return std::string(home) + "/.local/share/desktop-tui/plugins";
}

std::string PluginContextImpl::get_config_path() const {
    // Retornar ruta de configuración del plugin
    auto home = std::getenv("HOME");
    if (!home) {
        return "./config";
    }
    return std::string(home) + "/.config/desktop-tui/plugins";
}

// ============================================================================
// PluginManager Implementation
// ============================================================================

PluginManager::PluginManager() 
    : plugins_dir_("./plugins")
    , initialized_(false) {
}

PluginManager::~PluginManager() {
    shutdown();
}

PluginResult PluginManager::initialize(const std::string& plugins_dir) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (initialized_) {
        return PluginResult::ok("PluginManager already initialized");
    }
    
    plugins_dir_ = plugins_dir;
    
    // Crear directorio de plugins si no existe
    try {
        if (!fs::exists(plugins_dir_)) {
            fs::create_directories(plugins_dir_);
        }
    } catch (const fs::filesystem_error& e) {
        return PluginResult::fail("Failed to create plugins directory: " + std::string(e.what()));
    }
    
    // Crear contexto para plugins
    context_ = std::make_unique<PluginContextImpl>(*this);
    
    initialized_ = true;
    return PluginResult::ok("PluginManager initialized with directory: " + plugins_dir);
}

PluginResult PluginManager::shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!initialized_) {
        return PluginResult::ok("PluginManager not initialized");
    }
    
    PluginResult overall_result = PluginResult::ok();
    
    // Hacer shutdown de todos los plugins en orden inverso
    std::vector<std::string> plugin_names;
    for (const auto& [name, _] : plugins_) {
        plugin_names.push_back(name);
    }
    
    std::reverse(plugin_names.begin(), plugin_names.end());
    
    for (const auto& name : plugin_names) {
        auto result = unload_plugin(name);
        if (!result.success) {
            overall_result = PluginResult::fail(
                "Failed to unload plugin " + name + ": " + result.message
            );
        }
    }
    
    plugins_.clear();
    callbacks_.clear();
    config_.clear();
    widgets_.clear();
    context_.reset();
    
    initialized_ = false;
    
    return overall_result;
}

PluginResult PluginManager::load_plugin(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!initialized_) {
        return PluginResult::fail("PluginManager not initialized");
    }
    
    // Verificar que el archivo existe
    if (!fs::exists(path)) {
        return PluginResult::fail("Plugin file does not exist: " + path);
    }
    
    // Extraer nombre del plugin
    std::string name = extract_plugin_name(path);
    
    // Verificar si ya está cargado
    if (plugins_.find(name) != plugins_.end()) {
        return PluginResult::ok("Plugin already loaded: " + name);
    }
    
    // Crear wrapper dinámico
    auto plugin = std::make_unique<DynamicPlugin>(path);
    
    // Cargar el plugin
    auto load_result = plugin->load();
    if (!load_result.success) {
        return load_result;
    }
    
    // Validar dependencias
    auto info = plugin->get_info();
    auto dep_result = validate_dependencies(info);
    if (!dep_result.success) {
        plugin->unload();
        return dep_result;
    }
    
    // Inicializar el plugin con el contexto
    auto init_result = plugin->get_instance()->initialize(*context_);
    if (!init_result.success) {
        plugin->unload();
        return PluginResult::fail("Failed to initialize plugin: " + init_result.message);
    }
    
    // Registrar el plugin
    plugins_[name] = std::move(plugin);
    
    return PluginResult::ok("Plugin loaded and initialized: " + name + " (" + info.to_string() + ")");
}

PluginResult PluginManager::unload_plugin(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = plugins_.find(name);
    if (it == plugins_.end()) {
        return PluginResult::fail("Plugin not found: " + name);
    }
    
    // Hacer unload del plugin
    auto result = it->second->unload();
    
    // Eliminar del mapa
    plugins_.erase(it);
    
    return result;
}

std::vector<PluginResult> PluginManager::load_all_plugins() {
    std::vector<PluginResult> results;
    
    if (!initialized_) {
        results.push_back(PluginResult::fail("PluginManager not initialized"));
        return results;
    }
    
    // Buscar archivos de plugins en el directorio
    try {
        for (const auto& entry : fs::directory_iterator(plugins_dir_)) {
            if (entry.is_regular_file()) {
                std::string ext = entry.path().extension().string();
                
                // Verificar extensiones válidas según plataforma
#ifdef _WIN32
                if (ext == ".dll") {
#elif defined(__APPLE__)
                if (ext == ".dylib") {
#else
                if (ext == ".so") {
#endif
                    auto result = load_plugin(entry.path().string());
                    results.push_back(result);
                }
            }
        }
    } catch (const fs::filesystem_error& e) {
        results.push_back(PluginResult::fail("Failed to scan plugins directory: " + std::string(e.what())));
    }
    
    return results;
}

IPlugin* PluginManager::get_plugin(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = plugins_.find(name);
    if (it != plugins_.end()) {
        return it->second->get_instance();
    }
    
    return nullptr;
}

std::vector<std::string> PluginManager::list_plugins() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::string> names;
    for (const auto& [name, _] : plugins_) {
        names.push_back(name);
    }
    
    std::sort(names.begin(), names.end());
    return names;
}

std::optional<PluginInfo> PluginManager::get_plugin_info(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = plugins_.find(name);
    if (it != plugins_.end()) {
        return it->second->get_info();
    }
    
    return std::nullopt;
}

std::unordered_map<std::string, PluginResult> PluginManager::execute_all() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::unordered_map<std::string, PluginResult> results;
    
    for (const auto& [name, plugin] : plugins_) {
        if (plugin->is_initialized()) {
            results[name] = plugin->get_instance()->execute();
        }
    }
    
    return results;
}

void PluginManager::register_global_callback(const std::string& name, 
                                            std::function<void()> callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    callbacks_[name] = std::move(callback);
}

bool PluginManager::trigger_callback(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = callbacks_.find(name);
    if (it != callbacks_.end()) {
        it->second();
        return true;
    }
    
    return false;
}

std::optional<std::any> PluginManager::get_config(const std::string& key) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = config_.find(key);
    if (it != config_.end()) {
        return it->second;
    }
    
    return std::nullopt;
}

size_t PluginManager::get_plugin_count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return plugins_.size();
}

size_t PluginManager::get_active_plugin_count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    size_t count = 0;
    for (const auto& [_, plugin] : plugins_) {
        if (plugin->is_initialized()) {
            count++;
        }
    }
    
    return count;
}

PluginResult PluginManager::validate_dependencies(const PluginInfo& info) const {
    for (const auto& dep : info.dependencies) {
        bool found = false;
        
        // Buscar en plugins cargados
        for (const auto& [name, plugin] : plugins_) {
            if (name == dep || plugin->get_info().name == dep) {
                found = true;
                break;
            }
        }
        
        if (!found) {
            return PluginResult::fail("Missing dependency: " + dep);
        }
    }
    
    return PluginResult::ok("All dependencies satisfied");
}

std::string PluginManager::extract_plugin_name(const std::string& path) const {
    fs::path p(path);
    std::string filename = p.stem().string();
    
    // Eliminar prefijo "lib" si existe
    if (filename.substr(0, 3) == "lib") {
        filename = filename.substr(3);
    }
    
    return filename;
}

} // namespace desktop_tui
