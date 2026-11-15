// launcher_desktop_entry.c — see launcher_desktop_entry.h.
#include "launcher_desktop_entry.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void lde_err(char* err, size_t n, const char* msg) {
    if (err && n) snprintf(err, n, "%s", msg);
}

static int lde_valid(const LdeApp* app) {
    const char* p;
    if (!app || !app->id || !app->id[0] || !app->name || !app->name[0]) return 0;
    for (p = app->id; *p; ++p)   // the id becomes a file name: keep it boring
        if (!((*p >= 'a' && *p <= 'z') || (*p >= '0' && *p <= '9') || *p == '-' || *p == '_' || *p == '.'))
            return 0;
    for (p = app->name; *p; ++p)
        if (strchr("\\/:*?\"<>|", *p)) return 0;
    return 1;
}

#if defined(_WIN32)

#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <objbase.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <knownfolders.h>

const char* lde_menu_label(void) { return "Start menu"; }

int lde_supported(LdeKind kind) { (void)kind; return 1; }

static int lde_utf8_to_w(const char* s, wchar_t* out, int n) {
    return s && MultiByteToWideChar(CP_UTF8, 0, s, -1, out, n) > 0;
}

static int lde_link_path(const LdeApp* app, LdeKind kind, wchar_t* out, size_t n) {
    PWSTR folder = NULL;
    wchar_t name[256];
    const KNOWNFOLDERID* id = kind == LDE_DESKTOP ? &FOLDERID_Desktop : &FOLDERID_Programs;
    if (!lde_valid(app) || !lde_utf8_to_w(app->name, name, 256)) return 0;
    if (FAILED(SHGetKnownFolderPath(id, 0, NULL, &folder))) return 0;
    _snwprintf(out, n, L"%ls\\%ls.lnk", folder, name);
    out[n - 1] = 0;
    CoTaskMemFree(folder);
    return 1;
}

int lde_exists(const LdeApp* app, LdeKind kind) {
    wchar_t path[MAX_PATH * 2];
    if (!lde_link_path(app, kind, path, MAX_PATH * 2)) return 0;
    return GetFileAttributesW(path) != INVALID_FILE_ATTRIBUTES;
}

int lde_set(const LdeApp* app, LdeKind kind, int on, char* err, size_t err_len) {
    wchar_t path[MAX_PATH * 2], exe[MAX_PATH * 2], dir[MAX_PATH * 2], tmp[1024];
    HRESULT init, hr;
    IShellLinkW* link = NULL;
    IPersistFile* file = NULL;
    if (!lde_link_path(app, kind, path, MAX_PATH * 2)) { lde_err(err, err_len, "Could not find the folder."); return 0; }
    if (!on) {
        if (GetFileAttributesW(path) == INVALID_FILE_ATTRIBUTES || DeleteFileW(path)) return 1;
        lde_err(err, err_len, "Could not remove the shortcut.");
        return 0;
    }
    if (!GetModuleFileNameW(NULL, exe, MAX_PATH * 2)) { lde_err(err, err_len, "Could not find the game's program file."); return 0; }
    if (app->workdir && app->workdir[0]) lde_utf8_to_w(app->workdir, dir, MAX_PATH * 2);
    else {
        wchar_t* slash;
        wcsncpy(dir, exe, MAX_PATH * 2);
        dir[MAX_PATH * 2 - 1] = 0;
        slash = wcsrchr(dir, L'\\');
        if (slash) *slash = 0;
    }
    init = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    hr = CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, &IID_IShellLinkW, (void**)&link);
    if (SUCCEEDED(hr)) {
        IShellLinkW_SetPath(link, exe);
        IShellLinkW_SetWorkingDirectory(link, dir);
        if (app->comment && lde_utf8_to_w(app->comment, tmp, 1024)) IShellLinkW_SetDescription(link, tmp);
        if (app->icon_ico && app->icon_ico[0] && lde_utf8_to_w(app->icon_ico, tmp, 1024))
            IShellLinkW_SetIconLocation(link, tmp, 0);
        else
            IShellLinkW_SetIconLocation(link, exe, 0);
        hr = IShellLinkW_QueryInterface(link, &IID_IPersistFile, (void**)&file);
        if (SUCCEEDED(hr)) {
            hr = IPersistFile_Save(file, path, TRUE);
            IPersistFile_Release(file);
        }
        IShellLinkW_Release(link);
    }
    if (SUCCEEDED(init)) CoUninitialize();
    if (FAILED(hr)) { lde_err(err, err_len, "Windows could not create the shortcut."); return 0; }
    if (kind == LDE_MENU) SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_PATHW, path, NULL);
    return 1;
}

