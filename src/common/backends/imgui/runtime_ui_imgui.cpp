// In-game menu presentation for the renderer-independent runtime UI model.
// The host owns the ImGui context/frame/submission. This file only draws.
//
// Layout (TempleFrog house style): one rounded window over the dimmed game,
// a title, a row of tabs (the model's sections) with a gliding underline,
// option rows on the left (UPPERCASE label, "< value >"), the focused row's
// description on the right, and a prompt bar of button badges. Text is drawn
// with explicit font sizes relative to the window height, so it is equally
// sharp in a 720x480 window and at 4K. Fonts come from the host's atlas:
// Fonts[0] body, Fonts[1] bold, Fonts[2] title (any may be missing).

#include "recomp_runtime_ui_internal.h"
#include "launcher_theme.h"

#include "imgui.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace {

ImVec4 col(const LngColor &c, float alpha = 1.0f) {
    return ImVec4(c.r, c.g, c.b, c.a * alpha);
}

ImU32 u32(const LngColor &c, float alpha = 1.0f) {
    return ImGui::ColorConvertFloat4ToU32(col(c, alpha));
}

LngColor mix(const LngColor &a, const LngColor &b, float t) {
    return LngColor{a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t,
                    a.b + (b.b - a.b) * t, a.a + (b.a - a.a) * t};
}

// Frame-rate independent exponential approach (SuperWonders' helper).
float approach(float current, float target, float rate, float dt) {
    return target + (current - target) * std::exp(-rate * dt);
}

float smoothstep(float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

struct Fonts {
    ImFont *body = nullptr, *bold = nullptr, *title = nullptr;
};

Fonts fonts() {
    Fonts f;
    ImFontAtlas *atlas = ImGui::GetIO().Fonts;
    if (atlas && atlas->Fonts.Size > 0) f.body = atlas->Fonts[0];
    f.bold = atlas && atlas->Fonts.Size > 1 ? atlas->Fonts[1] : f.body;
    f.title = atlas && atlas->Fonts.Size > 2 ? atlas->Fonts[2] : f.bold;
    if (!f.body) f.body = f.bold = f.title = ImGui::GetFont();
    return f;
}

float text_w(ImFont *font, float size, const char *s) {
    return font->CalcTextSizeA(size, 1e9f, 0.0f, s).x;
}

void upper(const char *in, char *out, size_t n) {
    size_t i = 0;
    for (; in && in[i] && i + 1 < n; ++i) {
        const char c = in[i];
        out[i] = (c >= 'a' && c <= 'z') ? char(c - 'a' + 'A') : c;
    }
    out[i] = 0;
}

void value_text(RecompRuntimeUi *ui, const RecompRuntimeUiItem *item,
                char *out, size_t out_size) {
    if (item->type == RECOMP_RUNTIME_UI_ACTION) {
        out[0] = 0;
        return;
    }
    if (item->type == RECOMP_RUNTIME_UI_TEXT) {
        char buf[128];
        recomp_runtime_ui_current_text(ui, item, buf, sizeof(buf));
        std::snprintf(out, out_size, "%s", buf[0] ? buf : "(not set)");
        return;
    }
    int value = 0;
    if (!recomp_runtime_ui_current_value(ui, item, &value)) {
        std::snprintf(out, out_size, "Unavailable");
    } else if (item->type == RECOMP_RUNTIME_UI_BOOL) {
        std::snprintf(out, out_size, "%s", value ? "On" : "Off");
    } else if (item->type == RECOMP_RUNTIME_UI_CHOICE && item->choices) {
        size_t index = item->choice_count;
        if (item->choice_values) {
            for (size_t i = 0; i < item->choice_count; ++i)
                if (item->choice_values[i] == value) { index = i; break; }
        } else if (value >= item->minimum &&
                   static_cast<size_t>(value - item->minimum) < item->choice_count) {
            index = static_cast<size_t>(value - item->minimum);
        }
        if (index < item->choice_count)
            std::snprintf(out, out_size, "%s", item->choices[index]);
        else
            std::snprintf(out, out_size, "Unavailable");
    } else if (item->key &&
               std::strcmp(item->key,
                           RECOMP_RUNTIME_UI_KEY_GYRO_SENSITIVITY) == 0) {
        std::snprintf(out, out_size, "%.2fx",
                      static_cast<float>(value) / 100.0f);
    } else {
        std::snprintf(out, out_size, "%d", value);
    }
}

// Filled triangles rather than glyphs, so they look the same in every font.
void arrow(ImDrawList *d, float cx, float cy, float r, bool left, ImU32 c) {
    if (left)
        d->AddTriangleFilled(ImVec2(cx - r, cy), ImVec2(cx + r * 0.7f, cy - r),
                             ImVec2(cx + r * 0.7f, cy + r), c);
    else
        d->AddTriangleFilled(ImVec2(cx + r, cy), ImVec2(cx - r * 0.7f, cy - r),
                             ImVec2(cx - r * 0.7f, cy + r), c);
}

// Cream pill with a dark glyph, then a muted label. Returns the next x.
float badge(ImDrawList *d, const Fonts &f, const LauncherTheme &th, float x,
            float y, float h, const char *glyph, const char *label, float size) {
    const float gw = text_w(f.bold, size, glyph);
    const float pw = std::max(h, gw + h * 0.55f);
    d->AddRectFilled(ImVec2(x, y), ImVec2(x + pw, y + h), u32(th.text), h * 0.5f);
    d->AddText(f.bold, size, ImVec2(x + (pw - gw) * 0.5f, y + (h - size) * 0.5f),
               u32(th.background), glyph);
    x += pw;
    if (label && label[0]) {
        x += h * 0.30f;
        d->AddText(f.body, size, ImVec2(x, y + (h - size) * 0.5f), u32(th.text_muted), label);
        x += text_w(f.body, size, label) + h * 0.85f;
    } else {
        x += h * 0.25f;
    }
    return x;
}

// Word-wrapped text inside [x, x+w]; returns the y after the last line.
float wrapped(ImDrawList *d, ImFont *font, float size, ImVec2 pos, float w,
              ImU32 c, const char *s) {
    d->AddText(font, size, pos, c, s, nullptr, w);
    return pos.y + font->CalcTextSizeA(size, 1e9f, w, s).y;
}

struct Motion {
    bool was_open = false;
    float reveal = 0.0f;
    float tab_x = 0.0f, tab_w = 0.0f;
    bool tab_init = false;
    float row_y = 0.0f;
    bool row_init = false;
    size_t last_section = (size_t)-1;
};
Motion g_motion;

void push_runtime_style(const LauncherTheme &theme) {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 18.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_Text, col(theme.text));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, col(theme.control));
    ImGui::PushStyleColor(ImGuiCol_Border, col(theme.border));
}

