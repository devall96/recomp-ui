#include "launcher_i18n.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unordered_map>
#include <unordered_set>
#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#ifndef RECOMP_UI_LANGUAGE
#define RECOMP_UI_LANGUAGE "en"
#endif

struct LauncherI18nEntry {
    const char* english;
    const char* localized;
};

static const LauncherI18nEntry kItalian[] = {
    {"Settings", "Impostazioni"},
    {"Mods", "Mod"},
    {"Assist Tools", "Strumenti"},
    {"Credits", "Crediti"},
    {"< Back", "< Indietro"},
    {"Set player name", "Imposta nome giocatore"},
    {"Install", "Installa"},
    {"Install Mod Package", "Installa pacchetto mod"},
    {"Package installed. Changes apply when you press PLAY.",
     "Pacchetto installato. Le modifiche si applicano premendo GIOCA."},
    {"Disable all", "Disabilita tutto"},
    {"Disable every installed mod feature", "Disabilita tutte le funzioni mod installate"},
    {"Search features, groups, packages...", "Cerca funzioni, gruppi, pacchetti..."},
    {"General", "Generale"},
    {"No matching features.", "Nessuna funzione corrispondente."},
    {"No mod features installed.", "Nessuna funzione mod installata."},
    {"Disable", "Disabilita"},
    {"Enable", "Abilita"},
    {"From", "Da"},
    {"Source: ", "Origine: "},
    {"Project page", "Pagina progetto"},
    {"3D camera controls", "Controlli camera 3D"},
    {"This mod adds live camera input. Review the current right-stick and keyboard bindings before playing.",
     "Questa mod aggiunge input camera in tempo reale. Controlla i binding dello stick destro e della tastiera prima di giocare."},
    {"Enable this feature to expose its camera bindings on the Controller page.",
     "Abilita questa funzione per mostrare i binding camera nella pagina Controller."},
    {"Review Camera Bindings", "Controlla binding camera"},
    {"Files", "File"},
    {"(optional)", "(facoltativo)"},
    {"Not selected", "Non selezionato"},
    {"Change folder", "Cambia cartella"},
    {"Change file", "Cambia file"},
    {"Select folder", "Seleziona cartella"},
    {"Select file", "Seleziona file"},
    {"Configuration", "Configurazione"},
    {"Install or select a feature to configure it.",
     "Installa o seleziona una funzione da configurare."},
    {"Skip launcher on boot", "Salta launcher all'avvio"},
    {"Restore Defaults", "Ripristina predefiniti"},
    {"Host Lobby", "Ospita lobby"},
    {"Network Settings", "Impostazioni rete"},
    {"Refresh", "Aggiorna"},
    {"Join Direct", "Connessione diretta"},
    {"Automatch", "Partita rapida"},
    {"Queued", "In coda"},
    {"Actions##np_footer", "Azioni##np_footer"},
    {"Browse For", "Sfoglia"},
    {"Browse", "Sfoglia"},
    {"New", "Nuova"},
    {"Configure", "Configura"},
    {"Enabled", "Abilitata"},
    {"connected", "connessa"},
    {"not assigned", "non assegnata"},
    {"PLAY", "GIOCA"},
    {"Disc", "disco"},
    {"ROM", "ROM"},
    {"Memory Card", "Memory card"},
    {"Serial", "Seriale"},
    {"Region", "Regione"},
    {"ISO header", "Header ISO"},
    {"File", "File"},
    {"Tracks", "Tracce"},
    {"OK", "OK"},
    {"Mismatch", "Non corrisponde"},
    {"Disc Selection", "Selezione disco"},
    {"No disc selected", "Nessun disco selezionato"},
    {"Disc verified", "Disco verificato"},
    {"Disc verified (warnings)", "Disco verificato (avvisi)"},
    {"Disc verification failed", "Verifica disco fallita"},
    {"Disc not recognized", "Disco non riconosciuto"},
    {"Copy details", "Copia dettagli"},
    {"Copy this file's checksums and the required ROM", "Copia i checksum di questo file e la ROM richiesta"},
    {"Skip (Play Unpatched)", "Salta (gioca senza patch)"},
    {"Patch ROM", "Applica patch ROM"},
    {"Insert...", "Inserisci..."},
    {"Browse save...", "Sfoglia salvataggio..."},
    {"None", "Nessuno"},
    {"Keyboard + Mouse", "Tastiera + mouse"},
    {"Keyboard", "Tastiera"},
    {"Controller", "Controller"},
    {"(disconnected)", "(disconnesso)"},
    {"(no gamepad connected)", "(nessun gamepad connesso)"},
    {"PLAYER", "GIOCATORE"},
    {"NETPLAY", "NETPLAY"},
    {"CONTROLLER", "CONTROLLER"},
    {"CONTROLLER - PLAYER", "CONTROLLER - GIOCATORE"},
    {"Input source", "Sorgente input"},
    {"Rename Gamepad", "Rinomina gamepad"},
    {"Delete Gamepad", "Elimina gamepad"},
    {"Display name for this gamepad:", "Nome visualizzato per questo gamepad:"},
    {"Cancel", "Annulla"},
    {"Deadzone", "Zona morta"},
    {"DISPLAY", "VIDEO"},
    {"AUDIO", "AUDIO"},
    {"HOTKEYS", "TASTI RAPIDI"},
    {"SYSTEM", "SISTEMA"},
    {"INPUT", "INPUT"},
    {"ONLINE", "ONLINE"},
    {"CREDITS", "CREDITI"},
    {"ASSIST TOOLS / CHEATS", "STRUMENTI / CHEAT"},
    {"MOUSE", "MOUSE"},
    {"SAVES", "SALVATAGGI"},
    {"LOCALIZATION", "LOCALIZZAZIONE"},
    {"SOLAR SENSOR", "SENSORE SOLARE"},
    {"ROM PATCH", "PATCH ROM"},
    {"Window scale", "Scala finestra"},
    {"Window size", "Dimensione finestra"},
    {"Fullscreen", "Schermo intero"},
    {"Screen layout", "Layout schermo"},
    {"Integer scaling", "Scala intera"},
    {"Scaling filter", "Filtro scala"},
    {"Linear filtering", "Filtro lineare"},
    {"Texture filtering", "Filtro texture"},
    {"Affine background smoothing", "Smussatura fondali affine"},
    {"Shader", "Shader"},
    {"Custom", "Personalizzato"},
    {"Folder", "Cartella"},
    {"Renderer", "Renderer"},
    {"Supersampling", "Supersampling"},
    {"Internal resolution", "Risoluzione interna"},
    {"Native", "Nativa"},
    {"Match display", "Come lo schermo"},
    {"Antialiasing", "Antialiasing"},
    {"FMV filtering", "Filtro FMV"},
    {"Nearest", "Nearest"},
    {"Bilinear", "Bilineare"},
    {"Sharp", "Nitido"},
    {"Bicubic", "Bicubico"},
    {"Perspective textures", "Texture prospettiche"},
    {"Screen model", "Modello schermo"},
    {"Frame interpolation", "Interpolazione frame"},
    {"Presentation target", "Target presentazione"},
    {"VSync", "VSync"},
    {"Skip FMVs", "Salta FMV"},
    {"Rewind", "Riavvolgimento"},
    {"Rewind buffer", "Buffer riavvolgimento"},
    {"Rewind interval", "Intervallo riavvolgimento"},
    {"HD texture pack", "Pacchetto texture HD"},
    {"HD textures", "Texture HD"},
    {"Sample rate", "Frequenza campionamento"},
    {"Volume", "Volume"},
    {"Output device", "Dispositivo output"},
    {"Language", "Lingua"},
    {"(system default)", "(predefinito di sistema)"},
    {"High-quality SPU", "SPU alta qualita"},
    {"Enable MSU-1 music", "Abilita musica MSU-1"},
    {"BIOS", "BIOS"},
    {"Retail GBA BIOS required", "BIOS GBA retail richiesto"},
    {"Use OpenBIOS", "Usa OpenBIOS"},
    {"Clear", "Cancella"},
    {"Edit", "Modifica"},
    {"Edit##cc", "Modifica##cc"},
    {"Save", "Salva"},
    {"Save##cc", "Salva##cc"},
    {"Browse...", "Sfoglia..."},
    {"Cancel##cc", "Annulla##cc"},
    {"Optimize FMV Playback", "Ottimizza riproduzione FMV"},
    {"Features", "Funzioni"},
    {"Installed packages", "Pacchetti installati"},
    {"Player name", "Nome giocatore"},
    {"console default", "predefinito console"},
    {"Off", "Disattivato"},
    {"On", "Attivato"},
    {"Borderless", "Senza bordi"},
    {"Exclusive", "Esclusivo"},
    {"Default", "Predefinito"},
    {"Native", "Nativo"},
    {"Adaptive", "Adattivo"},
    {"Display refresh", "Frequenza schermo"},
    {"Software", "Software"},
    {"OpenGL", "OpenGL"},
    {"Reset", "Reset"},
    {"Pause", "Pausa"},
    {"Pause (dimmed)", "Pausa (scurita)"},
    {"Fast-forward", "Avanzamento rapido"},
    {"Window bigger", "Finestra piu grande"},
    {"Window smaller", "Finestra piu piccola"},
    {"Volume up", "Alza volume"},
    {"Volume down", "Abbassa volume"},
    {"FPS readout", "Indicatore FPS"},
    {"Toggle renderer", "Cambia renderer"},
    {"Save states menu", "Menu stati salvataggio"},
    {"Open launcher", "Apri launcher"},
    {"RESUME", "RIPRENDI"},
    {"QUIT GAME", "ESCI DAL GIOCO"},
    {"(unbound)", "(non assegnato)"},
    {"[ press... ]", "[ premi... ]"},
    {"Enable Assist Tools / Cheats", "Abilita strumenti / cheat"},
    {"FAST-FORWARD SPEED", "VELOCITA AVANZAMENTO"},
    {"Enable ROM patch", "Abilita patch ROM"},
    {"No patch selected", "Nessuna patch selezionata"},
};

