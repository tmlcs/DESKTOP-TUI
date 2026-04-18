#include "core/capability_detector.hpp"
#include "core/logger.hpp"
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <sys/select.h>
#include <termios.h>

namespace tui {

// Variables estáticas de cache
TerminalInfo CapabilityDetector::cached_info_{};
bool CapabilityDetector::cache_valid_ = false;

std::string TerminalInfo::to_string() const {
    std::string result;
    result += "Terminal Info:\n";
    result += "  TERM: " + term + "\n";
    result += "  TERM_PROGRAM: " + term_program + "\n";
    result += "  COLORTERM: " + colorterm + "\n";
    result += "  Max Colors: " + std::to_string(max_colors) + "\n";
    result += "  Unicode: " + std::string(is_unicode ? "Yes" : "No") + "\n";
    result += "  Graphics Capabilities:\n";
    
    if (supports_braille) result += "    ✓ Braille\n";
    if (supports_sixel) result += "    ✓ Sixel\n";
    if (supports_kitty) result += "    ✓ Kitty Graphics\n";
    if (supports_iterm2) result += "    ✓ iTerm2 Inline Images\n";
    if (supports_truecolor) result += "    ✓ True Color (24-bit)\n";
    if (supports_256color) result += "    ✓ 256 Colors\n";
    
    if (can_render_high_res()) {
        result += "  → Can render high-resolution graphics\n";
    }
    if (can_render_images()) {
        result += "  → Can render inline images\n";
    }
    
    return result;
}

std::string CapabilityDetector::read_env_var(const std::string& name) {
    const char* value = std::getenv(name.c_str());
    return value ? std::string(value) : "";
}

bool CapabilityDetector::check_colorterm() {
    std::string colorterm = read_env_var("COLORTERM");
    return colorterm == "truecolor" || colorterm == "24bit";
}

int CapabilityDetector::query_da1() {
    // Primary Device Attributes: ESC [ c
    // Response: ESC [ ? 6 2 ; param c (xterm compatible)
    // El parámetro indica características soportadas
    
    struct termios oldt, newt;
    if (tcgetattr(STDIN_FILENO, &oldt) != 0) {
        TUI_LOG(LogLevel::WARN, "Failed to get terminal attributes for DA1 query");
        return 0;
    }
    
    // Configurar terminal en modo raw temporalmente
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    newt.c_cc[VMIN] = 0;
    newt.c_cc[VTIME] = 1; // 100ms timeout
    
    if (tcsetattr(STDIN_FILENO, TCSANOW, &newt) != 0) {
        return 0;
    }
    
    // Enviar secuencia DA1
    write(STDOUT_FILENO, "\033[c", 3);
    fsync(STDOUT_FILENO);
    
    // Leer respuesta con timeout
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);
    
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100000; // 100ms
    
    int ret = select(STDIN_FILENO + 1, &readfds, nullptr, nullptr, &tv);
    
    std::string response;
    if (ret > 0 && FD_ISSET(STDIN_FILENO, &readfds)) {
        char buf[64];
        ssize_t n = read(STDIN_FILENO, buf, sizeof(buf));
        if (n > 0) {
            response = std::string(buf, n);
            TUI_LOG(LogLevel::DEBUG, "DA1 Response: %s", response.c_str());
            
            // Parsear respuesta: ESC [ ? 6 2 ; params c
            // Extraer terminal ID
            size_t pos = response.find(';');
            if (pos != std::string::npos && pos + 1 < response.size()) {
                try {
                    int id = std::stoi(response.substr(pos + 1));
                    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
                    return id;
                } catch (...) {
                    // Ignorar errores de parseo
                }
            }
        }
    }
    
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return 0;
}

