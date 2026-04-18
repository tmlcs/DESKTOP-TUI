# Issue Técnico: Android NDK Backend

## Descripción
Implementar backend nativo para Android usando NDK con ANativeWindow para renderizado directo y AInputQueue para input handling.

## Objetivos
1. Renderizado directo vía ANativeWindow (sin escape sequences)
2. Input handling asíncrono con AInputQueue
3. Integración con Android lifecycle (pause, resume, destroy)
4. JNI layer para comunicación Java ↔ C++
5. Event loop basado en ALooper

## Archivos a Crear
### Core Platform
- `src/platform/terminal_ndk.cpp`
- `include/platform/terminal_ndk.hpp`
- `src/platform/input_ndk.cpp`
- `include/platform/input_ndk.hpp`

### Android Integration
- `src/platform/android_jni.cpp`
- `include/platform/android_jni.hpp`
- `src/platform/android_looper.cpp`
- `include/platform/android_looper.hpp`
- `src/platform/android_helpers.cpp`
- `include/platform/android_helpers.hpp`

### Ejemplo Android
- `examples/android/TUIActivity.java` (Kotlin también)
- `examples/android/CMakeLists.txt`
- `examples/android/AndroidManifest.xml`

## API Propuesta

### Terminal NDK
```cpp
namespace tui {

class NDKTerminal : public ITerminal {
public:
    explicit NDKTerminal(ANativeWindow* window);
    ~NDKTerminal() override;
    
    // Lifecycle
    bool init() override;
    void shutdown() override;
    
    // Screen control
    void clear() override;
    void flush() override;
    void sync() override;
    
    // Cursor (emulado en buffer)
    void cursor_hide() override;
    void cursor_show() override;
    void cursor_move(int x, int y) override;
    
    // Dimensions
    int cols() const override;
    int rows() const override;
    
    // Writing (renderiza directamente a ANativeWindow)
    void write(const std::string& text) override;
    void write_at(int x, int y, const std::string& text) override;
    void write_styled(const std::string& text, const Style& style) override;
    void write_styled_at(int x, int y, const std::string& text, 
                         const Style& style) override;
    
    // Fill region
    void fill(int x, int y, int w, int h, char ch, const Style& style) override;
    void clear_region(int x, int y, int w, int h) override;
    
    // Line drawing
    void draw_hline(int x, int y, int w, const Style& style) override;
    void draw_vline(int x, int y, int h, const Style& style) override;
    void draw_rect(int x, int y, int w, int h, const Style& style) override;
    
    // Mode control (no-op en NDK, siempre full-screen)
    void enter_raw_mode() override {}
    void leave_raw_mode() override {}
    void enter_alternate_screen() override {}
    void leave_alternate_screen() override {}
    
    // Title (vía JNI a Activity)
    void set_title(const std::string& title) override;
    
    // Capabilities
    TerminalCaps caps() const override;
    std::string term_type() const override { return "android-ndk"; }
    
    // Callbacks desde Java
    void on_surface_created(ANativeWindow* window);
    void on_surface_destroyed();
    void on_window_resized(int width, int height);
    
private:
    void render_buffer();
    void update_dimensions();
    
    ANativeWindow* window_;
    int cols_, rows_;
    int dpi_;
    float refresh_rate_;
    std::vector<Cell> cell_buffer_;
    bool dirty_;
};

} // namespace tui
```

### Input NDK
```cpp
namespace tui {

class NDKInput : public IInput {
public:
    explicit NDKInput(ALooper* looper);
    ~NDKInput() override;
    
    bool init() override;
    void shutdown() override;
    
    std::optional<Event> poll() override;
    bool has_input() override;
    
    int mouse_x() const override;
    int mouse_y() const override;
    
    // Callback desde Java
    void on_input_event(const AInputEvent* event);
    
private:
    Event process_key_event(const AKeyEvent* key_event);
    Event process_motion_event(const AMotionEvent* motion_event);
    
    ALooper* looper_;
    std::queue<Event> event_queue_;
    int current_mouse_x_, current_mouse_y_;
    std::mutex queue_mutex_;
};

} // namespace tui
```

### JNI Layer
```cpp
// android_jni.hpp
extern "C" {
    JNIEXPORT void JNICALL
    Java_com_example_tui_TUILib_init(JNIEnv* env, jobject thiz, 
                                     jobject surface);
    
    JNIEXPORT void JNICALL
    Java_com_example_tui_TUILib_resize(JNIEnv* env, jobject thiz, 
                                       jint width, jint height);
    
    JNIEXPORT void JNICALL
    Java_com_example_tui_TUILib_input(JNIEnv* env, jobject thiz, 
                                      jobject input_event);
    
    JNIEXPORT void JNICALL
    Java_com_example_tui_TUILib_pause(JNIEnv* env, jobject thiz);
    
    JNIEXPORT void JNICALL
    Java_com_example_tui_TUILib_resume(JNIEnv* env, jobject thiz);
    
    JNIEXPORT void JNICALL
    Java_com_example_tui_TUILib_destroy(JNIEnv* env, jobject thiz);
}
```

## Cambios en CMakeLists.txt
```cmake
if(ANDROID)
    # Backend nativo Android
    add_library(tui_platform SHARED
        src/platform/terminal_ndk.cpp
        src/platform/input_ndk.cpp
        src/platform/android_jni.cpp
        src/platform/android_looper.cpp
        src/platform/android_helpers.cpp
    )
    
    target_link_libraries(tui_platform PRIVATE
        android
        log
        input
        native_window
        aaudio  # opcional, para audio futuro
    )
    
    target_include_directories(tui_platform PRIVATE
        ${ANDROID_NDK}/sources/android/native_app_glue
    )
endif()
```

## Integración con Activity Java/Kotlin
```kotlin
// MainActivity.kt
class MainActivity : AppCompatActivity() {
    private lateinit var tuiLib: TUILib
    
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        
        val surfaceView = SurfaceView(this)
        setContentView(surfaceView)
        
        tuiLib = TUILib()
        surfaceView.holder.addCallback(object : SurfaceHolder.Callback {
            override fun surfaceCreated(holder: SurfaceHolder) {
                tuiLib.init(holder.surface)
            }
            
            override fun surfaceChanged(...) {
                tuiLib.resize(width, height)
            }
            
            override fun surfaceDestroyed(...) {
                tuiLib.destroy()
            }
        })
    }
}
```

## Criterios de Aceptación
- [ ] Compilación exitosa con NDK r25+
- [ ] App de ejemplo ejecutable en Android 5.0+ (API 21)
- [ ] 60fps sostenidos en dispositivo mid-range
- [ ] Input latency <16ms (1 frame)
- [ ] Manejo correcto de lifecycle (pause/resume)
- [ ] Zero crashes en pruebas de estrés
- [ ] Memory footprint <50MB

## Dependencias
- Android NDK r25 o superior
- Android SDK API 21+
- CMake 3.20+
- Ninja build system

## Testing
- Emulator Android (x86_64)
- Dispositivos físicos:
  - Samsung Galaxy (ARM)
  - Google Pixel (ARM)
  - Tablet Android (various)

## Documentación Adicional
- Guía de setup de entorno Android
- Tutorial: "Tu primera app TUI para Android"
- Troubleshooting common issues

## Prioridad: Media-Alta
## Estimación: 7-10 días
## Asignado: Pendiente
