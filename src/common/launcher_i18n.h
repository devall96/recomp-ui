#pragma once

const char* launcher_i18n_language(void);
const char* launcher_i18n_text(const char* english);
#ifdef __cplusplus
extern "C" {
#endif
// Run-time UI language ("en", "fr", "de", "es", "it"; empty = build default).
void launcher_i18n_set_language(const char* code);
// The OS UI language mapped to one of the codes above ("en" otherwise).
const char* launcher_i18n_system_language(void);
#ifdef __cplusplus
}
#endif
