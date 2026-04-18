#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

namespace tui {

/**
 * @brief Capabilidades gráficas detectadas en el terminal
 */
enum class GraphicsCaps : uint32_t {
    None          = 0,
    Braille       = 1 << 0,  // Caracteres Braille Unicode (⠁⠂⠃...)
    ASCIIGraphics = 1 << 1,  // Box-drawing ASCII básico
    Sixel         = 1 << 2,  // Protocolo Sixel para imágenes
    Kitty         = 1 << 3,  // Kitty Graphics Protocol
    iTerm2        = 1 << 4,  // iTerm2 Inline Images
    TrueColor     = 1 << 5,  // Soporte RGB 24-bit
    Color256      = 1 << 6,  // Paleta 256 colores
};

inline GraphicsCaps operator|(GraphicsCaps a, GraphicsCaps b) {
    return static_cast<GraphicsCaps>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline GraphicsCaps operator&(GraphicsCaps a, GraphicsCaps b) {
    return static_cast<GraphicsCaps>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

inline bool has_flag(GraphicsCaps caps, GraphicsCaps flag) {
    return (static_cast<uint32_t>(caps) & static_cast<uint32_t>(flag)) != 0;
}

/**
 * @brief Información completa del terminal detectado
 */
struct TerminalInfo {
    std::string term;              // Variable $TERM
    std::string term_program;      // Variable $TERM_PROGRAM (ej: "Apple_Terminal", "iTerm.app")
    std::string colorterm;         // Variable $COLORTERM
    
    GraphicsCaps graphics_caps;    // Capacidades gráficas bitfield
    
    bool supports_braille;         // Detección explícita Braille
    bool supports_sixel;           // Detección explícita Sixel
    bool supports_kitty;           // Detección explícita Kitty
    bool supports_iterm2;          // Detección explícita iTerm2
    bool supports_truecolor;       // 24-bit color
    bool supports_256color;        // 256 color palette
    
    int max_colors;                // Número máximo de colores soportados
    int terminal_id;               // DA1 response code
    int terminal_version;          // DA2 version number
    
    std::string detected_font;     // Fuente detectada (si aplica)
    bool is_unicode;               // Soporte UTF-8 confirmado
    
    // Métodos de utilidad
    bool can_render_high_res() const {
        return supports_braille || has_flag(graphics_caps, GraphicsCaps::Sixel);
    }
    
    bool can_render_images() const {
        return supports_sixel || supports_kitty || supports_iterm2;
    }
    
    std::string to_string() const;
};

/**
 * @brief Sistema de detección de capacidades del terminal
 * 
 * Detecta automáticamente las capacidades del terminal donde se ejecuta
 * la aplicación, permitiendo habilitar características avanzadas de forma
 * dinámica y segura con fallback graceful.
 */
class CapabilityDetector {
public:
    /**
     * @brief Realiza detección completa de capacidades
     * @return TerminalInfo con todas las capacidades detectadas
     * 
     * Proceso de detección:
     * 1. Lee variables de entorno (TERM, COLORTERM, TERM_PROGRAM)
     * 2. Consulta Device Attributes (DA1/DA2) vía secuencias ESC
     * 3. Testea soporte Braille con caracteres de prueba
     * 4. Detecta protocolos gráficos (Sixel, Kitty, iTerm2)
     * 5. Cachea resultados para evitar re-detección
     */
    static TerminalInfo detect();
    
    /**
     * @brief Obtiene información cached de la última detección
     * @return TerminalInfo cached o llama a detect() si no existe cache
     */
    static TerminalInfo get_cached_info();
    
    /**
     * @brief Limpia el cache de detección
     */
    static void clear_cache();
    
    /**
     * @brief Fuerza re-detección ignorando cache
     * @return TerminalInfo actualizado
     */
    static TerminalInfo force_re_detect();

private:
    // Variables de entorno
    static std::string read_env_var(const std::string& name);
    static bool check_colorterm();
    
    // Device Attributes queries
    static int query_da1();  // Primary Device Attributes: ESC [ c
    static int query_da2();  // Secondary Device Attributes: ESC [ > c
    
    // Tests específicos
    static bool test_braille_support();
    static bool test_sixel_support();
    static bool test_kitty_support();
    static bool test_iterm2_support();
    static bool test_truecolor_support();
    static bool test_256color_support();
    static bool test_unicode_support();
    
public:
    // Tests específicos (públicos para testing via TestAPI)
    struct TestAPI {
        static bool test_braille_support();
        static bool test_sixel_support();
        static bool test_kitty_support();
        static bool test_iterm2_support();
        static bool test_truecolor_support();
        static bool test_256color_support();
        static bool test_unicode_support();
        
        // Heurísticas basadas en TERM (accesibles via TestAPI para testing)
        static GraphicsCaps infer_from_term(const std::string& term);
        static bool is_known_sixel_terminal(const std::string& term);
        static bool is_known_kitty_terminal(const std::string& term);
    };
    
    // Public wrappers for test access
    static GraphicsCaps infer_from_term(const std::string& term) {
        return TestAPI::infer_from_term(term);
    }
    static bool is_known_sixel_terminal(const std::string& term) {
        return TestAPI::is_known_sixel_terminal(term);
    }
    static bool is_known_kitty_terminal(const std::string& term) {
        return TestAPI::is_known_kitty_terminal(term);
    }
    
private:
    // Cache
    static TerminalInfo cached_info_;
    static bool cache_valid_;
};

// Make test methods accessible via TestAPI alias
using CapabilityDetectorPrivate = CapabilityDetector::TestAPI;

} // namespace tui
