#include "settings.h"
#include "ui_graphics.h"
#include <string.h>
#include <stddef.h>

extern Theme_t g_theme;
extern uint8_t g_current_bg_idx;
extern uint8_t g_anim_type;
extern UiState_t ui_state;
extern uint8_t g_main_param_visible[PARAM_COUNT];
extern uint8_t g_top_icons;  extern MfaParam_t g_top_left;  extern MfaParam_t g_top_right;
extern uint8_t g_bot_icons;  extern MfaParam_t g_bot_slots[4];

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint8_t  version;
    uint8_t  bg_idx;
    uint8_t  anim_type;
    uint8_t  main_param;
    uint16_t header_color;
    uint16_t cursor_color;
    uint8_t  main_visible[PARAM_COUNT];
    uint8_t  top_icons, top_left, top_right;
    uint8_t  bot_icons, bot_slots[4];
    uint8_t  shift_en;
    uint16_t shift_up, shift_down, shift_crit;
    uint8_t misc_flags; uint8_t menu_to_sec; uint8_t msg_to_sec;
    uint16_t crc16;
    int16_t  vbat_off;      /* 0.01 В  */
    uint8_t  speed_src;
    uint16_t speed_coef;    /* 0.1 %   */
    uint16_t fuel_coef;     /* 0.1 %   */
    uint16_t final_drive;   /* 0.001   */
    uint16_t wheel_mm;
    uint16_t ratios[6];     /* 0.001   */
} Settings_t;   /* 37 байт -> пишем 64 (2 flashword по 32 Б) */

#define CRC_LEN offsetof(Settings_t, crc16)

static uint16_t crc16_ccitt(const uint8_t* d, uint32_t len) {
    uint16_t crc = 0xFFFF;
    for(uint32_t i = 0; i < len; i++) {
        crc ^= (uint16_t)d[i] << 8;
        for(uint8_t b = 0; b < 8; b++)
            crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : (crc << 1);
    }
    return crc;
}

static void Build(Settings_t* s) {
    memset(s, 0, sizeof(*s));
    s->magic  = SETTINGS_MAGIC;
    s->version = SETTINGS_VERSION;
    s->bg_idx = g_current_bg_idx;
    s->anim_type = g_anim_type;
    s->main_param = (uint8_t)ui_state.main_param;
    s->header_color = g_theme.header_color;
    s->cursor_color = g_theme.cursor_color;
    memcpy(s->main_visible, g_main_param_visible, PARAM_COUNT);
    s->top_icons = g_top_icons; s->top_left = (uint8_t)g_top_left; s->top_right = (uint8_t)g_top_right;
    s->bot_icons = g_bot_icons;
    for(uint8_t i=0;i<6;i++) s->ratios[i] = (uint16_t)(g_calib.ratios[i]*1000.0f);
    s->vbat_off    = (int16_t)(g_calib.vbat_offset * 100.0f);
    s->speed_src   = g_calib.speed_src;
    s->speed_coef  = (uint16_t)(g_calib.speed_coef * 1000.0f);
    s->fuel_coef   = (uint16_t)(g_calib.fuel_coef  * 1000.0f);
    s->final_drive = (uint16_t)(g_calib.final_drive * 1000.0f);
    s->wheel_mm    = g_calib.wheel_mm;
    for(uint8_t i=0;i<5;i++) s->ratios[i] = (uint16_t)(g_calib.ratios[i]*1000.0f);
    s->shift_en   = g_shift_assist_enabled;
    s->shift_up   = g_shift_up_rpm;
    s->shift_down = g_shift_down_rpm;
    s->shift_crit = g_shift_crit_rpm;
    s->misc_flags = (g_misc.menu_to_en?1:0)|(g_misc.msg_to_en?2:0)|(g_misc.icon_blink?4:0)|
                    (g_misc.belt_en?8:0)|(g_misc.mirror_en?16:0);
    s->menu_to_sec = g_misc.menu_to_sec;
    s->msg_to_sec  = g_misc.msg_to_sec;
}

static Settings_t last_saved;
static uint8_t  have_saved = 0;
static uint32_t change_t = 0;

static void WriteFlash(const Settings_t* s) {
    static uint32_t wr[32] __attribute__((aligned(32)));   /* 128 Б = 4 flashword по 32 Б */
    memset(wr, 0xFF, sizeof(wr));
    memcpy(wr, s, sizeof(Settings_t));
    HAL_FLASH_Unlock();
    FLASH_EraseInitTypeDef er = {0}; uint32_t pg_err = 0;
    er.TypeErase = FLASH_TYPEERASE_SECTORS;
    er.Banks     = FLASH_BANK_2;
    er.Sector    = FLASH_SECTOR_7;
    er.NbSectors = 1;
    if(HAL_FLASHEx_Erase(&er, &pg_err) == HAL_OK) {
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, SETTINGS_ADDR,      (uint32_t)wr);
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, SETTINGS_ADDR + 32, (uint32_t)wr + 32);
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, SETTINGS_ADDR + 64, (uint32_t)wr + 64);  /* <-- хвост с CRC */
    }
    HAL_FLASH_Lock();
}

