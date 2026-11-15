// launcher_desktop_entry.h — "Add desktop shortcut" / "Add to Start menu" /
// "Add to applications menu" for a recomp game, per user, no installer.
//
// Windows: IShellLinkW .lnk files in the user's Desktop and Start menu
// Programs folders. Linux: an XDG .desktop file in
// $XDG_DATA_HOME/applications (menu) or the XDG desktop directory, plus the
// icon in hicolor/256x256/apps. Only files this module names are ever
// created or removed.
#ifndef LAUNCHER_DESKTOP_ENTRY_H
#define LAUNCHER_DESKTOP_ENTRY_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    LDE_DESKTOP = 0,   // desktop shortcut (Windows and Linux)
    LDE_MENU    = 1,   // Start menu (Windows) / applications menu (Linux)
} LdeKind;

typedef struct {
    const char* id;        // file-safe slug: "templefrog" (.desktop id, icon name)
    const char* name;      // display name: "TempleFrog"
    const char* comment;   // one-line description, may be NULL
    const char* icon_ico;  // Windows icon file (absolute), may be NULL
    const char* icon_png;  // Linux 256x256 PNG icon (absolute), may be NULL
    const char* workdir;   // working directory for the shortcut, may be NULL (exe dir)
} LdeApp;

// 1 when this platform supports the kind (and, on Linux, not under Flatpak).
int lde_supported(LdeKind kind);
// 1 when our entry of this kind currently exists.
int lde_exists(const LdeApp* app, LdeKind kind);
// Create (on != 0) or remove our entry. Returns 1 on success; on failure
// writes a short reason into err.
int lde_set(const LdeApp* app, LdeKind kind, int on, char* err, size_t err_len);
// Platform label for the menu kind: "Start menu" or "Applications menu".
const char* lde_menu_label(void);

#ifdef __cplusplus
}
#endif

#endif
