/**
 * @brief Plugin de ejemplo "Hello World" para Desktop TUI
 * 
 * Este plugin demuestra cómo implementar la interfaz IPlugin
 * y puede ser usado como template para crear plugins personalizados.
 */

#include "../../src/plugins/plugin_interface.hpp"
#include <iostream>

using namespace desktop_tui;

// ============================================================================
// Implementación del Plugin Hello World
// ============================================================================

class HelloWorldPlugin : public IPlugin {
public:
    HelloWorldPlugin() : execution_count_(0) {}
    
    ~HelloWorldPlugin() override = default;
    
    PluginInfo get_info() const override {
        PluginInfo info;
        info.name = "hello_world";
        info.version = "1.0.0";
        info.description = "Plugin de ejemplo que muestra mensajes Hello World";
        info.author = "Desktop TUI Team";
        info.is_core = false;
        return info;
    }
    
    PluginResult initialize(PluginContext& context) override {
        context_ = &context;
        context.log_info("HelloWorldPlugin initialized");
        
        // Registrar un callback de ejemplo
        context.register_callback("hello_action", []() {
            std::cout << "[CALLBACK] Hello action triggered!" << std::endl;
        });
        
        return PluginResult::ok("HelloWorldPlugin ready");
    }
    
    PluginResult execute() override {
        execution_count_++;
        
        if (context_) {
            context_->log_info("Execution #" + std::to_string(execution_count_) + 
                             " - Hello from Desktop TUI!");
        }
        
        // No sleep in execute - solo contar ejecuciones
        return PluginResult::ok("Executed " + std::to_string(execution_count_) + " times");
    }
    
    PluginResult shutdown() override {
        if (context_) {
            context_->log_info("HelloWorldPlugin shutting down after " + 
                             std::to_string(execution_count_) + " executions");
        }
        
        execution_count_ = 0;
        context_ = nullptr;
        
        return PluginResult::ok("Shutdown complete");
    }
    
    PluginResult handle_command(const std::string& command,
                               const std::vector<std::string>& args) override {
        if (command == "greet") {
            std::string name = args.empty() ? "World" : args[0];
            if (context_) {
                context_->log_info("Hello, " + name + "!");
            }
            return PluginResult::ok("Greeting sent to " + name);
        }
        
        if (command == "count") {
            return PluginResult::ok("Execution count: " + std::to_string(execution_count_));
        }
        
        return IPlugin::handle_command(command, args);
    }

private:
    PluginContext* context_ = nullptr;
    size_t execution_count_ = 0;
};

// ============================================================================
// Funciones Factory exportadas (ABI estable)
// ============================================================================

extern "C" {

/**
 * @brief Crea una nueva instancia del plugin
 * Esta función debe ser exportada por todos los plugins
 */
IPlugin* create_plugin() {
    return new HelloWorldPlugin();
}

/**
 * @brief Destruye una instancia del plugin
 * Esta función debe ser exportada por todos los plugins
 */
void destroy_plugin(IPlugin* plugin) {
    delete plugin;
}

} // extern "C"