int CapabilityDetector::query_da2() {
    // Secondary Device Attributes: ESC [ > c
    // Usado para identificar tipo específico de terminal
    
    struct termios oldt, newt;
    if (tcgetattr(STDIN_FILENO, &oldt) != 0) {
        return 0;
    }
    
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    newt.c_cc[VMIN] = 0;
    newt.c_cc[VTIME] = 1;
    
    if (tcsetattr(STDIN_FILENO, TCSANOW, &newt) != 0) {
        return 0;
    }
    
    // Enviar secuencia DA2
    write(STDOUT_FILENO, "\033[>c", 4);
    fsync(STDOUT_FILENO);
    
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);
    
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100000;
    
    int ret = select(STDIN_FILENO + 1, &readfds, nullptr, nullptr, &tv);
    
    if (ret > 0 && FD_ISSET(STDIN_FILENO, &readfds)) {
        char buf[64];
        ssize_t n = read(STDIN_FILENO, buf, sizeof(buf));
        if (n > 0) {
            std::string response(buf, n);
            TUI_LOG(LogLevel::DEBUG, "DA2 Response: %s", response.c_str());
            
            // Parsear versión del terminal
            size_t pos = response.find(';');
            if (pos != std::string::npos && pos + 1 < response.size()) {
                try {
                    int version = std::stoi(response.substr(pos + 1));
                    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
                    return version;
                } catch (...) {
                    // Ignorar errores de parseo
                }
            }
        }
    }
    
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return 0;
}

// Implementación de TestAPI methods
bool CapabilityDetector::TestAPI::test_braille_support() {
    // Testear si el terminal puede renderizar caracteres Braille
    // Los caracteres Braille están en el rango U+2800 a U+28FF
    
    // Estrategia 1: Verificar soporte Unicode general
    std::string lang = CapabilityDetector::read_env_var("LANG");
    bool has_utf8 = lang.find("UTF-8") != std::string::npos || 
                    lang.find("UTF8") != std::string::npos;
    
    if (!has_utf8) {
        return false;
    }
    
    // Estrategia 2: Heurística basada en TERM
    std::string term = CapabilityDetector::read_env_var("TERM");
    
    // Terminales modernos generalmente soportan Braille
    bool known_good = 
        term.find("xterm") != std::string::npos ||
        term.find("screen") != std::string::npos ||
        term.find("tmux") != std::string::npos ||
        term.find("kitty") != std::string::npos ||
        term.find("alacritty") != std::string::npos ||
        term.find("wezterm") != std::string::npos ||
        term.find("foot") != std::string::npos;
    
    if (known_good) {
        return true;
    }
    
    // Estrategia 3: Test directo (opcional, requiere interacción)
    // En producción, podríamos escribir un carácter Braille y verificar
    // si el terminal responde adecuadamente, pero esto es complejo.
    // Por defecto, asumimos que si hay UTF-8, hay soporte Braille.
    
    return has_utf8;
}

bool CapabilityDetector::TestAPI::test_sixel_support() {
    // Detectar soporte Sixel
    
    // Estrategia 1: Variable de entorno
    std::string sixel = CapabilityDetector::read_env_var("SIXEL_SUPPORT");
    if (sixel == "1" || sixel == "yes" || sixel == "true") {
        return true;
    }
    
    // Estrategia 2: TERM conocido
    std::string term = CapabilityDetector::read_env_var("TERM");
    return TestAPI::is_known_sixel_terminal(term);
}

bool CapabilityDetector::TestAPI::is_known_sixel_terminal(const std::string& term) {
    // Lista de terminales conocidos por soportar Sixel
    static const char* sixel_terms[] = {
        "xterm-directcolor",
        "xterm+direct2",
        "xterm+direct",
        "mlterm",
        "yaft",
        "foot",
        "wezterm",
        "kitty",  // kitty tiene su propio protocolo pero puede emular sixel
        nullptr
    };
    
    for (const char* sterm : sixel_terms) {
        if (sterm && term.find(sterm) != std::string::npos) {
            return true;
        }
    }
    
    return false;
}

bool CapabilityDetector::TestAPI::test_kitty_support() {
    // Detectar protocolo Kitty Graphics
    
    // Estrategia 1: TERM_PROGRAM o TERM
    std::string term = CapabilityDetector::read_env_var("TERM");
    std::string term_program = CapabilityDetector::read_env_var("TERM_PROGRAM");
    
    if (term.find("kitty") != std::string::npos) {
        return true;
    }
    
    // Estrategia 2: Variable específica
    std::string kitty = CapabilityDetector::read_env_var("KITTY_WINDOW_ID");
    if (!kitty.empty()) {
        return true;
    }
    
    return TestAPI::is_known_kitty_terminal(term);
}

bool CapabilityDetector::TestAPI::is_known_kitty_terminal(const std::string& term) {
    return term.find("kitty") != std::string::npos;
}

