// Performance benchmarks for Desktop TUI
#include "ui/renderer.hpp"
#include "ui/panel.hpp"
#include "core/string_utils.hpp"
#include "platform/terminal.hpp"
#include <cstdio>
#include <chrono>
#include <vector>
#include <string>

struct BenchmarkResult {
    const char* name;
    double ops_per_sec;
    double total_ms;
};

static std::vector<BenchmarkResult> results;

class NullTerminal : public tui::ITerminal {
public:
    bool init() override { return true; }
    int cols() const override { return 80; }
    int rows() const override { return 24; }
    void shutdown() override {}
    void enter_raw_mode() override {}
    void leave_raw_mode() override {}
    void enter_alternate_screen() override {}
    void leave_alternate_screen() override {}
    void cursor_hide() override {}
    void cursor_show() override {}
    void cursor_move(int, int) override {}
    void cursor_save() override {}
    void cursor_restore() override {}
    void write(const std::string&) override {}
    void write_at(int, int, const std::string&) override {}
    void write_styled(const std::string&, const tui::Style&) override {}
    void write_styled_at(int, int, const std::string&, const tui::Style&) override {}
    void fill(int, int, int, int, char, const tui::Style&) override {}
    void clear_region(int, int, int, int) override {}
    void draw_hline(int, int, int, const tui::Style&) override {}
    void draw_vline(int, int, int, const tui::Style&) override {}
    void draw_rect(int, int, int, int, const tui::Style&) override {}
    void draw_box(int, int, int, int, const tui::Style&, const tui::Style&) override {}
    void clear() override {}
    void flush() override {}
    void sync() override {}
    void set_title(const std::string&) override {}
    bool has_cap(tui::TerminalCaps) const override { return true; }
    tui::TerminalCaps caps() const override { return tui::TerminalCaps::BoxDrawing | tui::TerminalCaps::TrueColor; }
    std::string term_type() const override { return "null"; }
};

template<typename Func>
double benchmark(const char* name, int iterations, Func&& func) {
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; i++) {
        func();
    }
    auto end = std::chrono::high_resolution_clock::now();
    double elapsed_ms = std::chrono::duration<double, std::milli>(end - start).count();
    double ops_per_sec = (iterations / elapsed_ms) * 1000.0;
    results.push_back({name, ops_per_sec, elapsed_ms});
    return ops_per_sec;
}

void bench_display_width() {
    printf("\n=== display_width benchmark ===\n");
    std::string ascii = "Hello, World! This is a test string with 50 characters.";
    std::string mixed = "Hello 日本語 한국어 中文 테스트";
    std::string cjk = "日本語テスト한국어中文";

    benchmark("display_width ASCII (50 chars)", 1000000, [&]() {
        volatile auto r = tui::display_width(ascii); (void)r;
    });
    benchmark("display_width mixed UTF-8", 500000, [&]() {
        volatile auto r = tui::display_width(mixed); (void)r;
    });
    benchmark("display_width CJK (30 chars)", 500000, [&]() {
        volatile auto r = tui::display_width(cjk); (void)r;
    });
}

void bench_truncate() {
    printf("\n=== truncate benchmark ===\n");
    std::string long_text = "This is a long string with many characters for testing truncate performance";
    std::string long_cjk = "日本語テスト日本語テスト日本語テスト日本語テスト";

    benchmark("truncate ASCII to 20", 500000, [&]() {
        volatile auto r = tui::truncate(long_text, 20); (void)r;
    });
    benchmark("truncate CJK to 10", 200000, [&]() {
        volatile auto r = tui::truncate(long_cjk, 10); (void)r;
    });
}

void bench_render_write() {
    printf("\n=== Renderer::write benchmark ===\n");
    NullTerminal term;
    tui::Renderer r(term);
    r.resize(80, 24);

    benchmark("write ASCII (50 chars)", 500000, [&]() {
        r.write(0, 0, "Hello, World! This is a test string with 50 chars.");
    });
    benchmark("write CJK (15 chars)", 200000, [&]() {
        r.write(0, 0, "日本語テスト123");
    });
}

void bench_render_full_frame() {
    printf("\n=== Full frame render benchmark ===\n");
    NullTerminal term;
    tui::Renderer r(term);
    r.resize(80, 24);

    benchmark("clear + fill 80x24", 1000, [&]() {
        r.clear();
        r.fill_rect(0, 0, 80, 24, ' ', tui::Style::Default());
    });
    benchmark("draw_box 40x10", 50000, [&]() {
        r.draw_box({10, 5, 40, 10}, tui::Style::Default(), tui::Style::Default());
    });
    benchmark("write_center", 100000, [&]() {
        r.write_center(0, 12, "Centered Text", 80, tui::Style::Default());
    });
}

void bench_emit_style() {
    printf("\n=== emit_style_to_string benchmark ===\n");
    tui::Style default_style;
    tui::Style bold_style; bold_style.bold = true;
    tui::Style complex_style;
    complex_style.bold = true; complex_style.italic = true; complex_style.underline = true;
    complex_style.fg.mode = tui::Color::Mode::TrueColor;
    complex_style.fg.rgb = {255, 128, 64};
    complex_style.bg.mode = tui::Color::Mode::TrueColor;
    complex_style.bg.rgb = {0, 100, 200};

    benchmark("emit_style default", 1000000, [&]() {
        volatile auto r = tui::emit_style_to_string(default_style); (void)r;
    });
    benchmark("emit_style bold", 1000000, [&]() {
        volatile auto r = tui::emit_style_to_string(bold_style); (void)r;
    });
    benchmark("emit_style complex (truecolor)", 500000, [&]() {
        volatile auto r = tui::emit_style_to_string(complex_style); (void)r;
    });
}

int main() {
    printf("\n========================================\n");
    printf("  Desktop TUI Performance Benchmarks\n");
    printf("========================================\n");

    bench_display_width();
    bench_truncate();
    bench_render_write();
    bench_render_full_frame();
    bench_emit_style();

    printf("\n========================================\n");
    printf("  Summary\n");
    printf("========================================\n");
    printf("%-45s %15s %12s\n", "Benchmark", "ops/sec", "total ms");
    printf("%-45s %15s %12s\n", "-------------------------------------------", "---------------", "------------");
    for (const auto& r : results) {
        printf("%-45s %15.0f %11.1f\n", r.name, r.ops_per_sec, r.total_ms);
    }
    printf("\n");

    return 0;
}