void Settings_Load(void) {
    Settings_t ram;
    memcpy(&ram, (const void*)SETTINGS_ADDR, sizeof(ram));
    if(ram.magic == SETTINGS_MAGIC && ram.version == SETTINGS_VERSION &&
       crc16_ccitt((const uint8_t*)&ram, CRC_LEN) == ram.crc16 &&
       ram.bg_idx < 3 && ram.anim_type <= 2 && ram.main_param < PARAM_COUNT &&
       ram.top_left < PARAM_COUNT && ram.top_right < PARAM_COUNT) {
        g_current_bg_idx = ram.bg_idx;
        g_anim_type      = ram.anim_type;
        ui_state.main_param = (MfaParam_t)ram.main_param;
        g_theme.header_color = ram.header_color;
        g_theme.cursor_color = ram.cursor_color;
        memcpy(g_main_param_visible, ram.main_visible, PARAM_COUNT);
        g_top_icons = ram.top_icons;
        g_top_left  = (MfaParam_t)ram.top_left;
        g_top_right = (MfaParam_t)ram.top_right;
        g_bot_icons = ram.bot_icons;
        for(uint8_t i = 0; i < 4; i++) g_bot_slots[i] = (MfaParam_t)ram.bot_slots[i];
        last_saved = ram; have_saved = 1;
        g_calib.vbat_offset = ram.vbat_off / 100.0f;
        g_calib.speed_src   = (ram.speed_src <= SPEED_SRC_KLINE) ? ram.speed_src : SPEED_SRC_CAN;
        g_calib.speed_coef  = (ram.speed_coef >=500 && ram.speed_coef <=1500) ? ram.speed_coef/1000.0f : 1.0f;
        g_calib.fuel_coef   = (ram.fuel_coef  >=500 && ram.fuel_coef  <=1500) ? ram.fuel_coef /1000.0f : 1.0f;
        g_calib.final_drive = (ram.final_drive>=2500&& ram.final_drive<=5500) ? ram.final_drive/1000.0f : 3.944f;
        g_calib.wheel_mm    = (ram.wheel_mm>=1500 && ram.wheel_mm<=2600) ? ram.wheel_mm : 1985;
        g_shift_assist_enabled = ram.shift_en ? 1 : 0;
        g_shift_up_rpm   = (ram.shift_up   >=2000 && ram.shift_up   <=7000) ? ram.shift_up   : 4500;
        g_shift_down_rpm = (ram.shift_down >=800  && ram.shift_down <=3000) ? ram.shift_down : 1200;
        g_shift_crit_rpm = (ram.shift_crit >=3000 && ram.shift_crit <=8000) ? ram.shift_crit : 6000;
        g_misc.menu_to_en = (ram.misc_flags&1)?1:0;  g_misc.msg_to_en = (ram.misc_flags&2)?1:0;
        g_misc.icon_blink = (ram.misc_flags&4)?1:0;  g_misc.belt_en   = (ram.misc_flags&8)?1:0;
        g_misc.mirror_en  = (ram.misc_flags&16)?1:0;
        g_misc.menu_to_sec = (ram.menu_to_sec>=1 && ram.menu_to_sec<=60)?ram.menu_to_sec:8;
        g_misc.msg_to_sec  = (ram.msg_to_sec >=1 && ram.msg_to_sec <=30)?ram.msg_to_sec:3;
        for(uint8_t i=0;i<6;i++)
            g_calib.ratios[i] = (ram.ratios[i]>=500 && ram.ratios[i]<=5000) ? ram.ratios[i]/1000.0f : 1.0f;
    }
    /* иначе — остаются дефолты из инициализаторов глобалов */
}

/* Опрос раз в 500 мс: изменилось -> ждём 2 с стабильности -> пишем */
void Settings_Tick(void) {
    static uint32_t last_poll = 0;
    if(HAL_GetTick() - last_poll < 500) return;
    last_poll = HAL_GetTick();

    Settings_t cur; Build(&cur);
    if(have_saved && memcmp(&cur, &last_saved, CRC_LEN) == 0) { change_t = 0; return; }

    if(change_t == 0) { change_t = HAL_GetTick(); return; }
    if(HAL_GetTick() - change_t < 2000) return;

    cur.crc16 = crc16_ccitt((const uint8_t*)&cur, CRC_LEN);
    WriteFlash(&cur);
    last_saved = cur; have_saved = 1; change_t = 0;
}

void Settings_SaveNow(void) {
    Settings_t cur; Build(&cur);
    cur.crc16 = crc16_ccitt((const uint8_t*)&cur, CRC_LEN);
    WriteFlash(&cur);
    last_saved = cur; have_saved = 1; change_t = 0;
}
/* Какие экраны считаются настройками */
static uint8_t IsSettingsScreen(ScreenId_t s) {
    switch(s) {
        case SCREEN_WALLPAPER:
        case SCREEN_COLORS:
        case SCREEN_CUSTOM_COLOR:
        case SCREEN_MIDDLE_AREA:
        case SCREEN_GRAPH_ANIM:
        case SCREEN_GRAPH_TOP:
        case SCREEN_TOP_PARAMS:
        case SCREEN_GRAPH_BOTTOM:
        case SCREEN_BOT_PARAMS:
        case SCREEN_PARAM_CALIB:
        case SCREEN_CALIB_MKPP:
        case SCREEN_PARAM_SHIFT:
            return 1;
        default:
            return 0;
    }
}

/* Изменилось ли что-то относительно последней записи */
static uint8_t Settings_Dirty(void) {
    Settings_t cur; Build(&cur);
    return (!have_saved || memcmp(&cur, &last_saved, CRC_LEN) != 0) ? 1 : 0;
}

/* Вызывать в главном цикле: видит ЛЮБОЙ выход из настроек */
void Settings_CheckSave(void) {
    static uint8_t prev = 0xFF;
    if(prev == 0xFF) { prev = (uint8_t)ui_state.screen; return; }
    if((uint8_t)ui_state.screen != prev) {
        if(IsSettingsScreen((ScreenId_t)prev) && Settings_Dirty()) {
            Settings_SaveNow();
        }
        prev = (uint8_t)ui_state.screen;
    }
}
