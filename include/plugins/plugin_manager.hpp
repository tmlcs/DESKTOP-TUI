#pragma once

#include "plugin_interface.hpp"
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include <mutex>
#include <optional>

namespace desktop_tui {

/**
 * @brief Implementación concreta del contexto para plugins
 */
class PluginContextImpl : public PluginContext {
public:
    PluginContextImpl(class PluginManager& manager);
    
    void register_callback(const std::string& name, 
                          std::function<void()> callback) override;
    
    std::any get_config(const std::string& key) const override;
    
    void log_info(const std::string& message) override;
    void log_warning(const std::string& message) override;
    void log_error(const std::string& message) override;
    
    void register_widget(const std::string& name, void* widget_factory) override;
    
    std::string get_data_path() const override;
    std::string get_config_path() const override;
    
private:
    class PluginManager& manager_;
};

/**
 * @brief Gestor de plugins dinámicos
 * 
 * Responsable de:
 * - Cargar/descargar plugins desde archivos .so/.dll/.dylib
 * - Mantener el ciclo de vida de los plugins
 * - Proporcionar un contexto seguro para los plugins
 * - Validar dependencias entre plugins
 */
class PluginManager {
public:
    PluginManager();
    ~PluginManager();
    
    // No copiable
    PluginManager(const PluginManager&) = delete;
    PluginManager& operator=(const PluginManager&) = delete;
    
    /**
     * @brief Inicializa el gestor de plugins
     * @param plugins_dir Directorio donde buscar plugins
     * @return Resultado de la inicialización
     */
    PluginResult initialize(const std::string& plugins_dir = "./plugins");
    
    /**
     * @brief Finaliza el gestor y descarga todos los plugins
     */
    PluginResult shutdown();
    
    /**
     * @brief Carga un plugin desde una ruta específica
     * @param path Ruta al archivo del plugin
     * @return Resultado de la carga
     */
    PluginResult load_plugin(const std::string& path);
    
    /**
     * @brief Descarga un plugin por su nombre
     * @param name Nombre del plugin
     * @return Resultado de la descarga
     */
    PluginResult unload_plugin(const std::string& name);
    
    /**
     * @brief Carga todos los plugins encontrados en el directorio configurado
     * @return Lista de resultados por cada plugin intentado
     */
    std::vector<PluginResult> load_all_plugins();
    
    /**
     * @brief Obtiene un plugin por nombre
     * @param name Nombre del plugin
     * @return Puntero al plugin o nullptr si no existe
     */
    IPlugin* get_plugin(const std::string& name) const;
    
    /**
     * @brief Lista todos los plugins cargados
     * @return Vector de nombres de plugins
     */
    std::vector<std::string> list_plugins() const;
    
    /**
     * @brief Obtiene información detallada de un plugin
     * @param name Nombre del plugin
     * @return Información del plugin o nullo si no existe
     */
    std::optional<PluginInfo> get_plugin_info(const std::string& name) const;
    
    /**
     * @brief Ejecuta todos los plugins activos
     * @return Resultados de ejecución por plugin
     */
    std::unordered_map<std::string, PluginResult> execute_all();
    
    /**
     * @brief Registra un callback global accesible por plugins
     * @param name Nombre del callback
     * @param callback Función a registrar
     */
    void register_global_callback(const std::string& name, 
                                 std::function<void()> callback);
    
    /**
     * @brief Ejecuta un callback global por nombre
     * @param name Nombre del callback
     * @return true si el callback existe y fue ejecutado
     */
    bool trigger_callback(const std::string& name);
    
    /**
     * @brief Establece un valor de configuración
     * @param key Clave de configuración
     * @param value Valor a almacenar
     */
    template<typename T>
    void set_config(const std::string& key, const T& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        config_[key] = value;
    }
    
    /**
     * @brief Obtiene un valor de configuración
     * @param key Clave de configuración
     * @return Valor de configuración o nullo si no existe
     */
    std::optional<std::any> get_config(const std::string& key) const;
    
    /**
     * @brief Obtiene el directorio de plugins
     */
    const std::string& get_plugins_directory() const { return plugins_dir_; }
    
    /**
     * @brief Verifica si el gestor está inicializado
     */
    bool is_initialized() const { return initialized_; }
    
    /**
     * @brief Obtiene el número de plugins cargados
     */
    size_t get_plugin_count() const;
    
    /**
     * @brief Obtiene el número de plugins activos (inicializados y corriendo)
     */
    size_t get_active_plugin_count() const;

private:
    std::string plugins_dir_;
    std::unordered_map<std::string, std::unique_ptr<DynamicPlugin>> plugins_;
    std::unordered_map<std::string, std::function<void()>> callbacks_;
    std::unordered_map<std::string, std::any> config_;
    std::unordered_map<std::string, void*> widgets_;
    
    mutable std::mutex mutex_;
    bool initialized_ = false;
    
    std::unique_ptr<PluginContextImpl> context_;
    
    PluginResult validate_dependencies(const PluginInfo& info) const;
    std::string extract_plugin_name(const std::string& path) const;
};

} // namespace desktop_tui
