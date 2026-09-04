#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <commdlg.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <vector>

#include "vmd_parser.h"
#include "mmd_player.h"
#include "bone_anim_player.h"
#include "bone_map.h"
#include "il2cpp_api.h"
#include "il2cpp_dump.h"
#include "muscle_player.h"
#include "camera_player.h"


#include "globals.h"
#include "il2cpp_trace.h"
#include "scene_dump.h"
#include "model_dump.h"
#include "eiem_config.h"
#include "update_check.h"

#include "audio.h"

#include "smc_face.h"

static int32_t UnboxInt(void *boxed) {
  __try {
    return boxed ? *(int32_t *)((char *)boxed + 16) : 0;
  } __except (1) {
    return 0;
  }
}
static bool UnboxBool(void *boxed) {
  __try {
    return boxed ? *(bool *)((char *)boxed + 16) : false;
  } __except (1) {
    return false;
  }
}

#include "animation.h"
#include "trojan.h"
#include "gui.h"
#include "init.h"

#define APPLEPIE_PLUGIN_IMPL
#include "applepie_mgr.h"

static AP_HotkeyInfo s_apHotkeys[] = {
    { "\xe5\x91\xbc\xe5\x87\xba/\xe9\x9a\x90\xe8\x97\x8f GUI \xe9\x9d\xa2\xe6\x9d\xbf", "gui_toggle_key", VK_INSERT },
    { "Reload EIEM mods", "mod_reload_key", VK_F10 },
};

static AP_PluginInfo s_apPluginInfo = {
    APPLEPIE_PLUGIN_API_VERSION,
    "eiem",
    "EIEM",
    "Endfield MMD",
    "eiem_config.txt",
    true 
};

APPLEPIE_PLUGIN_EXPORT AP_PluginInfo* AP_GetPluginInfo() {
  return &s_apPluginInfo;
}

APPLEPIE_PLUGIN_EXPORT bool AP_PluginEnable() {
  g_pluginActive = true;
  Log("[AP] Plugin enabled by manager");
  return true;
}

APPLEPIE_PLUGIN_EXPORT bool AP_PluginDisable() {
  g_pluginActive = false;
  if (g_guiVisible) ToggleGui();
  Log("[AP] Plugin disabled by manager");
  return true;
}

APPLEPIE_PLUGIN_EXPORT bool AP_ReloadConfig() {
  LoadEiemConfig();
  EiemRequestModUpdate(EiemModUpdate::Reload, "ApplePie config reload");
  s_apHotkeys[0].currentVK = g_guiToggleVK;
  s_apHotkeys[1].currentVK = g_modReloadVK;
  Log("[AP] Config reloaded: gui_toggle_key=%d (%s)", g_guiToggleVK,
      EiemVKToString(g_guiToggleVK));
  return true;
}

APPLEPIE_PLUGIN_EXPORT int AP_GetHotkeys(AP_HotkeyInfo* outArray, int maxCount) {
  s_apHotkeys[0].currentVK = g_guiToggleVK;
  s_apHotkeys[1].currentVK = g_modReloadVK;
  int count = sizeof(s_apHotkeys) / sizeof(s_apHotkeys[0]);
  if (count > maxCount) count = maxCount;
  for (int i = 0; i < count; i++) outArray[i] = s_apHotkeys[i];
  return count;
}

APPLEPIE_PLUGIN_EXPORT void AP_SetLanguage(const char* langCode) {
  Log("[AP] Language set by manager: %s", langCode ? langCode : "null");
}

static HANDLE g_initThread = nullptr;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID reserved) {
  if (reason == DLL_PROCESS_ATTACH) {
    DisableThreadLibraryCalls(hModule);
    g_initThread = CreateThread(NULL, 0, InitThread, NULL, 0, NULL);
  }
  return TRUE;
}
