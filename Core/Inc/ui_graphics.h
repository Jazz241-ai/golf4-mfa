#ifndef __UI_GRAPHICS_H
#define __UI_GRAPHICS_H
#include "main.h"
#include "ili9481.h"
#include <stdint.h>
#include "can_dual.h"
#include "kline.h"

typedef struct { uint16_t header_color; uint16_t cursor_color; } Theme_t;
extern Theme_t g_theme;

#define ANIM_NONE  0
#define ANIM_SLIDE 1
#define ANIM_FADE  2
extern uint8_t g_anim_type;

typedef struct {
    float avg_speed, avg_consumption, distance;
    uint32_t time_sec;
    float fuel_consumed;
    uint32_t moto_hours_sec;
    float max_speed, max_rpm, max_coolant_temp, max_oil_temp, max_intake_temp;
} TripData_t;
extern TripData_t g_trip_data;
extern TripData_t g_total_data;

typedef enum {
    SCREEN_MAIN = 0, SCREEN_MENU_ROOT, SCREEN_MENU_SUB,
    SCREEN_WALLPAPER, SCREEN_MIDDLE_AREA, SCREEN_COLORS, SCREEN_CUSTOM_COLOR,
    SCREEN_MEAS_BC1, SCREEN_MEAS_BC2, SCREEN_MEAS_GENERAL, SCREEN_MEAS_EXTRA,
    SCREEN_MEAS_OIL, SCREEN_MEAS_POWER, SCREEN_MEAS_ACCEL, SCREEN_MEAS_GRAPHS, SCREEN_GRAPHS_PICK, SCREEN_GRAPHS_VIEW, SCREEN_MEAS_GAUGES,
    SCREEN_PARAM_SPEED_LIM, SCREEN_PARAM_ECO, SCREEN_PARAM_CALIB, SCREEN_PARAM_GEARBOX,
    SCREEN_PARAM_SHIFT, SCREEN_PARAM_NAV, SCREEN_PARAM_COFFEE, SCREEN_PARAM_MISC, SCREEN_CALIB_MKPP,
	SCREEN_GRAPH_TOP, SCREEN_TOP_PARAMS, SCREEN_GRAPH_OK, SCREEN_GRAPH_BOTTOM, SCREEN_BOT_PARAMS,
    SCREEN_GRAPH_SCREENSAVER, SCREEN_GRAPH_ANIM,
    SCREEN_DIAG_ECU, SCREEN_DIAG_MEAS, SCREEN_DIAG_READ_ERR, SCREEN_DIAG_DTC_DETAIL, SCREEN_DIAG_CLEAR_ERR, SCREEN_DIAG_OBD, SCREEN_DIAG_KLINE,
    SCREEN_SERV_LANG, SCREEN_SERV_UNITS, SCREEN_SERV_LIGHT, SCREEN_SERV_COMFORT,
	SCREEN_SERV_TPMS, SCREEN_TPMS_PARAM, SCREEN_TPMS_WHEEL, SCREEN_TPMS_PRESS, SCREEN_SERV_PNEUMO,
    SCREEN_SERV_CAN_MON_MENU, SCREEN_SERV_CAN_MON_DATA,
    SCREEN_SERV_SS, SCREEN_SERV_MOTOR, SCREEN_SERV_FW, SCREEN_SERV_DEBUG,
	SCREEN_MSG_LIST, SCREEN_MSG_VIEW, SCREEN_COUNT
} ScreenId_t;

typedef enum {
    SUB_MEAS = 0, SUB_PARAM, SUB_GRAPH, SUB_DIAG, SUB_SERVICE, SUB_COUNT
} SubMenuId_t;

typedef enum {
PARAM_SPEED = 0, PARAM_RPM, PARAM_COOLANT_TEMP, PARAM_CONSUMPTION_INSTANT,
PARAM_CONSUMPTION_AVG, PARAM_DISTANCE, PARAM_FUEL_LEFT, PARAM_RANGE,
PARAM_TRIP_TIME, PARAM_OIL_TEMP, PARAM_OUTSIDE_TEMP, PARAM_INTAKE_TEMP,
PARAM_VOLTAGE, PARAM_STEERING_ANGLE,
PARAM_ODOMETER,        // <-- ДОБАВИТЬ: общий пробег
PARAM_COUNT
} MfaParam_t;

typedef struct {
    ScreenId_t screen; ScreenId_t prev_screen;
    MfaParam_t main_param; uint8_t menu_idx; SubMenuId_t active_submenu;
} UiState_t;

