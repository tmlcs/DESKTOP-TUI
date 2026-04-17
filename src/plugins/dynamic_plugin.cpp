#include "plugins/plugin_interface.hpp"

#include <dlfcn.h>
#include <cstring>
#include <stdexcept>

namespace desktop_tui {

// ============================================================================
// DynamicPlugin Implementation
// ============================================================================

DynamicPlugin::DynamicPlugin(const std::string& path) 
    : path_(path), handle_(nullptr), instance_(nullptr), state_(PluginState::Unloaded) {
}

DynamicPlugin::~DynamicPlugin() {
    unload();
}

DynamicPlugin::DynamicPlugin(DynamicPlugin&& other) noexcept
    : path_(std::move(other.path_))
    , handle_(other.handle_)
    , instance_(other.instance_)
    , state_(other.state_)
    , create_func_(other.create_func_)
    , destroy_func_(other.destroy_func_) {
    
    other.handle_ = nullptr;
    other.instance_ = nullptr;
    other.state_ = PluginState::Unloaded;
    other.create_func_ = nullptr;
    other.destroy_func_ = nullptr;
}

DynamicPlugin& DynamicPlugin::operator=(DynamicPlugin&& other) noexcept {
    if (this != &other) {
        unload();
        
        path_ = std::move(other.path_);
        handle_ = other.handle_;
        instance_ = other.instance_;
        state_ = other.state_;
        create_func_ = other.create_func_;
        destroy_func_ = other.destroy_func_;
        
        other.handle_ = nullptr;
        other.instance_ = nullptr;
        other.state_ = PluginState::Unloaded;
        other.create_func_ = nullptr;
        other.destroy_func_ = nullptr;
    }
    return *this;
}

PluginResult DynamicPlugin::load() {
    if (state_ >= PluginState::Loaded) {
        return PluginResult::ok("Plugin already loaded");
    }
    
    state_ = PluginState::Loading;
    
    // Cargar la biblioteca compartida
    // RTLD_NOW: resolver todos los símbolos inmediatamente
    // RTLD_LOCAL: los símbolos no están disponibles para otras bibliotecas cargadas dinámicamente
    handle_ = dlopen(path_.c_str(), RTLD_NOW | RTLD_LOCAL);
    
    if (!handle_) {
        std::string error = dlerror();
        state_ = PluginState::Error;
        return PluginResult::fail("Failed to load plugin: " + error);
    }
    
    // Limpiar errores previos
    dlerror();
    
    // Obtener función factory create_plugin
    create_func_ = reinterpret_cast<CreateFunc>(dlsym(handle_, "create_plugin"));
    const char* error = dlerror();
    if (error != nullptr) {
        dlclose(handle_);
        handle_ = nullptr;
        state_ = PluginState::Error;
        return PluginResult::fail("Missing create_plugin symbol: " + std::string(error));
    }
    
    // Obtener función destroy_plugin
    destroy_func_ = reinterpret_cast<DestroyFunc>(dlsym(handle_, "destroy_plugin"));
    error = dlerror();
    if (error != nullptr) {
        dlclose(handle_);
        handle_ = nullptr;
        state_ = PluginState::Error;
        return PluginResult::fail("Missing destroy_plugin symbol: " + std::string(error));
    }
    
    // Crear instancia del plugin
    auto result = create_instance();
    if (!result.success) {
        dlclose(handle_);
        handle_ = nullptr;
        state_ = PluginState::Error;
        return result;
    }
    
    state_ = PluginState::Loaded;
    return PluginResult::ok("Plugin loaded successfully from " + path_);
}

PluginResult DynamicPlugin::unload() {
    if (state_ == PluginState::Unloaded || state_ == PluginState::Error) {
        return PluginResult::ok("Plugin not loaded");
    }
    
    // Si está inicializado, hacer shutdown primero
    if (state_ >= PluginState::Initialized && instance_) {
        auto result = instance_->shutdown();
        if (!result.success) {
            // Log warning but continue unloading
        }
    }
    
    destroy_instance();
    
    if (handle_) {
        int ret = dlclose(handle_);
        handle_ = nullptr;
        if (ret != 0) {
            std::string error = dlerror();
            return PluginResult::fail("Failed to unload plugin: " + error);
        }
    }
    
    create_func_ = nullptr;
    destroy_func_ = nullptr;
    state_ = PluginState::Unloaded;
    
    return PluginResult::ok("Plugin unloaded successfully");
}

PluginInfo DynamicPlugin::get_info() const {
    if (instance_) {
        return instance_->get_info();
    }
    
    PluginInfo info;
    info.name = "Unknown";
    info.version = "0.0.0";
    info.description = "Plugin not loaded";
    info.author = "Unknown";
    return info;
}

PluginResult DynamicPlugin::create_instance() {
    if (!create_func_) {
        return PluginResult::fail("Create function not available");
    }
    
    try {
        instance_ = create_func_();
        if (!instance_) {
            return PluginResult::fail("create_plugin returned null");
        }
        return PluginResult::ok("Plugin instance created");
    } catch (const std::exception& e) {
        return PluginResult::fail(std::string("Exception creating plugin: ") + e.what());
    } catch (...) {
        return PluginResult::fail("Unknown exception creating plugin");
    }
}

void DynamicPlugin::destroy_instance() {
    if (instance_ && destroy_func_) {
        try {
            destroy_func_(instance_);
        } catch (...) {
            // Ignore exceptions during destruction
        }
        instance_ = nullptr;
    }
}

} // namespace desktop_tui
