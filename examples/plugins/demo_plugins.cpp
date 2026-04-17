/**
 * @brief Programa de demostración del sistema de plugins
 * 
 * Este ejemplo muestra cómo:
 * 1. Inicializar el PluginManager
 * 2. Cargar plugins dinámicamente
 * 3. Interactuar con los plugins
 * 4. Ejecutar comandos personalizados
 * 5. Descargar plugins correctamente
 */

#include "../src/plugins/plugin_manager.hpp"
#include <iostream>
#include <string>

using namespace desktop_tui;

void print_usage(const char* program) {
    std::cout << "Uso: " << program << " [opciones]\n"
              << "\nOpciones:\n"
              << "  --plugin <ruta>   Cargar un plugin específico\n"
              << "  --list            Listar plugins cargados\n"
              << "  --execute         Ejecutar todos los plugins\n"
              << "  --command <cmd>   Ejecutar comando en el plugin\n"
              << "  --help            Mostrar esta ayuda\n"
              << "\nEjemplos:\n"
              << "  " << program << " --plugin ./examples/plugins/libhello_world.so\n"
              << "  " << program << " --list\n"
              << std::endl;
}

int main(int argc, char* argv[]) {
    std::cout << "=== Desktop TUI Plugin Demo ===\n" << std::endl;
    
    // Crear gestor de plugins
    PluginManager manager;
    
    // Inicializar con directorio por defecto
    auto init_result = manager.initialize("./plugins");
    if (!init_result.success) {
        std::cerr << "Error inicializando PluginManager: " << init_result.message << std::endl;
        return 1;
    }
    
    std::cout << "✓ PluginManager inicializado" << std::endl;
    std::cout << "  Directorio: " << manager.get_plugins_directory() << "\n" << std::endl;
    
    // Configurar algunos valores de ejemplo
    manager.set_config("debug_mode", true);
    manager.set_config("max_executions", 100);
    
    // Procesar argumentos de línea de comandos
    bool list_plugins = false;
    bool execute_plugins = false;
    std::string command_to_run;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--help" || arg == "-h") {
            print_usage(argv[0]);
            return 0;
        }
        
        if (arg == "--plugin" && i + 1 < argc) {
            std::string plugin_path = argv[++i];
            std::cout << "Cargando plugin: " << plugin_path << std::endl;
            
            auto result = manager.load_plugin(plugin_path);
            if (result.success) {
                std::cout << "✓ " << result.message << std::endl;
            } else {
                std::cerr << "✗ Error: " << result.message << std::endl;
            }
        }
        else if (arg == "--list") {
            list_plugins = true;
        }
        else if (arg == "--execute") {
            execute_plugins = true;
        }
        else if (arg == "--command" && i + 1 < argc) {
            command_to_run = argv[++i];
        }
    }
    
    // Si no se especificaron plugins, intentar cargar todos del directorio
    if (manager.get_plugin_count() == 0) {
        std::cout << "No se especificaron plugins, buscando en el directorio..." << std::endl;
        auto results = manager.load_all_plugins();
        
        if (results.empty()) {
            std::cout << "  No se encontraron plugins en el directorio." << std::endl;
        } else {
            for (const auto& result : results) {
                if (result.success) {
                    std::cout << "✓ " << result.message << std::endl;
                } else {
                    std::cout << "⚠ " << result.message << std::endl;
                }
            }
        }
    }
    
    std::cout << "\n=== Estado de Plugins ===" << std::endl;
    std::cout << "Plugins cargados: " << manager.get_plugin_count() << std::endl;
    std::cout << "Plugins activos: " << manager.get_active_plugin_count() << std::endl;
    
    // Listar plugins
    if (list_plugins || !command_to_run.empty()) {
        auto names = manager.list_plugins();
        
        if (names.empty()) {
            std::cout << "\nNo hay plugins cargados." << std::endl;
        } else {
            std::cout << "\nPlugins disponibles:" << std::endl;
            for (const auto& name : names) {
                auto info_opt = manager.get_plugin_info(name);
                if (info_opt.has_value()) {
                    const auto& info = info_opt.value();
                    std::cout << "  - " << info.name << " v" << info.version << std::endl;
                    std::cout << "    " << info.description << std::endl;
                    std::cout << "    Autor: " << info.author << std::endl;
                }
            }
        }
    }
    
    // Ejecutar plugins
    if (execute_plugins) {
        std::cout << "\n=== Ejecutando Plugins ===" << std::endl;
        auto results = manager.execute_all();
        
        for (const auto& [name, result] : results) {
            std::cout << "  [" << name << "] " 
                      << (result.success ? "✓" : "✗") 
                      << " " << result.message << std::endl;
        }
    }
    
    // Ejecutar comando personalizado
    if (!command_to_run.empty()) {
        std::cout << "\n=== Ejecutando Comando ===" << std::endl;
        
        auto names = manager.list_plugins();
        if (names.empty()) {
            std::cout << "No hay plugins para ejecutar el comando." << std::endl;
        } else {
            // Ejecutar comando en el primer plugin encontrado
            const auto& plugin_name = names[0];
            auto* plugin = manager.get_plugin(plugin_name);
            
            if (plugin) {
                std::vector<std::string> args;
                auto result = plugin->handle_command(command_to_run, args);
                
                std::cout << "  Comando '" << command_to_run << "' en plugin '" 
                          << plugin_name << "': " << result.message << std::endl;
            }
        }
    }
    
    // Trigger de callback de ejemplo
    std::cout << "\n=== Probando Callbacks ===" << std::endl;
    if (manager.trigger_callback("hello_action")) {
        std::cout << "  Callback 'hello_action' ejecutado exitosamente" << std::endl;
    } else {
        std::cout << "  Callback 'hello_action' no encontrado (puede que ningún plugin lo registre)" << std::endl;
    }
    
    // Shutdown limpio
    std::cout << "\n=== Shutting Down ===" << std::endl;
    auto shutdown_result = manager.shutdown();
    
    if (shutdown_result.success) {
        std::cout << "✓ Todos los plugins descargados correctamente" << std::endl;
    } else {
        std::cerr << "⚠ Advertencia: " << shutdown_result.message << std::endl;
    }
    
    std::cout << "\n=== Demo Completada ===" << std::endl;
    
    return 0;
}
