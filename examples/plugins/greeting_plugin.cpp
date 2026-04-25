/**
 * @brief Plugin de ejemplo "Greeting" para Desktop TUI
 *
 * Este plugin demuestra cómo crear plugins personalizados
 * que pueden interactuar con la interfaz de usuario.
 */

#include "plugins/plugin_interface.hpp"
#include <iostream>
#include <string>

using namespace desktop_tui;

// ============================================================================
// Implementación del Plugin Greeting
// ============================================================================

class GreetingPlugin : public IPlugin {
public:
    GreetingPlugin() : execution_count_(0), greeting_("Hello") {}

    ~GreetingPlugin() override = default;

    PluginInfo get_info() const override {
        PluginInfo info;
        info.name = "greeting";
        info.version = "1.0.0";
        info.description = "Plugin de ejemplo que muestra mensajes de saludo";
        info.author = "Desktop TUI Team";
        info.is_core = false;
        info.dependencies = {};
        return info;
    }

    PluginResult initialize(PluginContext& context) override {
        context_ = &context;
        context.log_info("GreetingPlugin initialized");

        // Registrar callbacks de ejemplo
        context.register_callback("greet", [this]() {
            std::string name = "World";
            if (context_) {
                context_->log_info("Hello, " + name + "!");
            }
        });

        context.register_callback("farewell", []() {
            std::cout << "Goodbye!" << std::endl;
        });

        return PluginResult::ok("GreetingPlugin ready");
    }

    PluginResult execute() override {
        execution_count_++;

        if (context_) {
            context_->log_info("Greeting #" + std::to_string(execution_count_) +
                             " - " + greeting_ + " from Desktop TUI!");
        }

        return PluginResult::ok("Executed " + std::to_string(execution_count_) + " times");
    }

    PluginResult shutdown() override {
        if (context_) {
            context_->log_info("GreetingPlugin shutting down after " +
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

        if (command == "set-greeting") {
            if (!args.empty()) {
                greeting_ = args[0];
                return PluginResult::ok("Greeting set to: " + greeting_);
            }
            return PluginResult::fail("Usage: set-greeting <message>");
        }

        return IPlugin::handle_command(command, args);
    }

private:
    PluginContext* context_ = nullptr;
    size_t execution_count_ = 0;
    std::string greeting_ = "Hello";
};

// ============================================================================
// Funciones Factory exportadas (ABI estable)
// ============================================================================

extern "C" {

/**
 * @brief Crea una nueva instancia del plugin
 */
IPlugin* create_plugin() {
    return new GreetingPlugin();
}

/**
 * @brief Destruye una instancia del plugin
 */
void destroy_plugin(IPlugin* plugin) {
    delete plugin;
}

} // extern "C"