// ---- run-time language -------------------------------------------------------
// A game may pick the UI language at run time (launcher_i18n_set_language);
// otherwise the compile-time RECOMP_UI_LANGUAGE applies. The five-language
// table (launcher_i18n_strings.inc) covers the launcher, the in-game menu and
// game-supplied rows; the older Italian pairs above remain a fallback.

struct LauncherI18nRow {
    const char* en;
    const char* fr;
    const char* de;
    const char* es;
    const char* it;
};

static const LauncherI18nRow kRows[] = {
#include "launcher_i18n_strings.inc"
};

static std::string g_language;   // empty = compile-time default

static int language_column(const char* code) {
    if (!code || !code[0]) return 0;
    if (std::strncmp(code, "fr", 2) == 0) return 1;
    if (std::strncmp(code, "de", 2) == 0) return 2;
    if (std::strncmp(code, "es", 2) == 0) return 3;
    if (std::strncmp(code, "it", 2) == 0) return 4;
    return 0;
}

static const char* row_text(const LauncherI18nRow& r, int column) {
    switch (column) {
    case 1: return r.fr;
    case 2: return r.de;
    case 3: return r.es;
    case 4: return r.it;
    default: return r.en;
    }
}

static const char* find_translation(const std::string& english, int column) {
    static std::unordered_map<std::string, const LauncherI18nRow*> rows;
    static std::unordered_map<std::string, const char*> italian;
    if (rows.empty()) {
        for (const auto& r : kRows) rows.emplace(r.en, &r);
        for (const auto& e : kItalian) italian.emplace(e.english, e.localized);
    }
    const auto it = rows.find(english);
    if (it != rows.end()) {
        const char* t = row_text(*it->second, column);
        if (t && t[0]) return t;
    }
    if (column == 4) {
        const auto old = italian.find(english);
        if (old != italian.end()) return old->second;
    }
    return nullptr;
}

