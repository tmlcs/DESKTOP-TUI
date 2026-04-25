#ifndef TUI_UI_CONTEXT_MENU_HPP
#define TUI_UI_CONTEXT_MENU_HPP

#include <memory>
#include <string>
#include <vector>
#include <functional>

#include "core/rect.hpp"
#include "core/event.hpp"

namespace tui {

/**
 * @brief Representa un ítem de menú de contexto
 */
struct MenuItem {
    std::string label;          // Texto del ítem
    std::string tooltip;        // Tooltip opcional
    std::function<void()> action; // Acción al hacer clic
    bool enabled = true;        // Si el ítem está habilitado
};

/**
 * @brief Gestor de menús de contexto
 *
 * Maneja la creación, renderizado y manejo de eventos de menús de contexto
 * en la interfaz de usuario.
 */
class ContextMenu {
public:
    /**
     * @brief Constructor
     *
     * @param renderer Referencia al renderer para renderizar el menú
     */
    explicit ContextMenu(Renderer& renderer);

    /**
     * @brief Destructor
     */
    ~ContextMenu();

    /**
     * @brief Muestra el menú en la posición especificada
     *
     * @param x Coordenada X
     * @param y Coordenada Y
     * @param items Lista de ítems del menú
     * @return true Si el menú se mostró exitosamente
     * @return false Si falló mostrar el menú
     */
    bool show(int x, int y, const std::vector<MenuItem>& items);

    /**
     * @brief Oculta el menú
     */
    void hide();

    /**
     * @brief Verifica si el menú está visible
     *
     * @return true Si el menú está visible
     * @return false Si el menú no está visible
     */
    bool is_visible() const;

    /**
     * @brief Maneja eventos del menú
     *
     * @param e Evento a procesar
     * @return true Si el evento fue manejado
     * @return false Si el evento no fue manejado
     */
    bool handle_event(const Event& e);

    /**
     * @brief Obtiene el ítem seleccionado
     *
     * @return std::optional<int> Índice del ítem seleccionado o std::nullopt
     */
    std::optional<int> get_selected_item() const;

private:
    Renderer& m_renderer;      // Referencia al renderer
    bool m_visible = false;    // Estado de visibilidad
    std::optional<int> m_selected; // Ítem seleccionado
    Rect m_bounds;             // Bordes del menú
};

} // namespace tui

#endif // TUI_UI_CONTEXT_MENU_HPP
