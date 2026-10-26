#pragma once
/*
 * port_repro.h — declarations for the headless auto-repro harnesses.
 *
 * Each Port_ReproXxx_Tick is defined in its matching port_repro_xxx.c and
 * self-gates on a TMC_REPRO_* environment variable (a no-op when unset).
 * They are driven once per frame from Port_UpdateInput() in port_bios.c.
 *
 * Owning these declarations here (instead of inline `extern`s at the call
 * site) keeps the call site free of forward decls and lets each definer
 * include this header so the compiler checks the definition against it.
 *
 * The harnesses are only compiled with the xmake option `repro_harness`
 * (TMC_REPRO_HARNESS; default on in debug, off in release). Without it the
 * port_repro_*.c files are left out of the build and every entry point
 * below collapses to an empty inline, so call sites need no #ifdefs.
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef TMC_REPRO_HARNESS
void Port_ReproBackground_Tick(unsigned int frame);
void Port_ReproBackground_Event(const char* event, unsigned int value);
/* Heading styles: 0 absent, 1 top-left plate, 2 centered pieces/techniques. */
void Port_ReproBackground_Heading(int style, int x, int y, int transition);
/* Menu driver returns bit 0 to capture, bit 1 when complete. Repro-only. */
int Port_ReproBackground_MenusTick(unsigned int frame, int decoration, int screen, int mapGroup);
int Port_ReproBackground_MenuCase(void);
int Port_ReproBackground_TravelTick(unsigned int frame, int decoration);
int Port_ReproBackground_TravelCase(void);
int Port_ReproBackground_FigurinesTick(unsigned int frame);
int Port_ReproBackground_FigurineCase(void);
int Port_ReproBackground_FigurineActionsTick(unsigned int frame);
int Port_ReproBackground_FigurineActionCase(void);
int Port_ReproBackground_HintsTick(unsigned int frame);
int Port_ReproBackground_HintCase(void);
int Port_ReproBackground_DungeonsTick(unsigned int frame);
int Port_ReproBackground_DungeonCase(void);
int Port_ReproBackground_DetailMapsTick(unsigned int frame);
int Port_ReproBackground_DetailMapCase(void);
int Port_ReproBackground_SleepTick(unsigned int frame);
int Port_ReproBackground_SleepCase(void);
int Port_ReproBackground_FusionActionsTick(unsigned int frame);
int Port_ReproBackground_FusionActionCase(void);
int Port_ReproBackground_GameOverTick(unsigned int frame);
int Port_ReproBackground_GameOverCase(void);
int Port_ReproBackground_GameOverWriteFails(void);
int Port_ReproBackground_SaveTick(unsigned int frame);
int Port_ReproBackground_SaveCase(void);
int Port_ReproBackground_SaveWriteFails(void);
int Port_ReproBackground_TravelActionsTick(unsigned int frame);
int Port_ReproBackground_TravelActionCase(void);
/* Read-only: selected index when a native two-choice enquiry is ready. */
int Port_ReproBackground_MessageChoice(void);
int Port_ReproBackground_MessageWaiting(void);
void Port_ReproRando_Tick(unsigned int frame);
void Port_ReproRando_LateTick(void);
void Port_ReproPerfcap_Tick(unsigned int frame);
void Port_ReproA11y_Tick(unsigned int frame);
void Port_ReproRoomCap_Tick(unsigned int frame);
void Port_ReproRollMacro_Tick(unsigned int frame);
void Port_ReproNpcTalk_Tick(unsigned int frame);
void Port_ReproItemGet_Tick(unsigned int frame);
#else
static inline void Port_ReproBackground_Tick(unsigned int frame) { (void)frame; }
static inline void Port_ReproBackground_Event(const char* event, unsigned int value) {
    (void)event;
    (void)value;
}
static inline void Port_ReproBackground_Heading(int style, int x, int y, int transition) {
    (void)style; (void)x; (void)y; (void)transition;
}
static inline void Port_ReproRando_Tick(unsigned int frame) { (void)frame; }
static inline void Port_ReproRando_LateTick(void) {}
static inline void Port_ReproPerfcap_Tick(unsigned int frame) { (void)frame; }
static inline void Port_ReproA11y_Tick(unsigned int frame) { (void)frame; }
static inline void Port_ReproRoomCap_Tick(unsigned int frame) { (void)frame; }
static inline void Port_ReproRollMacro_Tick(unsigned int frame) { (void)frame; }
static inline void Port_ReproNpcTalk_Tick(unsigned int frame) { (void)frame; }
static inline void Port_ReproItemGet_Tick(unsigned int frame) { (void)frame; }
#endif

#ifdef __cplusplus
}
#endif