extern UiState_t ui_state;
extern float param_values[PARAM_COUNT];
extern uint8_t g_current_bg_idx;
extern uint8_t g_main_param_visible[PARAM_COUNT];

extern uint8_t g_top_icons;          // 1 = показывать иконки в верхней строке
extern MfaParam_t g_top_left;        // параметр левого слота
extern MfaParam_t g_top_right;       // параметр правого слота
extern uint8_t g_top_edit_side;      // 0 = редактируем левый, 1 = правый

extern uint8_t g_bot_icons;            // 1 = иконки в нижних слотах
extern MfaParam_t g_bot_slots[4];      // параметры 4 нижних слотов
extern uint8_t g_bot_edit_slot;        // 0..3 — какой слот редактируем

extern MfaParam_t g_graph_slots[5];   // PARAM_COUNT = «Пусто»
extern uint8_t g_graph_pick_slot;
extern uint8_t g_graphs_paused;
extern uint8_t g_graphs_reset;

extern uint8_t g_kline_auto_state;
extern uint8_t g_kline_auto_result;
extern uint8_t g_kline_kb1, g_kline_kb2;
extern uint8_t g_kline_par;   //  вот этой не хватало
extern uint8_t g_kline_inv;

extern float g_oil_level;
extern uint8_t g_graph_pick_slot;
extern uint8_t g_graphs_paused;
extern uint8_t g_graphs_reset;
extern uint8_t g_kline_ackmode;
extern uint8_t  g_kline_dialog;  // 255 = ещё не спрашивали, 0 = молчит, 1 = ответил
extern uint32_t g_kline_query;    // счётчик запросов (очистка дампа)

extern uint8_t g_kline_dialog_running;  // 1 = диалог запущен

extern uint8_t g_kline_btn_up;
extern uint8_t g_kline_btn_down;
extern uint8_t g_kline_btn_ok;

extern uint8_t  g_kl_state;        /* 0=idle, 1=опрос, 2=успех, 3=молчит */
extern uint8_t  g_kl_run_req;      /* 1 = запустить сессию из main */
extern uint8_t  g_kl_kb1, g_kl_kb2;
extern uint8_t  g_kl_ident[96];
extern uint8_t  g_kl_ident_len;

extern uint8_t g_diag_ecu_menu_open;
extern uint8_t g_diag_dtc_selected_idx;

/* ==================== Разное (Misc) ==================== */
typedef struct {
    uint8_t menu_to_en;   /* таймаут меню вкл           */
    uint8_t menu_to_sec;  /* сек, 1..60                 */
    uint8_t msg_to_en;    /* таймаут сообщений вкл      */
    uint8_t msg_to_sec;   /* сек, 1..30                 */
    uint8_t icon_blink;   /* 1 = иконки мигают          */
    uint8_t belt_en;      /* предупреждение о ремне     */
    uint8_t mirror_en;    /* парковочное зеркало        */
} MiscCfg_t;
extern MiscCfg_t g_misc;
extern uint8_t g_misc_edit;
extern uint8_t g_belt_unfastened;      /* источник: CAN (подключить позже) */
extern uint8_t g_mirror_joystick_pass; /* джойстик на пассажирском зеркале */
extern uint8_t g_mirror_dipped;        /* зеркало опущено                  */
extern uint8_t g_msg_view_id;
const char* Msg_Text(uint8_t id);   /* <-- добавить */
uint8_t     Msg_Sev(uint8_t id);    /* <-- добавить */

/* ==================== Сообщения ==================== */
#define MSG_COUNT 23
enum {
    MSG_WASHER=0, MSG_PADS, MSG_BELT, MSG_BULB, MSG_OIL_PRESS, MSG_COOLANT,
    MSG_TPMS_WARN, MSG_TPMS_CAL,
    MSG_CHARGE, MSG_COOL_LEVEL, MSG_ABS, MSG_ESP, MSG_BRAKE, MSG_AIRBAG,
    MSG_MIL, MSG_EPC, MSG_CAT, MSG_FUEL, MSG_STEER, MSG_AWD, MSG_LEVEL,
    MSG_EPB, MSG_GEARBOX
};
#define MSG_SEV_INFO 0
#define MSG_SEV_WARN 1
#define MSG_SEV_CRIT 2
/* пиктограммы неисправностей (рисуются примитивами, без ассетов) */
typedef enum {
    FICON_GENERAL=0, FICON_OIL, FICON_TEMP, FICON_BATT, FICON_BRAKE, FICON_ABS,
    FICON_ESP, FICON_AIRBAG, FICON_ENGINE, FICON_TIRE, FICON_LIGHT, FICON_BELT,
    FICON_WASHER, FICON_STEER, FICON_AWD, FICON_LEVEL, FICON_EPB, FICON_GEAR,
    FICON_FUEL
} FaultIconId_t;
uint8_t Msg_Icon(uint8_t id);