#elif defined(__linux__) && !defined(__ANDROID__)

#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>

const char* lde_menu_label(void) { return "Applications menu"; }

static int lde_flatpak(void) { return getenv("FLATPAK_ID") != NULL; }

int lde_supported(LdeKind kind) { (void)kind; return !lde_flatpak(); }

static int lde_mkdirs(const char* path) {
    char tmp[1024];
    char* p;
    snprintf(tmp, sizeof tmp, "%s", path);
    for (p = tmp + 1; *p; ++p)
        if (*p == '/') { *p = 0; if (mkdir(tmp, 0755) && errno != EEXIST) return 0; *p = '/'; }
    return mkdir(tmp, 0755) == 0 || errno == EEXIST;
}

static int lde_data_home(char* out, size_t n) {
    const char* x = getenv("XDG_DATA_HOME");
    const char* home = getenv("HOME");
    if (x && x[0] == '/') { snprintf(out, n, "%s", x); return 1; }
    if (!home || !home[0]) return 0;
    snprintf(out, n, "%s/.local/share", home);
    return 1;
}

// XDG desktop directory: $XDG_CONFIG_HOME/user-dirs.dirs XDG_DESKTOP_DIR, else ~/Desktop.
static int lde_desktop_dir(char* out, size_t n) {
    const char* home = getenv("HOME");
    const char* cfg = getenv("XDG_CONFIG_HOME");
    char path[1024], line[1024];
    FILE* f;
    if (!home || !home[0]) return 0;
    snprintf(out, n, "%s/Desktop", home);
    if (cfg && cfg[0] == '/') snprintf(path, sizeof path, "%s/user-dirs.dirs", cfg);
    else snprintf(path, sizeof path, "%s/.config/user-dirs.dirs", home);
    f = fopen(path, "r");
    if (!f) return 1;
    while (fgets(line, sizeof line, f)) {
        char* v;
        char* end;
        if (strncmp(line, "XDG_DESKTOP_DIR=\"", 17) != 0) continue;
        v = line + 17;
        end = strchr(v, '"');
        if (!end) break;
        *end = 0;
        if (strncmp(v, "$HOME", 5) == 0) snprintf(out, n, "%s%s", home, v + 5);
        else if (v[0] == '/') snprintf(out, n, "%s", v);
        break;
    }
    fclose(f);
    return 1;
}

static int lde_entry_path(const LdeApp* app, LdeKind kind, char* out, size_t n) {
    char base[1024];
    if (!lde_valid(app)) return 0;
    if (kind == LDE_MENU) {
        if (!lde_data_home(base, sizeof base)) return 0;
        snprintf(out, n, "%s/applications/%s.desktop", base, app->id);
    } else {
        if (!lde_desktop_dir(base, sizeof base)) return 0;
        snprintf(out, n, "%s/%s.desktop", base, app->id);
    }
    return 1;
}

int lde_exists(const LdeApp* app, LdeKind kind) {
    char path[1200];
    return lde_entry_path(app, kind, path, sizeof path) && access(path, F_OK) == 0;
}

static int lde_copy(const char* from, const char* to) {
    char buf[65536];
    size_t got;
    FILE* in = fopen(from, "rb");
    FILE* out;
    if (!in) return 0;
    out = fopen(to, "wb");
    if (!out) { fclose(in); return 0; }
    while ((got = fread(buf, 1, sizeof buf, in)) > 0) fwrite(buf, 1, got, out);
    fclose(in);
    return fclose(out) == 0;
}