bool CapabilityDetector::TestAPI::test_iterm2_support() {
    // Detectar iTerm2 en macOS
    
    std::string term_program = CapabilityDetector::read_env_var("TERM_PROGRAM");
    return term_program == "Apple_Terminal" || 
           term_program == "iTerm.app" ||
           term_program.find("iTerm") != std::string::npos;
}

bool CapabilityDetector::TestAPI::test_truecolor_support() {
    // Detectar soporte True Color (24-bit)
    
    // Estrategia 1: COLORTERM
    if (CapabilityDetector::check_colorterm()) {
        return true;
    }
    
    // Estrategia 2: TERM conocido
    std::string term = CapabilityDetector::read_env_var("TERM");
    
    bool known_truecolor = 
        term.find("direct") != std::string::npos ||
        term.find("truecolor") != std::string::npos ||
        term.find("24bit") != std::string::npos ||
        term.find("kitty") != std::string::npos ||
        term.find("alacritty") != std::string::npos ||
        term.find("wezterm") != std::string::npos ||
        term.find("foot") != std::string::npos ||
        term.find("iterm") != std::string::npos;
    
    if (known_truecolor) {
        return true;
    }
    
    // Estrategia 3: Heurística basada en $TERM moderno
    // xterm-256color y similares generalmente soportan truecolor hoy en día
    if (term.find("xterm-256color") != std::string::npos) {
        // Verificar si estamos en un sistema moderno
        std::string colorterm = CapabilityDetector::read_env_var("COLORTERM");
        return !colorterm.empty(); // Si COLORTERM está set, probablemente hay truecolor
    }
    
    return false;
}

bool CapabilityDetector::TestAPI::test_256color_support() {
    // Detectar soporte 256 colores
    
    std::string term = CapabilityDetector::read_env_var("TERM");
    
    bool has_256 = 
        term.find("256") != std::string::npos ||
        term.find("direct") != std::string::npos ||
        term.find("truecolor") != std::string::npos;
    
    if (has_256) {
        return true;
    }
    
    // La mayoría de los terminales modernos soportan al menos 256 colores
    // incluso si no lo declaran explícitamente
    std::string colorterm = CapabilityDetector::read_env_var("COLORTERM");
    return !colorterm.empty();
}

bool CapabilityDetector::TestAPI::test_unicode_support() {
    // Detectar soporte UTF-8
    
    std::string lang = CapabilityDetector::read_env_var("LANG");
    std::string lc_all = CapabilityDetector::read_env_var("LC_ALL");
    std::string lc_ctype = CapabilityDetector::read_env_var("LC_CTYPE");
    
    bool has_utf8 = 
        lang.find("UTF-8") != std::string::npos ||
        lang.find("UTF8") != std::string::npos ||
        lc_all.find("UTF-8") != std::string::npos ||
        lc_ctype.find("UTF-8") != std::string::npos;
    
    return has_utf8;
}

GraphicsCaps CapabilityDetector::TestAPI::infer_from_term(const std::string& term) {
    GraphicsCaps caps = GraphicsCaps::None;
    
    // Inferir capacidades basadas en el nombre del terminal
    if (term.find("kitty") != std::string::npos) {
        caps = caps | GraphicsCaps::Kitty | GraphicsCaps::TrueColor | GraphicsCaps::Braille;
    } else if (term.find("alacritty") != std::string::npos) {
        caps = caps | GraphicsCaps::TrueColor | GraphicsCaps::Braille;
    } else if (term.find("wezterm") != std::string::npos) {
        caps = caps | GraphicsCaps::TrueColor | GraphicsCaps::Braille | GraphicsCaps::Sixel;
    } else if (term.find("foot") != std::string::npos) {
        caps = caps | GraphicsCaps::TrueColor | GraphicsCaps::Braille | GraphicsCaps::Sixel;
    } else if (term.find("iterm") != std::string::npos) {
        caps = caps | GraphicsCaps::iTerm2 | GraphicsCaps::TrueColor | GraphicsCaps::Braille;
    } else if (term.find("xterm") != std::string::npos) {
        caps = caps | GraphicsCaps::ASCIIGraphics;
        if (term.find("256") != std::string::npos) {
            caps = caps | GraphicsCaps::Color256;
        }
        if (term.find("direct") != std::string::npos) {
            caps = caps | GraphicsCaps::TrueColor | GraphicsCaps::Sixel;
        }
    } else if (term.find("screen") != std::string::npos || 
               term.find("tmux") != std::string::npos) {
        // Multiplexores: capacidades limitadas pero generalmente UTF-8
        caps = caps | GraphicsCaps::ASCIIGraphics | GraphicsCaps::Braille;
    }
    
    return caps;
}