#define MSG_SEV_INFO 0
#define MSG_SEV_WARN 1
#define MSG_SEV_CRIT 2
void    Msg_Raise(uint8_t id);
void    Msg_Clear(uint8_t id);
uint8_t Msg_TopUnacked(void);          /* id+1 или 0, критические первыми */
uint8_t Msg_IsActive(uint8_t id);
uint8_t Msg_IsAcked(uint8_t id);
void    Msg_Ack(uint8_t id);

/* ==================== Калибровка ==================== */
#define SPEED_SRC_CAN   0
#define SPEED_SRC_KLINE 1
typedef struct {
    float    vbat_offset;   /* В, добавка к измеренному напряжению */
    uint8_t  speed_src;     /* SPEED_SRC_*                         */
    float    speed_coef;    /* множитель скорости   (1.0 = завод)  */
    float    fuel_coef;     /* множитель расхода    (1.0 = завод)  */
    float    final_drive;   /* главная пара МКПП                   */
    uint16_t wheel_mm;      /* окружность колеса, мм               */
    float    ratios[6];     /* передаточные числа 1..5             */
} CalibData_t;
extern CalibData_t g_calib;
extern uint8_t g_calib_edit;   /* 1 = сейчас меняем значение строки */

/* ==================== Помощник переключения передач ==================== */
extern uint8_t  g_shift_assist_enabled;  /* 0 = выключен, 1 = включен   */
extern uint16_t g_shift_up_rpm;          /* подсказка "вверх"           */
extern uint16_t g_shift_down_rpm;        /* подсказка "вниз"            */
extern uint16_t g_shift_crit_rpm;        /* красная стрелка превышения  */
extern uint8_t  g_shift_edit;            /* 1 = редактируем порог       */

/* ==================== Сообщения: дополнение TPMS ==================== */
/* (замените MSG_COUNT 6 -> 8 и добавьте в enum: MSG_TPMS_WARN=6, MSG_TPMS_CAL=7) */

/* ==================== Косвенный TPMS ==================== */
#define TPMS_WHEELS 4
#define TPMS_ST_IDLE      0
#define TPMS_ST_MONITOR   1
#define TPMS_ST_CAL_FULL  2
#define TPMS_ST_CAL_WHEEL 3
typedef struct {
  uint8_t  en, calibrated, state, cal_wheel;
  uint16_t samples;
  float    p_ref[TPMS_WHEELS];
  float    dev_ref[TPMS_WHEELS];
  float    dev_avg[TPMS_WHEELS];
  uint8_t  warn[TPMS_WHEELS];
} TpmsData_t;
extern TpmsData_t g_tpms;
extern float g_tpms_p_est[TPMS_WHEELS];
extern float g_tpms_temp;
extern uint8_t g_tpms_press_mode;   /* 0 = полное обучение, 1 = замена колеса */
extern uint8_t g_tpms_press_edit;   /* 1 = редактируем строку давления */
void Tpms_Tick(void);
void Tpms_Close(void);


void UI_Init(void);
void UI_Update(void);
void UI_Render(void);

/* События кнопок, передаваемые из UI_Update в UI_DrawDiag_Kline */
extern uint8_t g_kline_btn_up;
extern uint8_t g_kline_btn_down;
extern uint8_t g_kline_btn_ok;

/* ==========================================================================
Диагностика: выбор ЭБУ / группы / ошибки (данные + запросы к main)
========================================================================== */
extern uint8_t        g_diag_addr;          /* выбранный адрес ЭБУ            */
extern uint8_t        g_diag_group;         /* номер группы измерений         */
extern KLineMeas_t    g_diag_meas;          /* последняя прочитанная группа   */
extern KLineDtcList_t g_diag_dtcs;          /* последний список ошибок        */
extern uint8_t        g_diag_dtc_busy;      /* 1 = идёт опрос                 */
extern uint8_t        g_diag_dtc_result;    /* 0=нет данных 1=успех 2=молчит  */
extern uint8_t        g_diag_req_meas;      /* запрос к main: прочитать группу*/
extern uint8_t        g_diag_req_dtc;       /* запрос к main: прочитать ошибки*/
extern uint8_t        g_diag_req_clr;       /* запрос к main: стереть ошибки  */
extern uint8_t        g_diag_clr_result;    /* 0=нет 1=стёрто 2=ошибка        */

#endif