// Desktop Entry Exec quoting: wrap in quotes, escape \ " ` $.
static void lde_exec_quote(const char* s, char* out, size_t n) {
    size_t o = 0;
    if (n < 3) { if (n) out[0] = 0; return; }
    out[o++] = '"';
    for (; *s && o + 3 < n; ++s) {
        if (strchr("\\\"`$", *s)) out[o++] = '\\';
        out[o++] = *s;
    }
    out[o++] = '"';
    out[o] = 0;
}

int lde_set(const LdeApp* app, LdeKind kind, int on, char* err, size_t err_len) {
    char path[1200], tmp[1300], dir[1024], exe[1024], quoted[2100], icon_name[300];
    const char* appimage = getenv("APPIMAGE");
    FILE* f;
    ssize_t len;
    if (!lde_supported(kind)) { lde_err(err, err_len, "Not available in this package."); return 0; }
    if (!lde_entry_path(app, kind, path, sizeof path)) { lde_err(err, err_len, "Could not find the folder."); return 0; }
    if (!on) {
        if (access(path, F_OK) != 0 || unlink(path) == 0) return 1;
        lde_err(err, err_len, "Could not remove the entry.");
        return 0;
    }
    if (appimage && appimage[0] == '/') snprintf(exe, sizeof exe, "%s", appimage);
    else {
        len = readlink("/proc/self/exe", exe, sizeof exe - 1);
        if (len <= 0) { lde_err(err, err_len, "Could not find the game's program file."); return 0; }
        exe[len] = 0;
    }
    if (app->workdir && app->workdir[0]) snprintf(dir, sizeof dir, "%s", app->workdir);
    else {
        char* slash;
        snprintf(dir, sizeof dir, "%s", exe);
        slash = strrchr(dir, '/');
        if (slash) *slash = 0;
    }
    snprintf(icon_name, sizeof icon_name, "%s", app->id);
    if (app->icon_png && app->icon_png[0]) {
        char data[1024], icon_dir[1200], icon_path[1400];
        if (lde_data_home(data, sizeof data)) {
            snprintf(icon_dir, sizeof icon_dir, "%s/icons/hicolor/256x256/apps", data);
            snprintf(icon_path, sizeof icon_path, "%s/%s.png", icon_dir, app->id);
            if (!lde_mkdirs(icon_dir) || !lde_copy(app->icon_png, icon_path))
                snprintf(icon_name, sizeof icon_name, "%s", app->icon_png);  // fall back to the absolute file
        }
    }
    {
        char parent[1200];
        char* slash;
        snprintf(parent, sizeof parent, "%s", path);
        slash = strrchr(parent, '/');
        if (slash) { *slash = 0; lde_mkdirs(parent); }
    }
    lde_exec_quote(exe, quoted, sizeof quoted);
    snprintf(tmp, sizeof tmp, "%s.tmp", path);
    f = fopen(tmp, "w");
    if (!f) { lde_err(err, err_len, "Could not write the entry."); return 0; }
    fprintf(f, "[Desktop Entry]\nType=Application\nVersion=1.0\nName=%s\n", app->name);
    if (app->comment && app->comment[0]) fprintf(f, "Comment=%s\n", app->comment);
    fprintf(f, "Exec=%s\nPath=%s\nIcon=%s\nTerminal=false\nCategories=Game;\nStartupNotify=true\n",
            quoted, dir, icon_name);
    if (fclose(f) != 0 || rename(tmp, path) != 0) { unlink(tmp); lde_err(err, err_len, "Could not write the entry."); return 0; }
    chmod(path, 0755);   // desktop files must be executable to launch from a file manager
    return 1;
}

#else

const char* lde_menu_label(void) { return "Applications menu"; }
int lde_supported(LdeKind kind) { (void)kind; return 0; }
int lde_exists(const LdeApp* app, LdeKind kind) { (void)app; (void)kind; return 0; }
int lde_set(const LdeApp* app, LdeKind kind, int on, char* err, size_t err_len) {
    (void)app; (void)kind; (void)on;
    lde_err(err, err_len, "Not available on this platform.");
    return 0;
}

#endif
