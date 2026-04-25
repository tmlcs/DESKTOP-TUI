#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include <any>
#include <cstdint>

namespace desktop_tui {

// Forward declarations
class PluginContext;
class PluginManager;

/**
 * @brief Estados posibles de un plugin
 */
enum class PluginState {
    Unloaded,
    Loading,
    Loaded,
    Initialized,
    Running,
    Stopped,
    Error
};

/**
 * @brief Capabilities que puede solicitar un plugin
 *
 * Los plugins solicitan capabilities durante la inicialización.
 * El PluginManager decide qué capabilities conceder basándose en la configuración.
 */
enum class Capability : uint32_t {
    None = 0,                    // Plugin sin capabilities (sandboxing estricto)
    ReadConfig = 1 << 0,        // Leer configuración del host
    WriteConfig = 1 << 1,       // Modificar configuración del host
    SpawnProcess = 1 << 2,      // Lanzar procesos externos
    AccessClipboard = 1 << 3,   // Leer/escribir clipboard
    AccessNetwork = 1 << 4,     // Acceso a red
    AccessFiles = 1 << 5,       // Acceso al sistema de archivos
    AccessHardware = 1 << 6,    // Acceso a hardware (GPU, audio, etc.)
    All = 0xFFFFFFFF             // Todas las capabilities (desandboxeado)
};

/**
 * @brief Detecta y valida las capabilities solicitadas por un plugin
 *
 * Esta clase verifica que las capabilities solicitadas sean válidas
 * y reporta cualquier capability no reconocida.
 */
class CapabilityDetector {
public:
    /**
     * @brief Detecta capabilities en un string de capabilities
     * @param capabilities String con capabilities separadas por comas o espacios
     * @return bitmask de capabilities detectadas
     */
    static uint32_t detect_capabilities(const std::string& capabilities);

    /**
     * @brief Verifica si un capability es válido
     * @param capability Capability a verificar
     * @return true si es válido, false si no
     */
    static bool is_valid_capability(Capability capability);

    /**
     * @brief Obtiene el nombre de string de un capability
     * @param capability Capability
     * @return Nombre de string del capability
     */
    static std::string capability_name(Capability capability);

    /**
     * @brief Obtiene una descripción de las capabilities detectadas
     * @param capabilities bitmask de capabilities
     * @return Descripción de string
     */
    static std::string describe_capabilities(uint32_t capabilities);
};

/**
 * @brief Información metadata de un plugin
 */
struct PluginInfo {
    std::string name;
    std::string version;
    std::string description;
    std::string author;
    std::vector<std::string> dependencies;
    bool is_core = false;
    
    std::string to_string() const {
        return name + " v" + version + " by " + author;
    }
};

/**
 * @brief Resultado de operaciones de plugins
 */
struct PluginResult {
    bool success;
    std::string message;
    int error_code = 0;
    
    static PluginResult ok(const std::string& msg = "OK") {
        return {true, msg, 0};
    }
    
    static PluginResult fail(const std::string& msg, int code = 1) {
        return {false, msg, code};
    }
};

/**
 * @brief Interfaz base que deben implementar todos los plugins
 * 
 * Los plugins deben exportar una función factory: extern "C" IPlugin* create_plugin()
 * y una función destroy: extern "C" void destroy_plugin(IPlugin* plugin)
 */
class IPlugin {
public:
    virtual ~IPlugin() = default;
    
    /**
     * @brief Obtiene la información del plugin
     */
    virtual PluginInfo get_info() const = 0;
    
    /**
     * @brief Inicializa el plugin con el contexto proporcionado
     * @return Resultado de la inicialización
     */
    virtual PluginResult initialize(PluginContext& context) = 0;
    
    /**
     * @brief Ejecuta el plugin (puede ser llamado repetidamente)
     * @return Resultado de la ejecución
     */
    virtual PluginResult execute() = 0;
    
    /**
     * @brief Detiene el plugin y libera recursos
     * @return Resultado del shutdown
     */
    virtual PluginResult shutdown() = 0;
    
    /**
     * @brief Maneja comandos personalizados del plugin
     * @param command Nombre del comando
     * @param args Argumentos del comando
     * @return Resultado del comando
     */
    virtual PluginResult handle_command(const std::string& command, 
                                       const std::vector<std::string>& /* args */) {
        return PluginResult::fail("Command not implemented: " + command);
    }
};

/**
 * @brief Contexto proporcionado a los plugins para interactuar con el host
 */
class PluginContext {
public:
    virtual ~PluginContext() = default;
    
    // Registrar un callback global
    virtual void register_callback(const std::string& name, 
                                  std::function<void()> callback) = 0;
    
    // Obtener configuración del host
    virtual std::any get_config(const std::string& key) const = 0;
    
    // Loguear mensaje desde el plugin
    virtual void log_info(const std::string& message) = 0;
    virtual void log_warning(const std::string& message) = 0;
    virtual void log_error(const std::string& message) = 0;
    
    // Registrar un widget personalizado
    virtual void register_widget(const std::string& name, void* widget_factory) = 0;
    
    // Obtener ruta de datos del plugin
    virtual std::string get_data_path() const = 0;
    
    // Obtener ruta de configuración del plugin
    virtual std::string get_config_path() const = 0;
};

/**
 * @brief Wrapper para plugins cargados dinámicamente
 */
class DynamicPlugin {
public:
    DynamicPlugin(const std::string& path);
    ~DynamicPlugin();
    
    // No copiable, pero sí movible
    DynamicPlugin(const DynamicPlugin&) = delete;
    DynamicPlugin& operator=(const DynamicPlugin&) = delete;
    DynamicPlugin(DynamicPlugin&& other) noexcept;
    DynamicPlugin& operator=(DynamicPlugin&& other) noexcept;
    
    /**
     * @brief Carga el plugin desde la biblioteca compartida
     */
    PluginResult load();
    
    /**
     * @brief Descarga el plugin
     */
    PluginResult unload();
    
    /**
     * @brief Obtiene la instancia del plugin
     */
    IPlugin* get_instance() const { return instance_; }
    
    /**
     * @brief Obtiene la ruta del archivo del plugin
     */
    const std::string& get_path() const { return path_; }
    
    /**
     * @brief Obtiene el estado actual del plugin
     */
    PluginState get_state() const { return state_; }
    
    /**
     * @brief Obtiene la información del plugin
     */
    PluginInfo get_info() const;
    
    /**
     * @brief Verifica si el plugin está cargado
     */
    bool is_loaded() const { return state_ >= PluginState::Loaded; }
    
    /**
     * @brief Verifica si el plugin está inicializado
     */
    bool is_initialized() const { return state_ >= PluginState::Initialized; }

private:
    std::string path_;
    void* handle_ = nullptr;
    IPlugin* instance_ = nullptr;
    PluginState state_ = PluginState::Unloaded;
    
    // Funciones factory
    using CreateFunc = IPlugin*(*)();
    using DestroyFunc = void(*)(IPlugin*);
    
    CreateFunc create_func_ = nullptr;
    DestroyFunc destroy_func_ = nullptr;
    
    PluginResult create_instance();
    void destroy_instance();
};

} // namespace desktop_tui
