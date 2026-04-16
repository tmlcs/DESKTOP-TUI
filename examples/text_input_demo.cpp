#include "desktop_manager.hpp"
#include "window.hpp"
#include "panel.hpp"
#include "label.hpp"
#include "text_input.hpp"
#include "border.hpp"
#include "terminal_posix.hpp"
#include "input_posix.hpp"
#include "renderer.hpp"
#include "colors.hpp"
#include <memory>
#include <iostream>

using namespace tui;

int main() {
    // Inicializar terminal
    PosixTerminal terminal;
    PosixInput input;
    
    if (!terminal.init()) {
        std::cerr << "Failed to initialize terminal\n";
        return 1;
    }
    
    if (!input.init()) {
        std::cerr << "Failed to initialize input\n";
        return 1;
    }
    
    terminal.enable_raw_mode();
    terminal.hide_cursor();
    terminal.clear();
    
    // Crear Desktop Manager
    DesktopManager dm;
    dm.add_desktop();
    
    // Crear ventana principal
    auto window = std::make_shared<Window>(Rect{2, 2, 76, 20});
    window->set_title("TextInput Demo");
    window->set_visible(true);
    
    // Crear panel de contenido
    auto content = std::make_shared<Panel>();
    content->set_bounds(Rect{0, 0, 72, 16});
    
    // --- Widget 1: Label de título ---
    auto title_label = std::make_shared<Label>("=== TextInput Widget Demo ===");
    title_label->set_bounds(Rect{20, 1, 32, 1});
    Style title_style;
    title_style.fg_color = Color::CYAN;
    title_style.bold = true;
    title_label->set_style(title_style);
    content->add_child(title_label);
    
    // --- Widget 2: Input normal ---
    auto name_label = std::make_shared<Label>("Nombre:");
    name_label->set_bounds(Rect{5, 4, 10, 1});
    content->add_child(name_label);
    
    auto name_input = std::make_shared<TextInput>();
    name_input->set_bounds(Rect{16, 4, 40, 1});
    name_input->set_placeholder("Ingresa tu nombre...");
    content->add_child(name_input);
    
    // --- Widget 3: Input con máscara (password) ---
    auto pass_label = std::make_shared<Label>("Password:");
    pass_label->set_bounds(Rect{5, 6, 10, 1});
    content->add_child(pass_label);
    
    auto pass_input = std::make_shared<TextInput>();
    pass_input->set_bounds(Rect{16, 6, 40, 1});
    pass_input->set_placeholder("Contraseña");
    pass_input->set_mask_char(U'●'); // Caracter de máscara
    pass_input->set_max_length(20);
    content->add_child(pass_input);
    
    // --- Widget 4: Input con longitud máxima ---
    auto code_label = std::make_shared<Label>("Código (max 5):");
    code_label->set_bounds(Rect{5, 8, 15, 1});
    content->add_child(code_label);
    
    auto code_input = std::make_shared<TextInput>();
    code_input->set_bounds(Rect{21, 8, 10, 1});
    code_input->set_placeholder("ABC12");
    code_input->set_max_length(5);
    content->add_child(code_input);
    
    // --- Widget 5: Label de instrucciones ---
    auto instr_label = std::make_shared<Label>("Tab: Cambiar foco | Ctrl+C/X/V: Copiar/Cortar/Pegar | Esc: Salir");
    instr_label->set_bounds(Rect{8, 12, 56, 1});
    Style instr_style;
    instr_style.fg_color = Color::DARK_GRAY;
    instr_label->set_style(instr_style);
    content->add_child(instr_label);
    
    // --- Widget 6: Label dinámico para mostrar valores ---
    auto value_label = std::make_shared<Label>("");
    value_label->set_bounds(Rect{5, 14, 60, 1});
    Style value_style;
    value_style.fg_color = Color::GREEN;
    value_label->set_style(value_style);
    content->add_child(value_label);
    
    window->set_content(content);
    dm.active_desktop()->add_window(window);
    
    // Enfocar primer input
    name_input->set_focus();
    
    // Event loop
    bool running = true;
    Renderer renderer(terminal);
    
    while (running) {
        // Poll eventos
        auto events = input.poll();
        
        for (const auto& event : events) {
            if (event.type == EventType::Quit || 
                (event.type == EventType::KeyPress && event.key_code == KeyCode::Escape)) {
                running = false;
                break;
            }
            
            if (event.type == EventType::Resize) {
                terminal.get_size(renderer.width_, renderer.height_);
                renderer.set_dirty();
            }
            
            // Dispatch evento al desktop manager
            dm.handle_event(event);
            
            // Actualizar label dinámico con valores de los inputs
            if (event.type == EventType::KeyPress || event.type == EventType::MouseDown) {
                std::string status = "Nombre: '" + name_input->value() + "' | ";
                status += "Pass: '" + pass_input->value() + "' | ";
                status += "Code: '" + code_input->value() + "'";
                value_label->set_text(status);
            }
            
            // Navegación con Tab entre inputs
            if (event.type == EventType::KeyPress && event.key_code == KeyCode::Tab) {
                if (name_input->has_focus()) {
                    pass_input->set_focus();
                } else if (pass_input->has_focus()) {
                    code_input->set_focus();
                } else if (code_input->has_focus()) {
                    name_input->set_focus();
                }
            }
        }
        
        // Render
        renderer.clear();
        dm.draw(renderer);
        renderer.flush();
        
        // Update widgets (para animaciones como blink del cursor)
        dm.update(0.05); // 50ms frame time approx
    }
    
    // Cleanup
    terminal.show_cursor();
    terminal.disable_raw_mode();
    terminal.clear();
    
    std::cout << "\nFinal values:\n";
    std::cout << "  Nombre: " << name_input->value() << "\n";
    std::cout << "  Password: " << pass_input->value() << "\n";
    std::cout << "  Code: " << code_input->value() << "\n";
    
    return 0;
}
