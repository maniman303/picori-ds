#include "port_screen_background.h"
#include <assert.h>

int main(void) {
    Port_Background_Publish(PORT_BG_SCOPE_WORLD);
    assert(Port_Background_Published() == PORT_BACKGROUND_LIVE_WORLD);
    Port_Background_StageEvent(PORT_BG_EVENT_HEALTH, 1);
    Port_Background_StageEvent(PORT_BG_EVENT_PAUSE, 1);
    assert(Port_Background_Published() == PORT_BACKGROUND_LIVE_WORLD);
    Port_Background_Publish(PORT_BG_SCOPE_PAUSE);
    assert(Port_Background_Published() == PORT_BACKGROUND_MENU_TITLED_DAMAGED);
    Port_Background_StageEvent(PORT_BG_EVENT_PAUSE, 7);
    assert(Port_Background_Published() == PORT_BACKGROUND_MENU_TITLED_DAMAGED);
    Port_Background_Publish(PORT_BG_SCOPE_PAUSE);
    assert(Port_Background_Published() == PORT_BACKGROUND_KINSTONE_PIECES_DAMAGED);
    Port_Background_StageEvent(PORT_BG_EVENT_PAUSE, 8);
    Port_Background_Publish(PORT_BG_SCOPE_PAUSE);
    assert(Port_Background_Published() == PORT_BACKGROUND_SWORD_TECHNIQUES_DAMAGED);
    Port_Background_StageEvent(PORT_BG_EVENT_PAUSE, 10);
    Port_Background_Publish(PORT_BG_SCOPE_PAUSE);
    assert(Port_Background_Published() == PORT_BACKGROUND_MENU_DAMAGED);
    Port_Background_StageEvent(PORT_BG_EVENT_FUSION, 0);
    Port_Background_Publish(PORT_BG_SCOPE_FUSION);
    assert(Port_Background_Published() == PORT_BACKGROUND_KINSTONE_DAMAGED);
    Port_Background_StageEvent(PORT_BG_EVENT_TRAVEL, 0);
    Port_Background_Publish(PORT_BG_SCOPE_TRAVEL);
    assert(Port_Background_Published() == PORT_BACKGROUND_FAST_TRAVEL);
    Port_Background_StageEvent(PORT_BG_EVENT_HEALTH, 2);
    Port_Background_Publish(PORT_BG_SCOPE_TRAVEL);
    assert(Port_Background_Published() == PORT_BACKGROUND_FAST_TRAVEL);
    Port_Background_StageEvent(PORT_BG_EVENT_CLEAR, 0);
    Port_Background_Publish(PORT_BG_SCOPE_TRAVEL);
    assert(Port_Background_Published() == PORT_BACKGROUND_UNCLASSIFIED);
    Port_Background_StageEvent(PORT_BG_EVENT_LOGOS, 0);
    Port_Background_Publish(PORT_BG_SCOPE_STORY);
    assert(Port_Background_Published() == PORT_BACKGROUND_BOOT_LOGOS);
    Port_Background_StageEvent(PORT_BG_EVENT_TITLE, 0);
    Port_Background_Publish(PORT_BG_SCOPE_STORY);
    assert(Port_Background_Published() == PORT_BACKGROUND_TITLE);
    Port_Background_Reset();
    assert(Port_Background_Published() == PORT_BACKGROUND_UNCLASSIFIED);
    Port_Background_Publish(PORT_BG_SCOPE_STORY);
    assert(Port_Background_Published() == PORT_BACKGROUND_UNCLASSIFIED);
    Port_Background_Publish(PORT_BG_SCOPE_WORLD);
    Port_Background_StageEvent(PORT_BG_EVENT_PAUSE, 1);
    Port_Background_Publish(PORT_BG_SCOPE_PAUSE);
    assert(Port_Background_Published() == PORT_BACKGROUND_UNCLASSIFIED);
    Port_Background_StageEvent(PORT_BG_EVENT_PROLOGUE, 5);
    Port_Background_Publish(PORT_BG_SCOPE_STORY);
    assert(Port_Background_Published() == PORT_BACKGROUND_PROLOGUE_06);
    Port_Background_StageEvent(PORT_BG_EVENT_CREDITS, 8);
    Port_Background_Publish(PORT_BG_SCOPE_CREDITS);
    assert(Port_Background_Published() == PORT_BACKGROUND_ENDING);
    Port_Background_StageEvent(PORT_BG_EVENT_CREDITS_CLEAR, 0);
    Port_Background_Publish(PORT_BG_SCOPE_CREDITS);
    assert(Port_Background_Published() == PORT_BACKGROUND_CREDITS_CLEARED);
    Port_Background_StageEvent(PORT_BG_EVENT_ENDING_SAVE, 0);
    Port_Background_Publish(PORT_BG_SCOPE_CREDITS);
    assert(Port_Background_Published() == PORT_BACKGROUND_ENDING_SAVE);
    Port_Background_Publish(PORT_BG_SCOPE_WORLD);
    Port_Background_Publish(PORT_BG_SCOPE_CREDITS);
    assert(Port_Background_Published() == PORT_BACKGROUND_UNCLASSIFIED);
    for (unsigned health = 0; health < 3; ++health) {
        static const unsigned screens[] = { 9, 1, 7, 8 };
        Port_Background_Publish(PORT_BG_SCOPE_WORLD);
        Port_Background_StageEvent(PORT_BG_EVENT_HEALTH, health);
        for (unsigned family = 0; family < 4; ++family) {
            Port_Background_StageEvent(PORT_BG_EVENT_PAUSE, screens[family]);
            Port_Background_Publish(PORT_BG_SCOPE_PAUSE);
            assert(Port_Background_Published() == Port_Background_MenuContext(family, health));
        }
        Port_Background_StageEvent(PORT_BG_EVENT_FUSION, 0);
        Port_Background_Publish(PORT_BG_SCOPE_FUSION);
        assert(Port_Background_Published() == Port_Background_MenuContext(PORT_BACKGROUND_MENU_FUSION, health));
    }
    Port_Background_StageEvent(PORT_BG_EVENT_CREDITS, 42);
    Port_Background_Publish(PORT_BG_SCOPE_CREDITS);
    assert(Port_Background_Published() == PORT_BACKGROUND_UNCLASSIFIED);
    Port_Background_StageEvent(PORT_BG_EVENT_TRAVEL, 0);
    Port_Background_Publish(PORT_BG_SCOPE_FILE);
    Port_Background_Publish(PORT_BG_SCOPE_TRAVEL);
    assert(Port_Background_Published() == PORT_BACKGROUND_UNCLASSIFIED);
    Port_Background_StageEvent(PORT_BG_EVENT_FILE, 0);
    Port_Background_Publish(PORT_BG_SCOPE_FILE);
    assert(Port_Background_Published() == PORT_BACKGROUND_FILE_SELECT);
    Port_Background_StageEvent(PORT_BG_EVENT_GAME_OVER, 0);
    Port_Background_Publish(PORT_BG_SCOPE_GAME_OVER);
    assert(Port_Background_Published() == PORT_BACKGROUND_GAME_OVER);
    Port_Background_StageEvent(PORT_BG_EVENT_HINT, 0);
    Port_Background_Publish(PORT_BG_SCOPE_HINT);
    assert(Port_Background_Published() == PORT_BACKGROUND_MAP_HINT);
    Port_Background_StageEvent(PORT_BG_EVENT_LOCAL_HINT, 0);
    Port_Background_Publish(PORT_BG_SCOPE_LOCAL_HINT);
    assert(Port_Background_Published() == PORT_BACKGROUND_LOCAL_MAP_HINT);
    Port_Background_StageEvent(PORT_BG_EVENT_FIGURINE, 0);
    Port_Background_Publish(PORT_BG_SCOPE_FIGURINE);
    assert(Port_Background_Published() == PORT_BACKGROUND_FIGURINES);
    return 0;
}