TerminalInfo CapabilityDetector::detect() {
    TUI_LOG(LogLevel::INFO, "Starting terminal capability detection...");
    
    TerminalInfo info{};
    
    // 1. Leer variables de entorno
    info.term = read_env_var("TERM");
    info.term_program = read_env_var("TERM_PROGRAM");
    info.colorterm = read_env_var("COLORTERM");
    
    TUI_LOG(LogLevel::DEBUG, "TERM=%s, TERM_PROGRAM=%s, COLORTERM=%s", 
                  info.term.c_str(), info.term_program.c_str(), info.colorterm.c_str());
    
    // 2. Inferir capacidades iniciales desde TERM
    info.graphics_caps = infer_from_term(info.term);
    
    // 3. Query Device Attributes
    info.terminal_id = query_da1();
    info.terminal_version = query_da2();
    
    TUI_LOG(LogLevel::DEBUG, "Terminal ID: %d, Version: %d", info.terminal_id, info.terminal_version);
    
    // 4. Tests específicos de capacidades (usando wrapper público para tests)
    info.is_unicode = CapabilityDetectorPrivate::test_unicode_support();
    info.supports_braille = CapabilityDetectorPrivate::test_braille_support();
    info.supports_sixel = CapabilityDetectorPrivate::test_sixel_support();
    info.supports_kitty = CapabilityDetectorPrivate::test_kitty_support();
    info.supports_iterm2 = CapabilityDetectorPrivate::test_iterm2_support();
    info.supports_truecolor = CapabilityDetectorPrivate::test_truecolor_support();
    info.supports_256color = CapabilityDetectorPrivate::test_256color_support();
    
    // 5. Actualizar graphics_caps con resultados de tests
    if (info.supports_braille) {
        info.graphics_caps = info.graphics_caps | GraphicsCaps::Braille;
    }
    if (info.supports_sixel) {
        info.graphics_caps = info.graphics_caps | GraphicsCaps::Sixel;
    }
    if (info.supports_kitty) {
        info.graphics_caps = info.graphics_caps | GraphicsCaps::Kitty;
    }
    if (info.supports_iterm2) {
        info.graphics_caps = info.graphics_caps | GraphicsCaps::iTerm2;
    }
    if (info.supports_truecolor) {
        info.graphics_caps = info.graphics_caps | GraphicsCaps::TrueColor;
    }
    if (info.supports_256color) {
        info.graphics_caps = info.graphics_caps | GraphicsCaps::Color256;
    }
    
    // 6. Determinar max_colors
    if (info.supports_truecolor) {
        info.max_colors = 16777216; // 2^24
    } else if (info.supports_256color) {
        info.max_colors = 256;
    } else {
        info.max_colors = 16; // ANSI básico
    }
    
    // 7. Logging de resumen
    TUI_LOG(LogLevel::INFO, "Detection complete:");
    TUI_LOG(LogLevel::INFO, "  Unicode: %s", info.is_unicode ? "Yes" : "No");
    TUI_LOG(LogLevel::INFO, "  Braille: %s", info.supports_braille ? "Yes" : "No");
    TUI_LOG(LogLevel::INFO, "  Sixel: %s", info.supports_sixel ? "Yes" : "No");
    TUI_LOG(LogLevel::INFO, "  Kitty: %s", info.supports_kitty ? "Yes" : "No");
    TUI_LOG(LogLevel::INFO, "  TrueColor: %s", info.supports_truecolor ? "Yes" : "No");
    TUI_LOG(LogLevel::INFO, "  Max Colors: %d", info.max_colors);
    
    // Cache results
    cached_info_ = info;
    cache_valid_ = true;
    
    return info;
}

TerminalInfo CapabilityDetector::get_cached_info() {
    if (!cache_valid_) {
        return detect();
    }
    return cached_info_;
}

void CapabilityDetector::clear_cache() {
    cache_valid_ = false;
    cached_info_ = TerminalInfo{};
    TUI_LOG(LogLevel::DEBUG, "Capability cache cleared");
}

TerminalInfo CapabilityDetector::force_re_detect() {
    clear_cache();
    return detect();
}

} // namespace tui