void pop_runtime_style() {
    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar(3);
}

// The toast: a borderless panel centred near the top of the display, drawn
// with or without the menu. It takes no input and no focus.
void draw_toast(RecompRuntimeUi *ui, const LauncherTheme &theme) {
    ImGuiIO &io = ImGui::GetIO();
    const ImVec2 display = io.DisplaySize;
    if (display.x <= 0.0f || display.y <= 0.0f) return;
    const Fonts f = fonts();
    const float s = std::max(0.5f, display.y / 800.0f);
    const float size = 24.0f * s, pad = 16.0f * s;
    const char *title = ui->toast_title, *body = ui->toast_body;
    const float w = std::max(title[0] ? text_w(f.bold, size, title) : 0.0f,
                             body[0] ? text_w(f.body, size, body) : 0.0f) + pad * 2.0f;
    const float h = pad * 2.0f + (title[0] ? size * 1.25f : 0.0f) + (body[0] ? size * 1.25f : 0.0f);
    const ImVec2 p0((display.x - w) * 0.5f, 24.0f * s);
    ImDrawList *d = ImGui::GetForegroundDrawList();
    d->AddRectFilled(p0, ImVec2(p0.x + w, p0.y + h), u32(theme.panel, 0.95f * ui->opacity), 12.0f * s);
    d->AddRect(p0, ImVec2(p0.x + w, p0.y + h), u32(theme.accent, 0.9f), 12.0f * s, 0, 2.0f * s);
    float y = p0.y + pad;
    if (title[0]) { d->AddText(f.bold, size, ImVec2(p0.x + pad, y), u32(theme.accent2), title); y += size * 1.25f; }
    if (body[0]) d->AddText(f.body, size, ImVec2(p0.x + pad, y), u32(theme.text), body);
}

}  // namespace