static bool has_letter(const char* s) {
    for (; *s; ++s)
        if ((*s >= 'A' && *s <= 'Z') || (*s >= 'a' && *s <= 'z')) return true;
    return false;
}

// RECOMP_UI_I18N_MISSES=<path>: append each untranslated string once.
static void log_miss(const char* english, int column) {
    static const char* path = std::getenv("RECOMP_UI_I18N_MISSES");
    if (!path || !path[0] || !has_letter(english)) return;
    static std::unordered_set<std::string> seen;
    if (!seen.insert(std::to_string(column) + english).second) return;
    if (std::FILE* f = std::fopen(path, "a")) {
        std::fprintf(f, "%s\n", english);
        std::fclose(f);
    }
}

extern "C" void launcher_i18n_set_language(const char* code) {
    g_language = code ? code : "";
}

const char* launcher_i18n_language(void) {
    return g_language.empty() ? RECOMP_UI_LANGUAGE : g_language.c_str();
}

extern "C" const char* launcher_i18n_system_language(void) {
#if defined(_WIN32)
    switch (PRIMARYLANGID(GetUserDefaultUILanguage())) {
    case LANG_FRENCH: return "fr";
    case LANG_GERMAN: return "de";
    case LANG_SPANISH: return "es";
    case LANG_ITALIAN: return "it";
    default: return "en";
    }
#else
    for (const char* var : {"LC_ALL", "LC_MESSAGES", "LANG"}) {
        const char* v = std::getenv(var);
        if (!v || !v[0]) continue;
        const int column = language_column(v);
        static const char* const kCodes[] = {"en", "fr", "de", "es", "it"};
        return kCodes[column];
    }
    return "en";
#endif
}

const char* launcher_i18n_text(const char* english) {
    if (!english) return "";
    const int column = language_column(launcher_i18n_language());
    if (column == 0 || !english[0]) return english;
    static std::unordered_map<std::string, std::string> composed[5];
    const std::string key(english);
    if (const char* t = find_translation(key, column)) return t;
    // "Label##id": translate the visible part, keep the ImGui id.
    const auto hash = key.find("##");
    if (hash != std::string::npos && hash > 0) {
        if (const char* t = find_translation(key.substr(0, hash), column)) {
            auto& slot = composed[column][key];
            if (slot.empty()) slot = std::string(t) + key.substr(hash);
            return slot.c_str();
        }
    }
    log_miss(english, column);
    return english;
}
