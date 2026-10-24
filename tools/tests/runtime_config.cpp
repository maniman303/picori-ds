// Exercise the production loader with user-editable numeric settings.
#include "port_runtime_config.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>
#include <fstream>
#include <nlohmann/json.hpp>

static void load(const char* path, const char* json) {
    FILE* f = fopen(path, "w");
    assert(f);
    fputs(json, f);
    assert(fclose(f) == 0);
    Port_Config_Load(path);
}

static void CheckAspectConfig(const char* path) {
    load(path, R"({"aspect_mode":"16:9","widescreen_enabled":false,"keep":17})");
    assert(Port_Config_AspectMode() == PORT_ASPECT_WIDESCREEN_16_9);
    assert(Port_Config_EffectiveAspectMode() == PORT_ASPECT_WIDESCREEN_16_9);
    Port_Config_PublishViewport();
    assert(Port_Viewport_Get()->canvas.width == 285);
    const uint32_t generation = Port_Viewport_Get()->generation;
    Port_Config_SetAspectMode(PORT_ASPECT_ULTRAWIDE_21_9);
    // Settings can change during presentation. Geometry stays fixed until the
    // explicit boundary used by the input/VBlank path publishes the next tick.
    assert(Port_Viewport_Get()->generation == generation);
    assert(Port_Viewport_Get()->canvas.width == 285);
    Port_Config_PublishViewport();
    assert(Port_Viewport_Get()->canvas.width == 374);
    Port_Config_SetConsoleParity(true);
    Port_Config_PublishViewport();
    assert(Port_Config_AspectMode() == PORT_ASPECT_ULTRAWIDE_21_9);
    assert(Port_Config_EffectiveAspectMode() == PORT_ASPECT_NATIVE_3_2);
    assert(Port_Viewport_Get()->canvas.width == 240);
    Port_Config_SetConsoleParity(false);
    Port_Config_PublishViewport();
    assert(Port_Viewport_Get()->canvas.width == 374);
    nlohmann::json saved;
    std::ifstream(path) >> saved;
    assert(!saved.contains("widescreen_enabled"));
    assert(saved["aspect_mode"] == "21:9" && saved["keep"] == 17);

    load(path, R"({"aspect_mode":"native","widescreen_enabled":true})");
    Port_Config_PublishViewport();
    assert(Port_Viewport_Get()->canvas.width == 240);
    assert(Port_Config_AspectModeAvailable(PORT_ASPECT_NATIVE_3_2));
    assert(Port_Config_AspectModeAvailable(PORT_ASPECT_WIDESCREEN_16_9));
    assert(Port_Config_AspectModeAvailable(PORT_ASPECT_ULTRAWIDE_21_9));
    assert(!Port_Config_AspectModeAvailable(PORT_ASPECT_DS_4_3));
    assert(!Port_Config_AspectModeAvailable(PORT_ASPECT_SUPER_ULTRAWIDE_32_9));
    Port_Config_CycleAspectMode(1);
    assert(Port_Config_AspectMode() == PORT_ASPECT_WIDESCREEN_16_9);
    Port_Config_CycleAspectMode(1);
    assert(Port_Config_AspectMode() == PORT_ASPECT_ULTRAWIDE_21_9);
    Port_Config_CycleAspectMode(1);
    assert(Port_Config_AspectMode() == PORT_ASPECT_NATIVE_3_2);
    Port_Config_CycleAspectMode(-1);
    assert(Port_Config_AspectMode() == PORT_ASPECT_ULTRAWIDE_21_9);

    load(path, R"({"aspect_mode":"WIDESCREEN"})");
    assert(Port_Config_AspectMode() == PORT_ASPECT_WIDESCREEN_16_9);
    load(path, R"({"aspect_mode":"ULTRAWIDE"})");
    assert(Port_Config_AspectMode() == PORT_ASPECT_ULTRAWIDE_21_9);
    load(path, R"({"aspect_mode":"super_ultrawide"})");
    assert(Port_Config_AspectMode() == PORT_ASPECT_SUPER_ULTRAWIDE_32_9);
    assert(Port_Config_EffectiveAspectMode() == PORT_ASPECT_NATIVE_3_2);
    load(path, R"({"aspect_mode":"4:3"})");
    assert(Port_Config_AspectMode() == PORT_ASPECT_DS_4_3);
    assert(Port_Config_EffectiveAspectMode() == PORT_ASPECT_NATIVE_3_2);
    Port_Config_SetAspectMode(PORT_ASPECT_DS_4_3);
    Port_Config_Load(path);
    assert(Port_Config_AspectMode() == PORT_ASPECT_DS_4_3);
    std::ifstream(path) >> saved;
    assert(saved["aspect_mode"] == "4:3");
    Port_Config_SetAspectMode((PortAspectMode)-1);
    assert(Port_Config_AspectMode() == PORT_ASPECT_NATIVE_3_2);
}