extern "C" void recomp_runtime_ui_render_imgui(RecompRuntimeUi *ui) {
    if (!ui || ImGui::GetCurrentContext() == nullptr) return;
    LauncherTheme th = launcher_theme_by_name(ui->config.theme);
    Motion &mo = g_motion;
    if (!ui->open) {
        mo.was_open = false;
        if (recomp_runtime_ui_toast_visible(ui)) draw_toast(ui, th);
        return;
    }
    ImGuiIO &io = ImGui::GetIO();
    const ImVec2 display = io.DisplaySize;
    if (display.x <= 0.0f || display.y <= 0.0f) return;
    const float dt = std::clamp(io.DeltaTime, 0.0f, 0.1f);
    if (!mo.was_open) {   // fresh open: replay the reveal, snap the highlights
        mo = Motion{};
        mo.was_open = true;
    }
    if (!ui->in_section && ui->section_count)
        recomp_runtime_ui_enter_section(ui, ui->section_index);

    const Fonts f = fonts();
    // The game picture's rectangle (letterbox bars excluded) when the host
    // supplies it; the menu covers exactly that area.
    ImVec2 area0(0.0f, 0.0f), area1 = display;
    if (ui->vp_w > 0.0f && ui->vp_h > 0.0f) {
        area0 = ImVec2(ui->vp_x, ui->vp_y);
        area1 = ImVec2(ui->vp_x + ui->vp_w, ui->vp_y + ui->vp_h);
    }
    const float area_w = area1.x - area0.x, area_h = area1.y - area0.y;
    // Everything scales from a 1280x800 reference layout.
    const float s = std::clamp(std::min(area_h / 800.0f, area_w / 1280.0f), 0.42f, 3.0f);
    // Readable floors for small windows (a 3x window is about s = 0.56).
    const float fs_body = std::max(26.0f * s, 15.0f), fs_small = std::max(21.0f * s, 13.0f);
    const float fs_title = std::max(54.0f * s, 28.0f), fs_badge = std::max(18.0f * s, 12.0f);

    mo.reveal = approach(mo.reveal, 1.0f, 18.0f, dt);
    const float reveal = smoothstep(mo.reveal);
    const float alpha = reveal * ui->opacity;

    ImGui::GetBackgroundDrawList()->AddRectFilled(
        ImVec2(0.0f, 0.0f), display,
        IM_COL32(0, 0, 0, static_cast<int>(255.0f * ui->dim * reveal + 0.5f)));

    const float margin_x = std::max(16.0f, 40.0f * s), margin_y = std::max(12.0f, 30.0f * s);
    const float rise = (1.0f - reveal) * 26.0f * s;
    const ImVec2 w0(area0.x + margin_x, area0.y + margin_y + rise);
    const ImVec2 w1(area1.x - margin_x, area1.y - margin_y + rise);

    ImGui::SetNextWindowPos(w0, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(w1.x - w0.x, w1.y - w0.y), ImGuiCond_Always);
    push_runtime_style(th);
    // The host drives selection through recomp_runtime_ui_handle_input. ImGui's
    // own keyboard/gamepad navigation would move a second, hidden cursor over
    // the same rows. A text row being edited still needs the keyboard.
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                                   ImGuiWindowFlags_NoSavedSettings |
                                   ImGuiWindowFlags_NoScrollWithMouse |
                                   (ui->editing_text ? 0 : ImGuiWindowFlags_NoNavInputs);
    if (ImGui::Begin("##recomp-runtime-ui", nullptr, flags)) {
        ImDrawList *d = ImGui::GetWindowDrawList();
        const float radius = 18.0f * s;
        d->AddRectFilled(w0, w1, u32(th.panel, 0.95f * alpha), radius);
        d->AddRect(w0, w1, u32(th.border, alpha), radius, 0, std::max(1.0f, 2.0f * s));
        d->AddRectFilled(ImVec2(w0.x + 18.0f * s, w0.y + 10.0f * s),
                         ImVec2(w1.x - 18.0f * s, w0.y + 13.0f * s), u32(th.accent, alpha), 2.0f * s);

        // ---- header ---------------------------------------------------------
        const float pad = 34.0f * s;
        const char *title = "Paused";
        const ImVec2 tpos(w0.x + pad, w0.y + 26.0f * s);
        d->AddText(f.title, fs_title, ImVec2(tpos.x + 3.0f * s, tpos.y + 3.0f * s),
                   u32(th.background, alpha), title);
        d->AddText(f.title, fs_title, tpos, u32(th.accent, alpha), title);
        if (ui->config.title && ui->config.title[0]) {
            const float sw = text_w(f.body, fs_small, ui->config.title);
            d->AddText(f.body, fs_small, ImVec2(w1.x - pad - sw, w0.y + 50.0f * s),
                       u32(th.text_muted, alpha), ui->config.title);
        }

        // ---- tabs -------------------------------------------------------------
        const float tab_y = w0.y + 112.0f * s;
        float x = w0.x + pad;
        float sel_x = x, sel_w = 0.0f;
        for (size_t i = 0; i < ui->section_count; ++i) {
            char label[96];
            upper(ui->sections[i], label, sizeof label);
            const bool sel = i == ui->section_index;
            ImFont *font = sel ? f.bold : f.body;
            const float tw = text_w(font, fs_body, label);
            ImGui::SetCursorScreenPos(ImVec2(x - 14.0f * s, tab_y - 8.0f * s));
            ImGui::PushID(static_cast<int>(i));
            if (ImGui::InvisibleButton("##tab", ImVec2(tw + 28.0f * s, fs_body + 18.0f * s)))
                recomp_runtime_ui_enter_section(ui, i);
            const bool hot = ImGui::IsItemHovered();
            ImGui::PopID();
            if (sel || hot)
                d->AddRectFilled(ImVec2(x - 14.0f * s, tab_y - 8.0f * s),
                                 ImVec2(x + tw + 14.0f * s, tab_y + fs_body + 12.0f * s),
                                 u32(th.panel_hovered, alpha * (sel ? 1.0f : 0.6f)), 10.0f * s);
            d->AddText(font, fs_body, ImVec2(x, tab_y),
                       u32(sel ? th.text : th.text_muted, alpha), label);
            if (sel) { sel_x = x; sel_w = tw; }
            x += tw + 46.0f * s;
        }
        if (!mo.tab_init || mo.last_section != ui->section_index) {
            if (!mo.tab_init) { mo.tab_x = sel_x; mo.tab_w = sel_w; mo.tab_init = true; }
            if (mo.last_section != ui->section_index) mo.row_init = false;
            mo.last_section = ui->section_index;
        }
        mo.tab_x = approach(mo.tab_x, sel_x, 18.0f, dt);
        mo.tab_w = approach(mo.tab_w, sel_w, 18.0f, dt);
        d->AddRectFilled(ImVec2(mo.tab_x - 4.0f * s, tab_y + fs_body + 15.0f * s),
                         ImVec2(mo.tab_x + mo.tab_w + 4.0f * s, tab_y + fs_body + 20.0f * s),
                         u32(th.accent, alpha), 3.0f * s);
        const float rule_y = tab_y + fs_body + 30.0f * s;
        d->AddLine(ImVec2(w0.x + 24.0f * s, rule_y), ImVec2(w1.x - 24.0f * s, rule_y),
                   u32(th.border, alpha), std::max(1.0f, 2.0f * s));

        // ---- footer (laid out first so rows know their floor) ----------------
        const float badge_h = 32.0f * s;
        const float foot_y = w1.y - 58.0f * s;
        d->AddLine(ImVec2(w0.x + 24.0f * s, foot_y - 14.0f * s), ImVec2(w1.x - 24.0f * s, foot_y - 14.0f * s),
                   u32(th.border, alpha), std::max(1.0f, 2.0f * s));
        {
            float bx = w0.x + 40.0f * s;
            bx = badge(d, f, th, bx, foot_y, badge_h,
                       ui->config.accept_label && ui->config.accept_label[0] ? ui->config.accept_label : "A",
                       "Select", fs_badge);
            bx = badge(d, f, th, bx, foot_y, badge_h,
                       ui->config.back_label && ui->config.back_label[0] ? ui->config.back_label : "B",
                       "Back", fs_badge);
            bx = badge(d, f, th, bx, foot_y, badge_h, "LB", nullptr, fs_badge);
            bx = badge(d, f, th, bx, foot_y, badge_h, "RB", "Tabs", fs_badge);
            bx = badge(d, f, th, bx, foot_y, badge_h, "Esc", "Resume", fs_badge);
            if (ui->status_frames && ui->status[0]) {
                const float sw = text_w(f.bold, fs_small, ui->status);
                d->AddText(f.bold, fs_small, ImVec2(w1.x - 40.0f * s - sw, foot_y + (badge_h - fs_small) * 0.5f),
                           u32(th.warn, alpha), ui->status);
                recomp_runtime_ui_advance_status(ui, dt * 60.0f);
            }
            // Click target for Resume, over the Esc badge area.
            ImGui::SetCursorScreenPos(ImVec2(bx - 140.0f * s, foot_y));
            if (ImGui::InvisibleButton("##resume", ImVec2(140.0f * s, badge_h)))
                recomp_runtime_ui_close(ui);
        }

        // ---- rows -------------------------------------------------------------
        const float rows_x0 = w0.x + 30.0f * s;
        const float rows_x1 = w0.x + (w1.x - w0.x) * 0.56f;
        const float top = rule_y + 22.0f * s;
        const float floor_y = foot_y - 28.0f * s;
        const float row_h = 54.0f * s;
        float y = top;
        char page_title[256];
        recomp_runtime_ui_page_title(ui, page_title, sizeof page_title);
        if (ui->page[0]) {
            char head[256];
            std::snprintf(head, sizeof head, "\xE2\x97\x80  %s", page_title);   // "◀  page"
            ImGui::SetCursorScreenPos(ImVec2(rows_x0, y));
            if (ImGui::InvisibleButton("##up", ImVec2(rows_x1 - rows_x0, fs_small * 1.4f)))
                recomp_runtime_ui_leave_folder(ui);
            char up[256];
            upper(head, up, sizeof up);
            d->AddText(f.bold, fs_small, ImVec2(rows_x0 + 6.0f * s, y), u32(th.accent2, alpha), up);
            y += fs_small * 1.7f;
        }
        const size_t count = recomp_runtime_ui_page_entry_count(ui);
        // Keep the selected row on screen: scroll whole rows.
        const int visible = std::max(1, static_cast<int>((floor_y - y) / row_h));
        size_t first = 0;
        if (ui->row_index >= static_cast<size_t>(visible)) first = ui->row_index - visible + 1;
        const RecompRuntimeUiItem *focused = nullptr;
        char focused_folder[96] = {};
        float target_row_y = -1.0f;
        // Channel 1: rows. Channel 0: the eased focus highlight underneath them.
        d->ChannelsSplit(2);
        d->ChannelsSetCurrent(1);
        for (size_t index = first; index < count && y + row_h <= floor_y + 1.0f; ++index) {
            char folder[96];
            const RecompRuntimeUiItem *item = recomp_runtime_ui_page_entry(ui, index, folder, sizeof folder);
            if (!item && !folder[0]) continue;
            const bool selected = ui->in_section && index == ui->row_index;
            const bool enabled = !item || recomp_runtime_ui_item_enabled(ui, item) != 0;
            ImGui::PushID(static_cast<int>(index));
            ImGui::SetCursorScreenPos(ImVec2(rows_x0, y));
            const float cy = y + (row_h - 6.0f * s) * 0.5f;
            const bool stepped = item && (item->type == RECOMP_RUNTIME_UI_BOOL ||
                                          item->type == RECOMP_RUNTIME_UI_CHOICE ||
                                          item->type == RECOMP_RUNTIME_UI_INT);
            const bool clicked = ImGui::InvisibleButton("##row", ImVec2(rows_x1 - rows_x0, row_h - 6.0f * s));
            const bool hovered = ImGui::IsItemHovered();
            if (hovered) { ui->row_index = index; ui->in_section = 1; }
            if (selected) {
                target_row_y = y;
                focused = item;
                if (!item) std::snprintf(focused_folder, sizeof focused_folder, "%s", folder);
            }
            bool opened_folder = false;
            if (clicked) {
                ui->row_index = index;
                if (!item) {
                    opened_folder = recomp_runtime_ui_enter_folder(ui, index) != 0;
                } else {
                    // Clicking just left of the value steps back; elsewhere forward.
                    const float mx = ImGui::GetIO().MousePos.x;
                    const int dir = stepped && mx < rows_x1 - (rows_x1 - rows_x0) * 0.33f &&
                                    mx > rows_x0 + (rows_x1 - rows_x0) * 0.5f ? -1 : 1;
                    recomp_runtime_ui_adjust_current(ui, dir, item->type != RECOMP_RUNTIME_UI_INT, 0);
                }
            }
            if (opened_folder) { ImGui::PopID(); break; }   // the new page draws next frame
            char label[160];
            upper(item ? (item->label ? item->label : "") : folder, label, sizeof label);
            const float la = enabled ? 1.0f : 0.45f;
            d->AddText(selected ? f.bold : f.body, fs_body, ImVec2(rows_x0 + 20.0f * s, cy - fs_body * 0.5f),
                       u32(th.text, alpha * la), label);
            char value[128] = {};
            if (item) value_text(ui, item, value, sizeof value);
            else std::snprintf(value, sizeof value, "\xE2\x96\xB6");   // "▶"
            if (item && item->type == RECOMP_RUNTIME_UI_TEXT && ui->editing_text && selected) {
                const float field_w = (rows_x1 - rows_x0) * 0.42f;
                ImGui::SetCursorScreenPos(ImVec2(rows_x1 - 20.0f * s - field_w, cy - ImGui::GetFrameHeight() * 0.5f));
                ImGui::SetNextItemWidth(field_w);
                if (!ImGui::IsAnyItemActive()) ImGui::SetKeyboardFocusHere();
                const bool submitted = ImGui::InputText("##edit", ui->edit_buffer, sizeof(ui->edit_buffer),
                    ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
                if (submitted) { recomp_runtime_ui_commit_text(ui, item, ui->edit_buffer); ui->editing_text = 0; }
                else if (ImGui::IsKeyPressed(ImGuiKey_Escape)) ui->editing_text = 0;
                else if (!ImGui::IsItemActive() && ImGui::IsAnyItemActive()) ui->editing_text = 0;
            } else if (value[0]) {
                const float vw = text_w(f.body, fs_body, value);
                const float vx = rows_x1 - (stepped ? 40.0f : 22.0f) * s - vw;
                d->AddText(f.body, fs_body, ImVec2(vx, cy - fs_body * 0.5f),
                           u32(selected ? th.text : th.text_muted, alpha * la), value);
                if (stepped) {
                    const ImU32 ac = u32(selected ? th.accent : th.border, alpha * la);
                    arrow(d, vx - 22.0f * s, cy, 7.0f * s, true, ac);
                    arrow(d, rows_x1 - 22.0f * s, cy, 7.0f * s, false, ac);
                }
            }
            ImGui::PopID();
            y += row_h;
        }
        // Eased focus highlight, under the rows' text.
        d->ChannelsSetCurrent(0);
        if (target_row_y >= 0.0f) {
            if (!mo.row_init) { mo.row_y = target_row_y; mo.row_init = true; }
            mo.row_y = approach(mo.row_y, target_row_y, 22.0f, dt);
            const ImVec2 r0(rows_x0, mo.row_y), r1(rows_x1, mo.row_y + row_h - 6.0f * s);
            d->AddRectFilled(r0, r1, u32(mix(th.panel, th.accent, 0.16f), alpha), 10.0f * s);
            d->AddRect(r0, r1, u32(th.focus_ring, alpha), 10.0f * s, 0, std::max(1.5f, 3.0f * s));
        }
        d->ChannelsMerge();

        // ---- description pane -------------------------------------------------
        const ImVec2 p0(rows_x1 + 30.0f * s, top);
        const ImVec2 p1(w1.x - 30.0f * s, floor_y);
        d->AddRectFilled(p0, p1, u32(th.background, 0.92f * alpha), 14.0f * s);
        d->AddRect(p0, p1, u32(th.border, alpha), 14.0f * s, 0, std::max(1.0f, 2.0f * s));
        const float inner = 24.0f * s;
        char head[160] = {};
        const char *desc = nullptr;
        if (focused) {
            upper(focused->label ? focused->label : "", head, sizeof head);
            desc = focused->description;
        } else if (focused_folder[0]) {
            upper(focused_folder, head, sizeof head);
            desc = "Opens more settings.";
        }
        if (head[0]) {
            const float hy = wrapped(d, f.bold, fs_body, ImVec2(p0.x + inner, p0.y + inner),
                                     p1.x - p0.x - inner * 2.0f, u32(th.accent2, alpha), head);
            if (desc && desc[0])
                wrapped(d, f.body, fs_small, ImVec2(p0.x + inner, hy + 12.0f * s),
                        p1.x - p0.x - inner * 2.0f, u32(th.text_muted, alpha), desc);
            if (focused && !recomp_runtime_ui_item_enabled(ui, focused))
                d->AddText(f.bold, fs_small, ImVec2(p0.x + inner, p1.y - inner - fs_small),
                           u32(th.warn, alpha), "Not available right now.");
        }
    }
    ImGui::End();
    pop_runtime_style();
    // Over the menu too: a shortcut can still answer while it is open.
    if (recomp_runtime_ui_toast_visible(ui)) draw_toast(ui, th);
}