int main(int argc, char** argv) {
    assert(argc == 2);
    const char* path = argv[1];
    CheckAspectConfig(path);
    load(path, "{}");
    assert(Port_Config_BindingCount(PORT_INPUT_OPEN_SETTINGS) == 0);
    load(path, R"({"bindings":{"open_settings":["SDLK:0x0000006f"]}})");
    SDL_Event settingsPress = {};
    settingsPress.type = SDL_EVENT_KEY_DOWN;
    settingsPress.key.key = SDLK_O;
    assert(Port_Config_EventIsInputDown(&settingsPress, PORT_INPUT_OPEN_SETTINGS));
    Port_Config_Load(path);
    assert(Port_Config_EventIsInputDown(&settingsPress, PORT_INPUT_OPEN_SETTINGS));
    Port_Config_ResetAllBindings();
    assert(Port_Config_BindingCount(PORT_INPUT_OPEN_SETTINGS) == 0);
    load(path, "{}");
    assert(Port_Config_BindingCount(PORT_INPUT_QUIT_SAVE) == 0);
    load(path, R"({"bindings":{"quit_save":["SDLK:0x0000006b"]}})");
    SDL_Event quitPress = {};
    quitPress.type = SDL_EVENT_KEY_DOWN;
    quitPress.key.key = SDLK_K;
    assert(Port_Config_EventIsInputDown(&quitPress, PORT_INPUT_QUIT_SAVE));
    quitPress.key.repeat = true;
    assert(!Port_Config_EventIsInputDown(&quitPress, PORT_INPUT_QUIT_SAVE));
    Port_Config_ClearBindings(PORT_INPUT_QUIT_SAVE);
    Port_Config_Load(path);
    assert(Port_Config_BindingCount(PORT_INPUT_QUIT_SAVE) == 0);
    Port_Config_ResetAllBindings();
    assert(Port_Config_BindingCount(PORT_INPUT_QUIT_SAVE) == 0);
    load(path, "{}");
    assert(!Port_Config_GetHdDialogue());
    assert(Port_Config_GetMenuArtBackground());
    load(path, R"({"hd_dialogue":true,"menu_art_background":true,"console_parity":true})");
    assert(!Port_Config_GetHdDialogue());
    assert(!Port_Config_GetMenuArtBackground());
    Port_Config_SetConsoleParity(false);
    assert(Port_Config_GetHdDialogue());
    assert(Port_Config_GetMenuArtBackground());
    // Layout affects physical controller bindings, including custom ones, but
    // leaves keyboard inputs alone and survives a save/load cycle.
    load(path, "{}");
    assert(Port_Config_GetNintendoLayout());
    load(path, R"({"nintendo_layout":false})");
    assert(!Port_Config_GetNintendoLayout());
    SDL_Event press = {};
    press.type = SDL_EVENT_GAMEPAD_BUTTON_DOWN;
    press.gbutton.button = SDL_GAMEPAD_BUTTON_SOUTH;
    assert(Port_Config_EventIsInputDown(&press, PORT_INPUT_A));
    assert(!Port_Config_EventIsInputDown(&press, PORT_INPUT_B));
    Port_Config_SetNintendoLayout(true);
    Port_Config_Load(path);
    assert(Port_Config_GetNintendoLayout());
    const SDL_GamepadButton physical[] = {
        SDL_GAMEPAD_BUTTON_EAST, SDL_GAMEPAD_BUTTON_SOUTH,
        SDL_GAMEPAD_BUTTON_NORTH, SDL_GAMEPAD_BUTTON_WEST,
    };
    const PortInput actions[] = {PORT_INPUT_A, PORT_INPUT_B, PORT_INPUT_SOFT_X, PORT_INPUT_SOFT_Y};
    for (int i = 0; i < 4; ++i) {
        press.gbutton.button = physical[i];
        for (int j = 0; j < 4; ++j)
            assert(Port_Config_EventIsInputDown(&press, actions[j]) == (i == j));
    }
    load(path, R"({"nintendo_layout":true,"bindings":{"r":["SDL_GAMEPAD:0x00000002"]}})");
    press.gbutton.button = SDL_GAMEPAD_BUTTON_NORTH;
    assert(Port_Config_EventIsInputDown(&press, PORT_INPUT_R));
    press.gbutton.button = SDL_GAMEPAD_BUTTON_WEST;
    assert(!Port_Config_EventIsInputDown(&press, PORT_INPUT_R));
    assert(Port_Config_MapGamepadButton(SDL_GAMEPAD_BUTTON_LEFT_SHOULDER) == SDL_GAMEPAD_BUTTON_LEFT_SHOULDER);
    press = {};
    press.type = SDL_EVENT_KEY_DOWN;
    press.key.key = SDLK_X;
    assert(Port_Config_EventIsInputDown(&press, PORT_INPUT_A));
    Port_Config_SetNintendoLayout(false);
    press.type = SDL_EVENT_GAMEPAD_BUTTON_DOWN;
    press.gbutton.button = SDL_GAMEPAD_BUTTON_WEST;
    assert(Port_Config_EventIsInputDown(&press, PORT_INPUT_R));
    load(path, R"({"window_scale":1e100,"frame_time_ns":-1,"practice_slowmo":1e-30})");
    assert(Port_Config_WindowScale() == 3);
    assert(Port_Config_FrameTimeNs() == 1000000000ULL / 60);
    assert(Port_Config_GetPracticeSlowmo() == 0.05f);
    load(path, R"({"window_scale":4294967297,"practice_slowmo":1e100,"bg_fill_color":[1e100,-1e100,127.5]})");
    assert(Port_Config_WindowScale() == 3);
    assert(Port_Config_GetPracticeSlowmo() == 1.0f);
    u8 r, g, b;
    Port_Config_BgFillColor(&r, &g, &b);
    assert(r == 255 && g == 0 && b == 127);
    load(path, R"({"window_scale":4,"frame_time_ns":0,"practice_slowmo":0.5,"master_volume":"bad"})");
    assert(Port_Config_WindowScale() == 4);
    assert(Port_Config_FrameTimeNs() == 0);
    assert(Port_Config_GetPracticeSlowmo() == 0.5f);
    assert(Port_Config_GetMasterVolume() == 1.0f);
    load(path, R"({"reborn_features":4294967297})");
    assert(Port_Config_GetRebornMask() == 0);
    load(path, R"({"reborn_features":-1})");
    assert(Port_Config_GetRebornMask() == 0);
    for (const char* json : {R"({"practice_slowmo":0})", R"({"practice_slowmo":-0.001})"}) {
        load(path, json);
        assert(Port_Config_GetPracticeSlowmo() == 0.05f);
    }
    Port_Config_SetPracticeSlowmo(0);
    assert(Port_Config_GetPracticeSlowmo() == 0.05f);
    Port_Config_SetPracticeSlowmo(2);
    assert(Port_Config_GetPracticeSlowmo() == 1.0f);
    puts("runtime config regression checks passed");
}
