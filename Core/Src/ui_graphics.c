#include "ui_graphics.h"
#include "ui_screens.h"
#include "can_dual.h"
#include "buttons.h"
#include "main.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "kline.h"

Theme_t g_theme = {0xF800, 0xF800};
uint8_t g_anim_type = ANIM_SLIDE;
TripData_t g_trip_data = {0};
TripData_t g_total_data = {0};
uint8_t g_current_bg_idx = 0;
uint8_t g_top_icons = 1;
MfaParam_t g_top_left  = PARAM_VOLTAGE;
MfaParam_t g_top_right = PARAM_OUTSIDE_TEMP;
uint8_t g_top_edit_side = 0;
uint8_t g_main_param_visible[PARAM_COUNT];
float param_values[PARAM_COUNT] = {0};
float g_oil_level = 0.72f;
MfaParam_t g_graph_slots[5] = {PARAM_RPM, PARAM_SPEED, PARAM_COOLANT_TEMP, PARAM_COUNT, PARAM_COUNT};
uint8_t g_graph_pick_slot = 0;
uint8_t g_graphs_paused = 0;
uint8_t g_graphs_reset = 0;

// K-Line globals
uint8_t g_kline_kb1 = 0, g_kline_kb2 = 0;
uint8_t g_kl_state = 0;
uint8_t g_kl_run_req = 0;
uint8_t g_kl_ident[96];
uint8_t g_kl_ident_len = 0;

// Diagnostics globals
uint8_t g_diag_addr = 0x01;
uint8_t g_diag_group = 0;
KLineMeas_t g_diag_meas = {0};
KLineDtcList_t g_diag_dtcs = {0};
uint8_t g_diag_dtc_busy = 0;
uint8_t g_diag_dtc_result = 0;
uint8_t g_diag_req_meas = 0;
uint8_t g_diag_req_dtc = 0;
uint8_t g_diag_req_clr = 0;
uint8_t g_diag_clr_result = 0;
uint8_t g_diag_dtc_selected_idx = 0;

uint8_t g_kline_ackmode = 0;
uint8_t g_kline_dialog = 255;
uint8_t g_kline_dialog_running = 0;

uint8_t  g_shift_assist_enabled = 0;
uint16_t g_shift_up_rpm   = 4500;
uint16_t g_shift_down_rpm = 1200;
uint16_t g_shift_crit_rpm = 6000;
uint8_t  g_shift_edit = 0;

uint8_t g_bot_icons = 1;
MfaParam_t g_bot_slots[4] = {PARAM_FUEL_LEFT, PARAM_RANGE, PARAM_CONSUMPTION_INSTANT, PARAM_SPEED};
uint8_t g_bot_edit_slot = 0;

UiState_t ui_state = {
    .screen = SCREEN_MAIN, .prev_screen = SCREEN_MAIN,
    .main_param = PARAM_SPEED, .menu_idx = 0, .active_submenu = SUB_MEAS
};

CalibData_t g_calib = {
    .vbat_offset = 0.0f, .speed_src = SPEED_SRC_CAN,
    .speed_coef  = 1.0f, .fuel_coef  = 1.0f,
    .final_drive = 3.944f, .wheel_mm = 1985,
    .ratios = {3.300f, 1.944f, 1.308f, 1.029f, 0.837f, 0.700f},
};
uint8_t g_calib_edit = 0;

static ScreenId_t graph_menu_parent = SCREEN_MENU_SUB;
static uint8_t graph_menu_parent_idx = 0;
static ScreenId_t color_menu_parent = SCREEN_COLORS;
static uint8_t color_menu_parent_idx = 0;
static ColorPicker_t cp_state;

static const char* MENU_ROOT[] = {"Измерения","Параметры","Графика","Диагностика","Сообщения","Служебные","Выход", NULL};
static const char* SUB_IZM[]  = {"Данные БК1", "Данные БК2", "Общие данные", "Уровень масла", "Мощность", "Разгон", "Графики", "Назад", NULL};
static const char* SUB_PAR[]  = {"Огр.скорости", "ЭКО режим", "Калибровка", "Режим АКПП", "Перекл.передач", "Навигация", "Кофе-брэйк", "Разное", "Назад", NULL};
static const char* SUB_GRA[]  = {"Подложка", "Цвета", "Верхняя строка", "Средняя область", "Нижняя область", "Область ОК", "Заставка", "Анимация", "Назад", NULL};
static const char* SUB_DIA[]  = {"[Выбор блока]", "Блоки измерений", "Читать ошибки", "Стереть ошибки", "K-Line терминал", "Назад", NULL};
static const char* SUB_SERV[] = {"Язык", "Единицы", "Свет и обзор", "Комфорт", "TPMS", "Пневмоподв.", "КАН-монитор", "Скринсэйв", "МоторИнфо", "Прошивка", "Debug", "Назад", NULL};

static const char** SUBMENUS[] = {SUB_IZM, SUB_PAR, SUB_GRA, SUB_DIA, SUB_SERV};
static const uint8_t SUB_COUNTS[] = {8, 9, 9, 6, 12};
static const char* SUB_TITLES[] = {"ИЗМЕРЕНИЯ", "ПАРАМЕТРЫ", "ГРАФИКА", "ДИАГНОСТИКА", "СЛУЖЕБНЫЕ"};



/* ==========================================================================
   РАЗНОЕ + ПОДСИСТЕМА СООБЩЕНИЙ (единый блок, выше UI_Update)
   ========================================================================== */
MiscCfg_t g_misc = { .menu_to_en = 0, .menu_to_sec = 8,
                     .msg_to_en  = 1, .msg_to_sec  = 3,
                     .icon_blink = 1, .belt_en     = 0, .mirror_en = 0 };
uint8_t g_misc_edit            = 0;
uint8_t g_belt_unfastened      = 0;
uint8_t g_mirror_joystick_pass = 0;
uint8_t g_mirror_dipped        = 0;
uint8_t g_msg_view_id          = 0;

/* ---- Таблица сообщений и состояние ---- */
static const struct { uint8_t sev; const char* txt; } MSG_DEF[MSG_COUNT] = {
 { MSG_SEV_WARN, "Низкий уровень омывающей жидкости. Долейте омывайку." },
 { MSG_SEV_WARN, "Износ тормозных колодок. Замените колодки." },
 { MSG_SEV_WARN, "Пристегните ремень безопасности!" },
 { MSG_SEV_WARN, "Неисправен динамический корректор фар! Проверьте наружное освещение." },
 { MSG_SEV_CRIT, "АВАРИЙНОЕ ДАВЛЕНИЕ МАСЛА! Немедленно остановите двигатель." },
 { MSG_SEV_CRIT, "Перегрев охлаждающей жидкости! Остановитесь и дайте двигателю остыть." },
 { MSG_SEV_WARN, "TPMS: отклонение давления в колесе от эталона. Проверьте и подкачайте шины." },
 { MSG_SEV_INFO, "TPMS: эталонное давление сохранено." },
 { MSG_SEV_CRIT, "НЕТ ЗАРЯДА АКБ! Неисправен генератор или ремень привода. Прекратите движение!" },
 { MSG_SEV_WARN, "Недостаточный уровень охлаждающей жидкости. Проверьте расширительный бачок." },
 { MSG_SEV_WARN, "Неисправность ABS: антиблокировочная система отключена." },
 { MSG_SEV_WARN, "Неисправность ESP/ASR: система стабилизации отключена." },
 { MSG_SEV_CRIT, "НЕИСПРАВНОСТЬ ТОРМОЗНОЙ СИСТЕМЫ! Проверьте уровень ТЖ и стояночный тормоз." },
 { MSG_SEV_WARN, "Неисправность системы подушек безопасности (SRS)." },
 { MSG_SEV_WARN, "Check Engine: ошибка системы управления двигателем." },
 { MSG_SEV_WARN, "EPC: неисправность электронной педали газа / дроссельной заслонки." },
 { MSG_SEV_WARN, "Неисправность катализатора (система очистки ОГ)." },
 { MSG_SEV_WARN, "Низкий уровень топлива — заправьте автомобиль." },
 { MSG_SEV_WARN, "Неисправность усилителя рулевого управления (Lenkhilfe)." },
 { MSG_SEV_WARN, "Неисправность муфты полного привода (4Motion)." },
 { MSG_SEV_WARN, "Неисправность регулировки уровня / пневмоподвески." },
 { MSG_SEV_WARN, "Неисправность электромеханического стояночного тормоза (EPB)." },
 { MSG_SEV_WARN, "АКПП: аварийный режим (Notlauf). Обратитесь в сервис." },
 { MSG_SEV_INFO, "ESP отключена кнопкой. Стабилизация неактивна." },
};
static const uint8_t MSG_ICON[MSG_COUNT] = {
 FICON_WASHER, FICON_BRAKE, FICON_BELT,  FICON_LIGHT, FICON_OIL,  FICON_TEMP,
 FICON_TIRE,   FICON_TIRE,  FICON_BATT,  FICON_TEMP,  FICON_ABS,  FICON_ESP,
 FICON_BRAKE,  FICON_AIRBAG,FICON_ENGINE,FICON_ENGINE,FICON_ENGINE,FICON_FUEL,
 FICON_STEER,  FICON_AWD,   FICON_LEVEL, FICON_EPB,   FICON_GEAR, FICON_ESP,
};
uint8_t Msg_Icon(uint8_t id){ return (id < MSG_COUNT) ? MSG_ICON[id] : FICON_GENERAL; }
static uint8_t  msg_active = 0;
static uint8_t  msg_acked  = 0;
static uint32_t msg_raised[MSG_COUNT];

void Msg_Raise(uint8_t id){ if(id < MSG_COUNT){ msg_active |= (1u<<id); msg_acked &= ~(1u<<id); msg_raised[id] = HAL_GetTick(); } }
void Msg_Clear(uint8_t id){ if(id < MSG_COUNT){ msg_active &= ~(1u<<id); msg_acked &= ~(1u<<id); } }
uint8_t Msg_IsActive(uint8_t id){ return (id < MSG_COUNT) ? ((msg_active >> id) & 1) : 0; }
uint8_t Msg_IsAcked(uint8_t id) { return (id < MSG_COUNT) ? ((msg_acked  >> id) & 1) : 0; }
void Msg_Ack(uint8_t id){ if(id < MSG_COUNT) msg_acked |= (1u<<id); }
uint8_t Msg_TopUnacked(void){
    uint8_t m = msg_active & ~msg_acked;
    if(!m) return 0;
    for(uint8_t i = 0; i < MSG_COUNT; i++) if(((m >> i) & 1) && MSG_DEF[i].sev == MSG_SEV_CRIT) return i + 1;
    for(uint8_t i = 0; i < MSG_COUNT; i++) if((m >> i) & 1) return i + 1;
    return 0;
}
const char* Msg_Text(uint8_t id){ return (id < MSG_COUNT) ? MSG_DEF[id].txt : ""; }
uint8_t     Msg_Sev(uint8_t id) { return (id < MSG_COUNT) ? MSG_DEF[id].sev : MSG_SEV_INFO; }

/* ---- Тики ---- */
static uint32_t msg_was_shown = 0;   /* бит = сообщение уже было показано на главном */

static void Msg_Tick(void){
    uint32_t now = HAL_GetTick();
    msg_was_shown &= msg_active & ~msg_acked;      /* сброс для сброшенных/снятых */
    if (ui_state.screen != SCREEN_MAIN) return;    /* таймер не идёт, пока мы в меню */
    for(uint8_t i = 0; i < MSG_COUNT; i++){
        uint8_t bit = 1u << i;
        if((msg_active & bit) && !(msg_acked & bit)){
            if(!(msg_was_shown & bit)){
                msg_was_shown |= bit;
                msg_raised[i] = now;               /* отсчёт с момента показа */
            }
            else if(MSG_DEF[i].sev != MSG_SEV_CRIT &&
                    g_misc.msg_to_en &&
                    (now - msg_raised[i]) > (uint32_t)g_misc.msg_to_sec * 1000u)
                msg_acked |= bit;                  /* автосброс только с экрана */
        }
    }
}
static void Belt_Tick(void){
    static uint8_t raised = 0;
    uint8_t cond = g_misc.belt_en && g_belt_unfastened &&
                   CAN_IsMotorAlive() && param_values[PARAM_SPEED] > 10.0f;
    if(cond && !raised){ Msg_Raise(MSG_BELT); raised = 1; }
    if(!cond && raised){ Msg_Clear(MSG_BELT); raised = 0; }
}
void Mirror_HW_Apply(uint8_t dip){ (void)dip; /* TODO: привод зеркала */ }

static void MirrorDip_Tick(void){
    static uint8_t st = 0; static uint32_t t = 0;
    uint8_t  rev = (g_motor_can.gear_position == 2);   /* R */
    uint32_t now = HAL_GetTick();
    switch(st){
    case 0: if(g_misc.mirror_en && rev && g_mirror_joystick_pass){ st = 1; t = now; } break;
    case 1: if(!g_misc.mirror_en || !rev || !g_mirror_joystick_pass){ st = 0; break; }
            if(now - t >= 5000){ g_mirror_dipped = 1; Mirror_HW_Apply(1); st = 2; } break;
    case 2: if(!g_misc.mirror_en || !g_mirror_joystick_pass){ g_mirror_dipped = 0; Mirror_HW_Apply(0); st = 0; break; }
            if(!rev){ st = 3; t = now; } break;
    case 3: if(rev){ st = 2; break; }
            if(now - t >= 5000){ g_mirror_dipped = 0; Mirror_HW_Apply(0); st = 0; } break;
    }
}

static uint8_t IsTimeoutExempt(ScreenId_t s){
    switch(s){
    case SCREEN_DIAG_ECU: case SCREEN_DIAG_MEAS: case SCREEN_DIAG_READ_ERR:
    case SCREEN_DIAG_DTC_DETAIL: case SCREEN_DIAG_CLEAR_ERR: case SCREEN_DIAG_KLINE:
    case SCREEN_MEAS_POWER: case SCREEN_MEAS_ACCEL: case SCREEN_MEAS_OIL:
    case SCREEN_GRAPHS_VIEW: case SCREEN_SERV_CAN_MON_DATA: case SCREEN_MSG_VIEW:
        return 1;
    default: return 0;
    }
}


TpmsData_t g_tpms = {0};
float g_tpms_p_est[4] = {0};
float g_tpms_temp = 0;
#define TPMS_ABS_ADDR  0x03
#define TPMS_ABS_GROUP 0x00   /* если пусто — попробуйте 0x01 */
#define TPMS_K_BAR_PCT 0.5f
#define TPMS_CAL_N     60
#define TPMS_WARN_DROP 0.4f
static uint8_t  tpms_sess = 0;
static uint32_t tpms_last_poll = 0, tpms_last_close = 0;
uint8_t g_tpms_press_mode = 0;
uint8_t g_tpms_press_edit = 0;

void Tpms_Close(void){ if(tpms_sess){ KWP1281_End(); tpms_sess=0; tpms_last_close=HAL_GetTick(); } }

static uint8_t tpms_busy_elsewhere(void){
  return g_diag_dtc_busy || g_diag_req_meas || g_diag_req_dtc || g_diag_req_clr ||
         g_kline_dialog_running ||
         ui_state.screen==SCREEN_DIAG_ECU || ui_state.screen==SCREEN_DIAG_MEAS ||
         ui_state.screen==SCREEN_DIAG_READ_ERR || ui_state.screen==SCREEN_DIAG_DTC_DETAIL ||
         ui_state.screen==SCREEN_DIAG_CLEAR_ERR || ui_state.screen==SCREEN_DIAG_KLINE;
}
static uint8_t tpms_read_ws(float ws[4]){
  uint8_t resp[32], rlen=0;
  if(!KWP1281_ReadGroup(TPMS_ABS_GROUP, resp, &rlen) || rlen < 12) return 0;
  for(uint8_t ch=0; ch<4; ch++){
    uint8_t t = resp[ch*3];
    uint16_t raw = ((uint16_t)resp[ch*3+1]<<8) | resp[ch*3+2];
    ws[ch] = (t==0x07) ? raw*0.01f : (float)raw;
  }
  return 1;
}
void Tpms_Tick(void){
  uint32_t now = HAL_GetTick();
  /* модельная температура колеса: наружная + нагрев от скорости */
  static uint32_t t_sec=0; static float heat=0;
  if(now - t_sec >= 1000){ t_sec=now;
    float spd=param_values[PARAM_SPEED];
    float target=(spd>5.0f)?((spd>120.0f)?20.0f:spd*20.0f/120.0f):0.0f;
    heat += (target-heat)*0.02f;
    g_tpms_temp = param_values[PARAM_OUTSIDE_TEMP] + heat;
  }
  if(!g_tpms.en){ Tpms_Close(); g_tpms.state=TPMS_ST_IDLE; return; }
  if(tpms_busy_elsewhere()){ Tpms_Close(); return; }
  uint8_t cal = (g_tpms.state==TPMS_ST_CAL_FULL || g_tpms.state==TPMS_ST_CAL_WHEEL);
  uint32_t interval = cal ? 1000 : 5000;
  if(!tpms_sess){
    uint8_t on_screen = (ui_state.screen==SCREEN_SERV_TPMS || ui_state.screen==SCREEN_TPMS_PARAM || ui_state.screen==SCREEN_TPMS_WHEEL);
    uint8_t allow = (on_screen || cal) ? (now - tpms_last_close > 1500) : (now - tpms_last_close > 30000);
    if(!allow || now - tpms_last_poll < interval) return;
    tpms_last_poll = now;
    uint8_t k1,k2;
    if(KLine_KWP1281_Dialog(TPMS_ABS_ADDR,&k1,&k2) > 2){ tpms_last_close = now; return; }
    char dummy[96]; uint8_t dl; KWP1281_ReadIdent(dummy,sizeof(dummy),&dl);
    tpms_sess = 1;
  }
  if(now - tpms_last_poll < interval) return;
  tpms_last_poll = now;
  float ws[4];
  if(!tpms_read_ws(ws)){ Tpms_Close(); return; }
  float spd  = param_values[PARAM_SPEED];
  float mean = (ws[0]+ws[1]+ws[2]+ws[3]) * 0.25f;
  if(spd < 30.0f || spd > 160.0f || mean < 25.0f) return;
  if(fabsf(param_values[PARAM_STEERING_ANGLE]) > 40.0f) return;
  for(uint8_t i=0;i<4;i++){
    float dev = (ws[i]-mean)/mean*100.0f;
    g_tpms.dev_avg[i] += (dev - g_tpms.dev_avg[i]) * 0.05f;
  }
  if(g_tpms.state==TPMS_ST_CAL_FULL){
    if(++g_tpms.samples >= TPMS_CAL_N){
      for(uint8_t i=0;i<4;i++){ g_tpms.dev_ref[i]=g_tpms.dev_avg[i]; g_tpms.warn[i]=0; }
      g_tpms.calibrated=1; g_tpms.state=TPMS_ST_MONITOR;
      Msg_Raise(MSG_TPMS_CAL);
    }
  } else if(g_tpms.state==TPMS_ST_CAL_WHEEL){
    if(++g_tpms.samples >= TPMS_CAL_N/2){
      uint8_t w=g_tpms.cal_wheel;
      g_tpms.dev_ref[w]=g_tpms.dev_avg[w]; g_tpms.warn[w]=0;
      g_tpms.state=TPMS_ST_MONITOR;
      Msg_Raise(MSG_TPMS_CAL);
    }
  } else if(g_tpms.calibrated){
    g_tpms.state = TPMS_ST_MONITOR;
    for(uint8_t i=0;i<4;i++){
      g_tpms_p_est[i] = g_tpms.p_ref[i] - TPMS_K_BAR_PCT*(g_tpms.dev_avg[i]-g_tpms.dev_ref[i]);
      if(!g_tpms.warn[i] && (g_tpms.p_ref[i]-g_tpms_p_est[i]) > TPMS_WARN_DROP){
        g_tpms.warn[i]=1; Msg_Raise(MSG_TPMS_WARN);
      }
      if(g_tpms.warn[i] && fabsf(g_tpms.p_ref[i]-g_tpms_p_est[i]) < 0.2f) g_tpms.warn[i]=0;
    }
  }
}


static void GetCurrentMenu(const char*** list, uint8_t* count) {
    if (ui_state.screen == SCREEN_MENU_ROOT) { *list = MENU_ROOT; *count = 7; }
    else { *list = SUBMENUS[ui_state.active_submenu]; *count = SUB_COUNTS[ui_state.active_submenu]; }
}

static void UpdateTripData(void) {
    if(!CAN_IsMotorAlive()) return;
    float spd  = param_values[PARAM_SPEED];
    float cons = param_values[PARAM_CONSUMPTION_INSTANT];
    float fuel_ps = (spd > 2.0f) ? (cons * spd / 100.0f / 3600.0f) : (cons / 3600.0f);

    g_trip_data.distance += (spd / 3600.0f);
    g_trip_data.time_sec++;
    g_trip_data.fuel_consumed += fuel_ps;
    g_trip_data.moto_hours_sec++;

    g_total_data.distance += (spd / 3600.0f);
    g_total_data.time_sec++;
    g_total_data.fuel_consumed += fuel_ps;
    g_total_data.moto_hours_sec++;

    g_trip_data.avg_speed = g_trip_data.time_sec>0 ? (g_trip_data.distance/g_trip_data.time_sec*3600) : 0;
    g_trip_data.avg_consumption = g_trip_data.distance>0.1 ? (g_trip_data.fuel_consumed/g_trip_data.distance*100) : 0;
    g_total_data.avg_speed = g_total_data.time_sec>0 ? (g_total_data.distance/g_total_data.time_sec*3600) : 0;
    g_total_data.avg_consumption = g_total_data.distance>0.1 ? (g_total_data.fuel_consumed/g_total_data.distance*100) : 0;

    if(spd > g_trip_data.max_speed) g_trip_data.max_speed = spd;
    if(param_values[PARAM_RPM] > g_trip_data.max_rpm) g_trip_data.max_rpm = param_values[PARAM_RPM];
    if(param_values[PARAM_COOLANT_TEMP] > g_trip_data.max_coolant_temp) g_trip_data.max_coolant_temp = param_values[PARAM_COOLANT_TEMP];
    if(param_values[PARAM_OIL_TEMP] > g_trip_data.max_oil_temp) g_trip_data.max_oil_temp = param_values[PARAM_OIL_TEMP];
    if(param_values[PARAM_INTAKE_TEMP] > g_trip_data.max_intake_temp) g_trip_data.max_intake_temp = param_values[PARAM_INTAKE_TEMP];

    param_values[PARAM_CONSUMPTION_AVG] = g_trip_data.avg_consumption;
}

void UI_Init(void) { memset(g_main_param_visible, 1, sizeof(g_main_param_visible)); }

void UI_Update(void) {
    Buttons_Poll();
    static uint32_t up_hold=0, down_hold=0;
    static uint8_t up_act=0, down_act=0;
    uint8_t trig_up=0, trig_down=0;
    uint8_t pin_up   = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_1)==0;
    uint8_t pin_down = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_2)==0;

    if(pin_up)   { if(!up_act){up_act=1; up_hold=HAL_GetTick();} else if(HAL_GetTick()-up_hold>400){up_hold=HAL_GetTick()-120; trig_up=1;} } else up_act=0;
    if(pin_down) { if(!down_act){down_act=1; down_hold=HAL_GetTick();} else if(HAL_GetTick()-down_hold>400){down_hold=HAL_GetTick()-120; trig_down=1;} } else down_act=0;

    uint8_t ev_up   = Buttons_GetEvent(BTN_UP);
    uint8_t ev_down = Buttons_GetEvent(BTN_DOWN);
    if(ev_up||trig_up) trig_up=1;
    if(ev_down||trig_down) trig_down=1;

    Msg_Tick(); Belt_Tick(); MirrorDip_Tick();
    /* --- Таймаут меню --- */
    {
        static uint32_t last_act = 0;
        uint8_t pin_ok = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_3)==0;
        if(pin_up || pin_down || pin_ok) last_act = HAL_GetTick();
        if(g_misc.menu_to_en && ui_state.screen != SCREEN_MAIN &&
           !IsTimeoutExempt(ui_state.screen) &&
           (HAL_GetTick() - last_act) > (uint32_t)g_misc.menu_to_sec * 1000u) {
            ui_state.screen = SCREEN_MAIN; ui_state.menu_idx = 0;
            g_misc_edit = 0; last_act = HAL_GetTick();
        }
    }

    // --- K-Line Debug Screen Logic ---
    if(ui_state.screen==SCREEN_DIAG_KLINE) {
        if(Buttons_GetEvent(BTN_OK) && g_kl_state != 1) {
            g_kl_run_req = 1;
            g_kl_state = 1;
        }
        static uint32_t h=0; static uint8_t act=0, fired=0;
        uint8_t dn = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_2)==0;
        if(dn){
            if(!act){act=1;h=HAL_GetTick();fired=0;}
            else if(!fired&&HAL_GetTick()-h>2000){
                fired=1;
                ui_state.screen=SCREEN_MENU_SUB;
                ui_state.menu_idx=4;
            }
        } else act=0;
        return;
    }

    // --- ECU Selection Screen Logic ---
    if(ui_state.screen==SCREEN_DIAG_ECU) {
        uint8_t total = KLINE_ECU_COUNT + 1;
        if(trig_up)   ui_state.menu_idx = (ui_state.menu_idx==0) ? total-1 : ui_state.menu_idx-1;
        if(trig_down) ui_state.menu_idx = (ui_state.menu_idx>=total-1) ? 0 : ui_state.menu_idx+1;
        if(Buttons_GetEvent(BTN_OK)) {
            if(ui_state.menu_idx < KLINE_ECU_COUNT) {
                g_diag_addr = KLINE_ECU_LIST[ui_state.menu_idx].addr;
                ui_state.screen = SCREEN_MENU_SUB;
                ui_state.menu_idx = 0;
            } else {
                ui_state.screen = SCREEN_MENU_SUB;
                ui_state.menu_idx = 0;
            }
        }
        return;
    }

    // --- Measuring Blocks Logic ---
    if(ui_state.screen==SCREEN_DIAG_MEAS) {
        if(ui_state.menu_idx == 255) { ui_state.menu_idx = 0; memset(&g_diag_meas, 0, sizeof(g_diag_meas)); g_diag_req_meas = 1;}
        if(trig_up)   g_diag_group = (g_diag_group == 0)   ? 255 : g_diag_group - 1;
        if(trig_down) g_diag_group = (g_diag_group == 255) ? 0   : g_diag_group + 1;
        if(Buttons_GetEvent(BTN_OK)) {
            g_diag_req_meas = 1;
        }
        static uint32_t h=0; static uint8_t act=0, fired=0;
        uint8_t dn = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_2)==0;
        if(dn){
            if(!act){act=1;h=HAL_GetTick();fired=0;}
            else if(!fired&&HAL_GetTick()-h>2000){
                fired=1;
                ui_state.screen=SCREEN_MENU_SUB;
                ui_state.menu_idx=1;
            }
        } else act=0;
        return;
    }

    // --- Read Errors Logic ---
    if(ui_state.screen==SCREEN_DIAG_READ_ERR) {
        if(ui_state.menu_idx == 255) {
            ui_state.menu_idx = 0;
            g_diag_dtc_result = 0;
            g_diag_dtc_busy = 0;
        }

        /* Долгое ВНИЗ 2с = выход (работает в любом состоянии экрана) */
        static uint32_t h=0; static uint8_t act=0, fired=0;
        uint8_t dn = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_2)==0;
        if(dn){
            if(!act){act=1;h=HAL_GetTick();fired=0;}
            else if(!fired&&HAL_GetTick()-h>2000){
                fired=1;
                ui_state.screen=SCREEN_MENU_SUB;
                ui_state.menu_idx=2;
            }
        } else act=0;

        /* Ещё не читали (0) или ЭБУ молчал (2): ОК = ЧИТАТЬ ошибки (0x07) */
        if(g_diag_dtc_result == 0 || g_diag_dtc_result == 2) {
            if(Buttons_GetEvent(BTN_OK) && !g_diag_dtc_busy) {
                g_diag_dtc_busy = 1;
                g_diag_req_dtc  = 1;   // <-- именно чтение, а не стирание
            }
            return;
        }

        /* Список уже загружен: навигация по ошибкам + кнопки Стереть/Назад */
        uint8_t total_items = g_diag_dtcs.count + 2;
        if(trig_up)   ui_state.menu_idx = (ui_state.menu_idx == 0) ? total_items - 1 : ui_state.menu_idx - 1;
        if(trig_down) ui_state.menu_idx = (ui_state.menu_idx >= total_items - 1) ? 0 : ui_state.menu_idx + 1;

        if(Buttons_GetEvent(BTN_OK)) {
            if (ui_state.menu_idx < g_diag_dtcs.count) {
                g_diag_dtc_selected_idx = ui_state.menu_idx;
                ui_state.screen = SCREEN_DIAG_DTC_DETAIL;
                ui_state.menu_idx = 0;
            } else if (ui_state.menu_idx == g_diag_dtcs.count) {
                if(!g_diag_dtc_busy) {
                    g_diag_clr_result = 0;
                    g_diag_dtc_busy = 1;
                    g_diag_req_clr = 1;   // стирание только сознательно, по кнопке "Стереть"
                }
            } else {
                ui_state.screen = SCREEN_MENU_SUB;
                ui_state.menu_idx = 2;
            }
        }
        return;
    }

    // --- DTC Detail Logic ---
    if(ui_state.screen==SCREEN_DIAG_DTC_DETAIL) {
        if(Buttons_GetEvent(BTN_OK)) {
            ui_state.screen = SCREEN_DIAG_READ_ERR;
            ui_state.menu_idx = g_diag_dtc_selected_idx;
        }
        static uint32_t h=0; static uint8_t act=0, fired=0;
        uint8_t dn = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_2)==0;
        if(dn){
            if(!act){act=1;h=HAL_GetTick();fired=0;}
            else if(!fired&&HAL_GetTick()-h>1000){
                fired=1;
                ui_state.screen = SCREEN_DIAG_READ_ERR;
                ui_state.menu_idx = g_diag_dtc_selected_idx;
            }
        } else act=0;
        return;
    }

    // --- Clear Errors Logic ---
    if(ui_state.screen==SCREEN_DIAG_CLEAR_ERR) {
        if(ui_state.menu_idx == 255) { ui_state.menu_idx = 0; g_diag_clr_result = 0; g_diag_dtc_busy = 0; }
        if(Buttons_GetEvent(BTN_OK)) {
            if(!g_diag_dtc_busy) { g_diag_clr_result = 0; g_diag_dtc_busy = 1; g_diag_req_clr = 1; }
        }
        static uint32_t h=0; static uint8_t act=0, fired=0;
        uint8_t dn = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_2)==0;
        if(dn){
            if(!act){act=1;h=HAL_GetTick();fired=0;}
            else if(!fired&&HAL_GetTick()-h>2000){
                fired=1;
                ui_state.screen=SCREEN_MENU_SUB;
                ui_state.menu_idx=3;
            }
        } else act=0;
        return;
    }

    // --- Other Screens (Oil, Accel, Power, Graphs, etc.) ---
    if(ui_state.screen==SCREEN_MEAS_OIL) { if(Buttons_GetEvent(BTN_OK)) { ui_state.screen=SCREEN_MENU_SUB; ui_state.menu_idx=3; } return; }
    if(ui_state.screen==SCREEN_MEAS_ACCEL) { if(Buttons_GetEvent(BTN_OK)) { ui_state.screen=SCREEN_MENU_SUB; ui_state.menu_idx=5; } return; }
    if(ui_state.screen==SCREEN_MEAS_POWER) { if(Buttons_GetEvent(BTN_OK)) { ui_state.screen=SCREEN_MENU_SUB; ui_state.menu_idx=4; } return; }

    if(ui_state.screen==SCREEN_MEAS_GRAPHS) {
        uint8_t total=7;
        if(trig_up)   ui_state.menu_idx=(ui_state.menu_idx==0)?total-1:ui_state.menu_idx-1;
        if(trig_down) ui_state.menu_idx=(ui_state.menu_idx==total-1)?0:ui_state.menu_idx+1;
        if(Buttons_GetEvent(BTN_OK)) {
            if(ui_state.menu_idx<5) {
                g_graph_pick_slot=ui_state.menu_idx;
                ui_state.screen=SCREEN_GRAPHS_PICK;
                ui_state.menu_idx=(g_graph_slots[ui_state.menu_idx]==PARAM_COUNT)?0:(g_graph_slots[ui_state.menu_idx]+1);
            } else if(ui_state.menu_idx==5) { g_graphs_reset=1; g_graphs_paused=0; ui_state.screen=SCREEN_GRAPHS_VIEW; ui_state.menu_idx=0; }
            else { ui_state.screen=SCREEN_MENU_SUB; ui_state.menu_idx=6; }
        }
        return;
    }
    if(ui_state.screen==SCREEN_GRAPHS_PICK) {
        uint8_t total=PARAM_COUNT+1;
        if(trig_up)   ui_state.menu_idx=(ui_state.menu_idx==0)?total-1:ui_state.menu_idx-1;
        if(trig_down) ui_state.menu_idx=(ui_state.menu_idx==total-1)?0:ui_state.menu_idx+1;
        if(Buttons_GetEvent(BTN_OK)) {
            g_graph_slots[g_graph_pick_slot]=(ui_state.menu_idx==0)?PARAM_COUNT:(MfaParam_t)(ui_state.menu_idx-1);
            ui_state.screen=SCREEN_MEAS_GRAPHS; ui_state.menu_idx=g_graph_pick_slot;
        }
        return;
    }
    if(ui_state.screen==SCREEN_GRAPHS_VIEW) {
        if(trig_up)   ui_state.menu_idx=(ui_state.menu_idx==0)?1:0;
        if(trig_down) ui_state.menu_idx=(ui_state.menu_idx==1)?0:1;
        if(Buttons_GetEvent(BTN_OK)) {
            if(ui_state.menu_idx==0) g_graphs_paused=!g_graphs_paused;
            else { ui_state.screen=SCREEN_MEAS_GRAPHS; ui_state.menu_idx=5; }
        }
        return;
    }
    if(ui_state.screen==SCREEN_MAIN) {
        uint8_t top = Msg_TopUnacked();
        if(top){ if(Buttons_GetEvent(BTN_OK)) Msg_Ack(top-1); return; }   /* внимание важнее меню */
        UpdateTripData();
        if(trig_up){ int16_t n=ui_state.main_param,s=n; do{n=(n==0)?PARAM_COUNT-1:n-1;}while(!g_main_param_visible[n]&&n!=s); ui_state.main_param=n; }
        if(trig_down){ int16_t n=ui_state.main_param,s=n; do{n=(n==PARAM_COUNT-1)?0:n+1;}while(!g_main_param_visible[n]&&n!=s); ui_state.main_param=n; }
        if(Buttons_GetEvent(BTN_OK)){ ui_state.screen=SCREEN_MENU_ROOT; ui_state.menu_idx=0; }
        return;
    }
    if(ui_state.screen==SCREEN_WALLPAPER) {
        if(trig_up) ui_state.menu_idx=(ui_state.menu_idx==0)?3:ui_state.menu_idx-1;
        if(trig_down) ui_state.menu_idx=(ui_state.menu_idx==3)?0:ui_state.menu_idx+1;
        if(Buttons_GetEvent(BTN_OK)){ if(ui_state.menu_idx<3) g_current_bg_idx=ui_state.menu_idx; ui_state.screen=graph_menu_parent; ui_state.menu_idx=graph_menu_parent_idx; }
        return;
    }
    if(ui_state.screen==SCREEN_MIDDLE_AREA) {
        uint8_t total=PARAM_COUNT+1;
        if(trig_up) ui_state.menu_idx=(ui_state.menu_idx==0)?total-1:ui_state.menu_idx-1;
        if(trig_down) ui_state.menu_idx=(ui_state.menu_idx==total-1)?0:ui_state.menu_idx+1;
        if(Buttons_GetEvent(BTN_OK)){ if(ui_state.menu_idx<PARAM_COUNT) g_main_param_visible[ui_state.menu_idx]=!g_main_param_visible[ui_state.menu_idx]; else { ui_state.screen=graph_menu_parent; ui_state.menu_idx=graph_menu_parent_idx; } }
        return;
    }
    if(ui_state.screen==SCREEN_COLORS) {
        uint8_t total=8;
        if(trig_up) ui_state.menu_idx=(ui_state.menu_idx==0)?total-1:ui_state.menu_idx-1;
        if(trig_down) ui_state.menu_idx=(ui_state.menu_idx==total-1)?0:ui_state.menu_idx+1;
        static const uint16_t presets[] = {0xF800,0x001F,0x0520,0xFA60,0xF81F,0x0450};
        if(Buttons_GetEvent(BTN_OK)) {
            if(ui_state.menu_idx<6) { g_theme.header_color=g_theme.cursor_color=presets[ui_state.menu_idx]; }
            else if(ui_state.menu_idx==6) { color_menu_parent=SCREEN_COLORS; color_menu_parent_idx=ui_state.menu_idx; ui_state.screen=SCREEN_CUSTOM_COLOR; ui_state.menu_idx=0; cp_state.r=(g_theme.header_color>>11)&0x1F; cp_state.g=(g_theme.header_color>>5)&0x3F; cp_state.b=g_theme.header_color&0x1F; cp_state.focus=0; }
            else { ui_state.screen=graph_menu_parent; ui_state.menu_idx=graph_menu_parent_idx; }
        }
        return;
    }
    if(ui_state.screen==SCREEN_MEAS_BC1||ui_state.screen==SCREEN_MEAS_BC2) { UpdateTripData(); if(Buttons_GetEvent(BTN_OK)) { ui_state.screen=SCREEN_MENU_SUB; ui_state.menu_idx=0; } return; }
    if(ui_state.screen==SCREEN_MEAS_GENERAL) { UpdateTripData(); if(Buttons_GetEvent(BTN_OK)) { ui_state.screen=SCREEN_MENU_SUB; ui_state.menu_idx=2; } return; }
    if(ui_state.screen==SCREEN_CUSTOM_COLOR) {
        if(Buttons_GetEvent(BTN_OK)) {
            if(cp_state.focus==3) { g_theme.header_color=g_theme.cursor_color=((cp_state.r&0x1F)<<11)|((cp_state.g&0x3F)<<5)|(cp_state.b&0x1F); ui_state.screen=color_menu_parent; ui_state.menu_idx=color_menu_parent_idx; }
            else cp_state.focus++;
        }
        if(trig_up||trig_down) {
            if(cp_state.focus<3) {
                uint8_t step=trig_up?1:2;
                if(cp_state.focus==0) cp_state.r=trig_down?(cp_state.r+step>31?31:cp_state.r+step):(cp_state.r<step?0:cp_state.r-step);
                if(cp_state.focus==1) cp_state.g=trig_down?(cp_state.g+step>63?63:cp_state.g+step):(cp_state.g<step?0:cp_state.g-step);
                if(cp_state.focus==2) cp_state.b=trig_down?(cp_state.b+step>31?31:cp_state.b+step):(cp_state.b<step?0:cp_state.b-step);
                g_theme.header_color=g_theme.cursor_color=((cp_state.r&0x1F)<<11)|((cp_state.g&0x3F)<<5)|(cp_state.b&0x1F);
            }
        }
        return;
    }
    if(ui_state.screen==SCREEN_GRAPH_ANIM) {
        if(trig_up) ui_state.menu_idx=(ui_state.menu_idx==0)?2:ui_state.menu_idx-1;
        if(trig_down) ui_state.menu_idx=(ui_state.menu_idx==2)?0:ui_state.menu_idx+1;
        if(Buttons_GetEvent(BTN_OK)){ g_anim_type=ui_state.menu_idx; ui_state.screen=graph_menu_parent; ui_state.menu_idx=graph_menu_parent_idx; }
        return;
    }
    if(ui_state.screen==SCREEN_GRAPH_TOP) {
        uint8_t total=4;
        if(trig_up)   ui_state.menu_idx=(ui_state.menu_idx==0)?total-1:ui_state.menu_idx-1;
        if(trig_down) ui_state.menu_idx=(ui_state.menu_idx==total-1)?0:ui_state.menu_idx+1;
        if(Buttons_GetEvent(BTN_OK)) {
            if(ui_state.menu_idx==0)      { g_top_edit_side=0; ui_state.screen=SCREEN_TOP_PARAMS; ui_state.menu_idx=g_top_left; }
            else if(ui_state.menu_idx==1) { g_top_edit_side=1; ui_state.screen=SCREEN_TOP_PARAMS; ui_state.menu_idx=g_top_right; }
            else if(ui_state.menu_idx==2) { g_top_icons=!g_top_icons; }
            else                          { ui_state.screen=SCREEN_MENU_SUB; ui_state.menu_idx=2; }
        }
        return;
    }
    if(ui_state.screen==SCREEN_TOP_PARAMS) {
        uint8_t total=PARAM_COUNT+1;
        if(trig_up)   ui_state.menu_idx=(ui_state.menu_idx==0)?total-1:ui_state.menu_idx-1;
        if(trig_down) ui_state.menu_idx=(ui_state.menu_idx==total-1)?0:ui_state.menu_idx+1;
        if(Buttons_GetEvent(BTN_OK)) {
            if(ui_state.menu_idx<PARAM_COUNT) {
                if(g_top_edit_side==0) g_top_left  = (MfaParam_t)ui_state.menu_idx;
                else                   g_top_right = (MfaParam_t)ui_state.menu_idx;
            }
            ui_state.screen=SCREEN_GRAPH_TOP; ui_state.menu_idx=g_top_edit_side;
        }
        return;
    }
    if(ui_state.screen==SCREEN_GRAPH_BOTTOM) {
        uint8_t total=6;
        if(trig_up)   ui_state.menu_idx=(ui_state.menu_idx==0)?total-1:ui_state.menu_idx-1;
        if(trig_down) ui_state.menu_idx=(ui_state.menu_idx==total-1)?0:ui_state.menu_idx+1;
        if(Buttons_GetEvent(BTN_OK)) {
            if(ui_state.menu_idx<=3)      { g_bot_edit_slot=ui_state.menu_idx; ui_state.screen=SCREEN_BOT_PARAMS; ui_state.menu_idx=g_bot_slots[ui_state.menu_idx]; }
            else if(ui_state.menu_idx==4) { g_bot_icons=!g_bot_icons; }
            else                          { ui_state.screen=SCREEN_MENU_SUB; ui_state.menu_idx=4; }
        }
        return;
    }
    if(ui_state.screen==SCREEN_BOT_PARAMS) {
        uint8_t total=PARAM_COUNT+1;
        if(trig_up)   ui_state.menu_idx=(ui_state.menu_idx==0)?total-1:ui_state.menu_idx-1;
        if(trig_down) ui_state.menu_idx=(ui_state.menu_idx==total-1)?0:ui_state.menu_idx+1;
        if(Buttons_GetEvent(BTN_OK)) {
            if(ui_state.menu_idx<PARAM_COUNT) g_bot_slots[g_bot_edit_slot]=(MfaParam_t)ui_state.menu_idx;
            ui_state.screen=SCREEN_GRAPH_BOTTOM; ui_state.menu_idx=g_bot_edit_slot;
        }
        return;
    }
    if(ui_state.screen == SCREEN_SERV_CAN_MON_MENU) {
        uint8_t total=3;
        if(trig_up)   ui_state.menu_idx=(ui_state.menu_idx==0)?total-1:ui_state.menu_idx-1;
        if(trig_down) ui_state.menu_idx=(ui_state.menu_idx==total-1)?0:ui_state.menu_idx+1;
        if(Buttons_GetEvent(BTN_OK)) {
            if(ui_state.menu_idx==0)      { g_can_mon_bus_select=0; ui_state.screen=SCREEN_SERV_CAN_MON_DATA; }
            else if(ui_state.menu_idx==1) { g_can_mon_bus_select=1; ui_state.screen=SCREEN_SERV_CAN_MON_DATA; }
            else                          { ui_state.screen=SCREEN_MENU_SUB; ui_state.menu_idx=6; }
        }
        return;
    }
    if(ui_state.screen == SCREEN_SERV_CAN_MON_DATA) {
        if(trig_up || trig_down) g_can_mon_bus_select = !g_can_mon_bus_select;
        if(Buttons_GetEvent(BTN_OK)) { ui_state.screen=SCREEN_SERV_CAN_MON_MENU; ui_state.menu_idx=g_can_mon_bus_select; }
        return;
    }

    // --- Calibration Logic ---
    // --- Calibration Logic ---
    if(ui_state.screen==SCREEN_PARAM_CALIB || ui_state.screen==SCREEN_CALIB_MKPP) {
        static uint8_t edit = 0;
        static uint8_t in_calib = 0;
        if(!in_calib){ in_calib = 1; edit = 0; ui_state.menu_idx = 0; }

        /* Долгое ВНИЗ 2с = выход в меню ПАРАМЕТРЫ */
        static uint32_t h=0; static uint8_t act=0, fired=0;
        uint8_t dn = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_2)==0;
        if(dn){
            if(!act){ act=1; h=HAL_GetTick(); fired=0; }
            else if(!fired && HAL_GetTick()-h>2000){
                fired=1; edit=0; in_calib=0; g_calib_edit=0;
                ui_state.screen=SCREEN_MENU_SUB; ui_state.menu_idx=2;
                return;
            }
        } else act=0;
        if(fired) return;

        int8_t dir = trig_up ? 1 : (trig_down ? -1 : 0);

        if(ui_state.screen==SCREEN_PARAM_CALIB){
            const uint8_t TOTAL=6;
            if(!edit){
                if(trig_up)   ui_state.menu_idx=(ui_state.menu_idx==0)?TOTAL-1:ui_state.menu_idx-1;
                if(trig_down) ui_state.menu_idx=(ui_state.menu_idx>=TOTAL-1)?0:ui_state.menu_idx+1;
                if(Buttons_GetEvent(BTN_OK)){
                    if(ui_state.menu_idx==5){                 /* Назад */
                        in_calib=0; edit=0; g_calib_edit=0;
                        ui_state.screen=SCREEN_MENU_SUB; ui_state.menu_idx=2;
                        return;
                    } else if(ui_state.menu_idx==4){          /* МКПП */
                        ui_state.screen=SCREEN_CALIB_MKPP; ui_state.menu_idx=0;
                        edit=0; g_calib_edit=0;
                        return;
                    } else edit=1;
                }
            } else {
                if(Buttons_GetEvent(BTN_OK)) edit=0;
                if(dir){
                    switch(ui_state.menu_idx){
                    case 0: { float v=g_calib.vbat_offset+dir*0.05f; if(v<-3.0f)v=-3.0f; if(v>3.0f)v=3.0f; g_calib.vbat_offset=v; break; }
                    case 1: g_calib.speed_src^=1; break;
                    case 2: { float v=g_calib.speed_coef+dir*0.005f; if(v<0.5f)v=0.5f; if(v>1.5f)v=1.5f; g_calib.speed_coef=v; break; }
                    case 3: { float v=g_calib.fuel_coef +dir*0.005f; if(v<0.5f)v=0.5f; if(v>1.5f)v=1.5f; g_calib.fuel_coef=v;  break; }
                    }
                }
            }
        } else { /* SCREEN_CALIB_MKPP — else относится к ВНУТРЕННЕМУ if */
            const uint8_t TOTAL=9;
            if(!edit){
                if(trig_up)   ui_state.menu_idx=(ui_state.menu_idx==0)?TOTAL-1:ui_state.menu_idx-1;
                if(trig_down) ui_state.menu_idx=(ui_state.menu_idx>=TOTAL-1)?0:ui_state.menu_idx+1;
                if(Buttons_GetEvent(BTN_OK)){
                    if(ui_state.menu_idx==8){                 /* Назад */
                        ui_state.screen=SCREEN_PARAM_CALIB; ui_state.menu_idx=4;
                        edit=0; g_calib_edit=0;
                        return;
                    } else edit=1;
                }
            } else {
                if(Buttons_GetEvent(BTN_OK)) edit=0;
                if(dir && ui_state.menu_idx<=7){
                    switch(ui_state.menu_idx){
                    case 0: { float v=g_calib.final_drive+dir*0.005f; if(v<2.5f)v=2.5f; if(v>5.5f)v=5.5f; g_calib.final_drive=v; break; }
                    case 1: { int32_t v=(int32_t)g_calib.wheel_mm+dir*5; if(v<1500)v=1500; if(v>2600)v=2600; g_calib.wheel_mm=(uint16_t)v; break; }
                    default:{ uint8_t g=ui_state.menu_idx-2;   /* 2..7 -> 0..5 */
                              float v=g_calib.ratios[g]+dir*0.005f;
                              if(v<0.5f)v=0.5f; if(v>5.0f)v=5.0f;
                              g_calib.ratios[g]=v; break; }
                    }
                }
            }
        }

        g_calib_edit = edit;   /* ВНУТРИ внешнего блока! */
        return;
    }

    // --- Shift Assistant Logic ---
    if(ui_state.screen==SCREEN_PARAM_SHIFT) {
        const uint8_t TOTAL=5;   /* 0=Включено, 1=вверх, 2=вниз, 3=крит, 4=Назад */
        if(!g_shift_edit){
            if(trig_up)   ui_state.menu_idx=(ui_state.menu_idx==0)?TOTAL-1:ui_state.menu_idx-1;
            if(trig_down) ui_state.menu_idx=(ui_state.menu_idx>=TOTAL-1)?0:ui_state.menu_idx+1;
            if(Buttons_GetEvent(BTN_OK)){
                if(ui_state.menu_idx==0)      g_shift_assist_enabled=!g_shift_assist_enabled;
                else if(ui_state.menu_idx==4){ ui_state.screen=SCREEN_MENU_SUB; ui_state.menu_idx=4; }
                else g_shift_edit=1;
            }
        } else {
            if(Buttons_GetEvent(BTN_OK)) g_shift_edit=0;
            int16_t step = trig_up?100:(trig_down?-100:0);
            if(step){
                switch(ui_state.menu_idx){
                case 1: { int32_t v=g_shift_up_rpm+step;   if(v<2000)v=2000; if(v>7000)v=7000; g_shift_up_rpm=(uint16_t)v;   break; }
                case 2: { int32_t v=g_shift_down_rpm+step; if(v<800) v=800;  if(v>3000)v=3000; g_shift_down_rpm=(uint16_t)v; break; }
                case 3: { int32_t v=g_shift_crit_rpm+step; if(v<3000)v=3000; if(v>8000)v=8000; g_shift_crit_rpm=(uint16_t)v; break; }
                }
            }
        }
        return;
    }

    // --- Misc Logic ---
    if(ui_state.screen==SCREEN_PARAM_MISC) {
        static uint8_t entered=0;
        if(!entered){ entered=1; g_misc_edit=0; ui_state.menu_idx=0; }
        static uint32_t h=0; static uint8_t act=0, fired=0;
        uint8_t dn = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_2)==0;
        if(dn){ if(!act){act=1;h=HAL_GetTick();fired=0;}
                else if(!fired&&HAL_GetTick()-h>2000){ fired=1; entered=0; g_misc_edit=0;
                    ui_state.screen=SCREEN_MENU_SUB; ui_state.menu_idx=7; } }
        else act=0;
        if(fired) return;
        const uint8_t TOTAL=8;   /* 0..6 строки, 7 = Назад */
        if(!g_misc_edit){
            if(trig_up)   ui_state.menu_idx=(ui_state.menu_idx==0)?TOTAL-1:ui_state.menu_idx-1;
            if(trig_down) ui_state.menu_idx=(ui_state.menu_idx>=TOTAL-1)?0:ui_state.menu_idx+1;
            if(Buttons_GetEvent(BTN_OK)){
                switch(ui_state.menu_idx){
                case 0: g_misc.menu_to_en^=1; break;
                case 1: g_misc_edit=1; break;
                case 2: g_misc.msg_to_en^=1; break;
                case 3: g_misc_edit=1; break;
                case 4: g_misc.icon_blink^=1; break;
                case 5: g_misc.belt_en^=1; if(!g_misc.belt_en) Msg_Clear(MSG_BELT); break;
                case 6: g_misc.mirror_en^=1; break;
                case 7: entered=0; ui_state.screen=SCREEN_MENU_SUB; ui_state.menu_idx=7; break;
                }
            }
        } else {
            if(Buttons_GetEvent(BTN_OK)) g_misc_edit=0;
            int8_t d = trig_up?1:(trig_down?-1:0);
            if(d){
                if(ui_state.menu_idx==1){ int16_t v=g_misc.menu_to_sec+d; if(v<1)v=1; if(v>60)v=60; g_misc.menu_to_sec=v; }
                if(ui_state.menu_idx==3){ int16_t v=g_misc.msg_to_sec +d; if(v<1)v=1; if(v>30)v=30; g_misc.msg_to_sec=v; }
            }
        }
        return;
    }
    // --- Messages List Logic ---
    if(ui_state.screen==SCREEN_MSG_LIST) {
        uint8_t ids[MSG_COUNT]; uint8_t n=0;
        for(uint8_t i=0;i<MSG_COUNT;i++) if(Msg_IsActive(i)) ids[n++]=i;
        uint8_t TOTAL=n+2;   /* сообщения + Тест + Назад */
        if(trig_up)   ui_state.menu_idx=(ui_state.menu_idx==0)?TOTAL-1:ui_state.menu_idx-1;
        if(trig_down) ui_state.menu_idx=(ui_state.menu_idx>=TOTAL-1)?0:ui_state.menu_idx+1;
        if(Buttons_GetEvent(BTN_OK)){
            if(ui_state.menu_idx<n){ g_msg_view_id=ids[ui_state.menu_idx]; ui_state.screen=SCREEN_MSG_VIEW; }
            else if(ui_state.menu_idx==n){ Msg_Raise(MSG_WASHER); }   /* тестовое сообщение */
            else { ui_state.screen=SCREEN_MENU_ROOT; ui_state.menu_idx=4; }
        }
        return;
    }
    // --- Message View Logic ---
    if(ui_state.screen==SCREEN_MSG_VIEW) {
        if(Buttons_GetEvent(BTN_OK)){ Msg_Ack(g_msg_view_id); ui_state.screen=SCREEN_MSG_LIST; }
        return;
    }

    // --- TPMS Main Logic ---
    if(ui_state.screen==SCREEN_SERV_TPMS) {
      uint8_t total=2;
      if(trig_up)   ui_state.menu_idx=(ui_state.menu_idx==0)?total-1:ui_state.menu_idx-1;
      if(trig_down) ui_state.menu_idx=(ui_state.menu_idx>=total-1)?0:ui_state.menu_idx+1;
      if(Buttons_GetEvent(BTN_OK)){
        if(ui_state.menu_idx==0){ ui_state.screen=SCREEN_TPMS_PARAM; ui_state.menu_idx=0; }
        else { ui_state.screen=SCREEN_MENU_SUB; ui_state.menu_idx=4; }
      }
      return;
    }
    // --- TPMS Param Logic ---
    if(ui_state.screen==SCREEN_TPMS_PARAM) {
      uint8_t total=4;
      if(trig_up)   ui_state.menu_idx=(ui_state.menu_idx==0)?total-1:ui_state.menu_idx-1;
      if(trig_down) ui_state.menu_idx=(ui_state.menu_idx>=total-1)?0:ui_state.menu_idx+1;
      if(Buttons_GetEvent(BTN_OK)){
        switch(ui_state.menu_idx){
        case 0: g_tpms.en=!g_tpms.en;
                if(!g_tpms.en){ Tpms_Close(); g_tpms.state=TPMS_ST_IDLE; }
                else if(g_tpms.calibrated) g_tpms.state=TPMS_ST_MONITOR;
                break;
        case 1: g_tpms_press_mode = 0; g_tpms_press_edit = 0;
                ui_state.screen = SCREEN_TPMS_PRESS; ui_state.menu_idx = 0; break;
        case 2: ui_state.screen = SCREEN_TPMS_WHEEL; ui_state.menu_idx = 0; break;
        case 3: ui_state.screen = SCREEN_SERV_TPMS;  ui_state.menu_idx = 0; break;  /* было: SCREEN_MENU_SUB,4 */
        }
      }
      return;
    }
    // --- TPMS Wheel Logic ---
    if(ui_state.screen==SCREEN_TPMS_WHEEL) {
      uint8_t total=5;
      if(trig_up)   ui_state.menu_idx=(ui_state.menu_idx==0)?total-1:ui_state.menu_idx-1;
      if(trig_down) ui_state.menu_idx=(ui_state.menu_idx>=total-1)?0:ui_state.menu_idx+1;
      if(Buttons_GetEvent(BTN_OK)){
    	  if(ui_state.menu_idx<4){
    	      g_tpms.cal_wheel   = ui_state.menu_idx;
    	      g_tpms_press_mode  = 1; g_tpms_press_edit = 0;
    	      ui_state.screen    = SCREEN_TPMS_PRESS; ui_state.menu_idx = 0;
    	  } else { ui_state.screen=SCREEN_TPMS_PARAM; ui_state.menu_idx=2; }
      }
      return;
    }

    // --- TPMS Press Logic ---
    if(ui_state.screen==SCREEN_TPMS_PRESS) {
        const uint8_t TOTAL=6;   /* 0..3 колёса, 4 = применить, 5 = Назад */
        if(!g_tpms_press_edit){
            if(trig_up)   ui_state.menu_idx=(ui_state.menu_idx==0)?TOTAL-1:ui_state.menu_idx-1;
            if(trig_down) ui_state.menu_idx=(ui_state.menu_idx>=TOTAL-1)?0:ui_state.menu_idx+1;
            if(Buttons_GetEvent(BTN_OK)){
                if(ui_state.menu_idx<4) g_tpms_press_edit=1;
                else if(ui_state.menu_idx==4){
                    /* Применить: давление уже в g_tpms.p_ref[], старт обучения */
                    if(g_tpms_press_mode==0){
                        g_tpms.state=TPMS_ST_CAL_FULL; g_tpms.samples=0;
                        for(uint8_t i=0;i<4;i++) g_tpms.dev_avg[i]=0;
                    } else {
                        g_tpms.state=TPMS_ST_CAL_WHEEL; g_tpms.samples=0;
                        g_tpms.dev_avg[g_tpms.cal_wheel]=0;
                    }
                    g_tpms_press_edit=0;
                    ui_state.screen=SCREEN_SERV_TPMS; ui_state.menu_idx=0;
                }
                else {  /* Назад */
                    g_tpms_press_edit=0;
                    if(g_tpms_press_mode==0){ ui_state.screen=SCREEN_TPMS_PARAM; ui_state.menu_idx=1; }
                    else                    { ui_state.screen=SCREEN_TPMS_WHEEL; ui_state.menu_idx=0; }
                }
            }
        } else {
            if(Buttons_GetEvent(BTN_OK)) g_tpms_press_edit=0;
            int8_t d = trig_up?1:(trig_down?-1:0);
            if(d && ui_state.menu_idx<4){
                float v = g_tpms.p_ref[ui_state.menu_idx] + d*0.1f;
                if(v<1.0f)v=1.0f; if(v>3.5f)v=3.5f;
                g_tpms.p_ref[ui_state.menu_idx]=v;
            }
        }
        return;
    }




    // --- Main Menu Navigation ---
    const char** list; uint8_t count; GetCurrentMenu(&list,&count);
    if(count==0) count=1;
    if(trig_up) ui_state.menu_idx=(ui_state.menu_idx==0)?count-1:ui_state.menu_idx-1;
    if(trig_down) ui_state.menu_idx=(ui_state.menu_idx>=count-1)?0:ui_state.menu_idx+1;
    if(Buttons_GetEvent(BTN_OK)) {
    	if(ui_state.screen==SCREEN_MENU_ROOT) {
    	    if(ui_state.menu_idx==6)      { ui_state.screen=SCREEN_MAIN;     ui_state.menu_idx=0; }
    	    else if(ui_state.menu_idx==4) { ui_state.screen=SCREEN_MSG_LIST; ui_state.menu_idx=0; }
    	    else {
    	        /* 0..3 -> , 5 (Служебные) -> SUB_SERVICE=4 */
    	        ui_state.active_submenu = (ui_state.menu_idx < 4) ? ui_state.menu_idx : (ui_state.menu_idx - 1);
    	        ui_state.screen=SCREEN_MENU_SUB; ui_state.menu_idx=0;
    	    }
    	} else if(ui_state.screen==SCREEN_MENU_SUB) {
            if(ui_state.menu_idx==count-1){ ui_state.screen=SCREEN_MENU_ROOT; ui_state.menu_idx=0; }
            else {
                ScreenId_t target=SCREEN_MAIN;
                switch(ui_state.active_submenu) {
                    case SUB_MEAS: { static const ScreenId_t m[]={SCREEN_MEAS_BC1,SCREEN_MEAS_BC2,SCREEN_MEAS_GENERAL,SCREEN_MEAS_OIL,SCREEN_MEAS_POWER,SCREEN_MEAS_ACCEL,SCREEN_MEAS_GRAPHS}; if(ui_state.menu_idx<sizeof(m)/sizeof(m[0])) target=m[ui_state.menu_idx]; break; }
                    case SUB_PARAM: { static const ScreenId_t m[]={SCREEN_PARAM_SPEED_LIM,SCREEN_PARAM_ECO,SCREEN_PARAM_CALIB,SCREEN_PARAM_GEARBOX,SCREEN_PARAM_SHIFT,SCREEN_PARAM_NAV,SCREEN_PARAM_COFFEE,SCREEN_PARAM_MISC}; if(ui_state.menu_idx<sizeof(m)/sizeof(m[0])) target=m[ui_state.menu_idx]; break; }
                    case SUB_GRAPH: { static const ScreenId_t m[]={SCREEN_WALLPAPER,SCREEN_COLORS,SCREEN_GRAPH_TOP,SCREEN_MIDDLE_AREA,SCREEN_GRAPH_BOTTOM,SCREEN_GRAPH_OK,SCREEN_GRAPH_SCREENSAVER,SCREEN_GRAPH_ANIM}; if(ui_state.menu_idx<sizeof(m)/sizeof(m[0])) target=m[ui_state.menu_idx]; graph_menu_parent=SCREEN_MENU_SUB; graph_menu_parent_idx=ui_state.menu_idx; break; }
                    case SUB_DIAG: {
                        static const ScreenId_t m[]={SCREEN_DIAG_ECU, SCREEN_DIAG_MEAS, SCREEN_DIAG_READ_ERR, SCREEN_DIAG_CLEAR_ERR, SCREEN_DIAG_KLINE};
                        if(ui_state.menu_idx < 5) target=m[ui_state.menu_idx];
                        break;
                    }
                    case SUB_SERVICE: { static const ScreenId_t m[]={SCREEN_SERV_LANG,SCREEN_SERV_UNITS,SCREEN_SERV_LIGHT,SCREEN_SERV_COMFORT,SCREEN_SERV_TPMS,SCREEN_SERV_PNEUMO,SCREEN_SERV_CAN_MON_MENU,SCREEN_SERV_SS,SCREEN_SERV_MOTOR,SCREEN_SERV_FW,SCREEN_SERV_DEBUG}; if(ui_state.menu_idx<sizeof(m)/sizeof(m[0])) target=m[ui_state.menu_idx]; break; }
                    default: break;
                }
                ui_state.screen=target; ui_state.menu_idx=0;
                if(target == SCREEN_PARAM_SHIFT) g_shift_edit = 0;
                if(target == SCREEN_DIAG_MEAS || target == SCREEN_DIAG_READ_ERR || target == SCREEN_DIAG_CLEAR_ERR) {
                    ui_state.menu_idx = 255;
                }
            }
        }
    }
}

void UI_Render(void) {
    switch(ui_state.screen) {
        case SCREEN_MAIN: UI_DrawMain(&ui_state, param_values); break;
        case SCREEN_MENU_ROOT: UI_DrawMenuList("ГЛАВНОЕ МЕНЮ", MENU_ROOT, 7, ui_state.menu_idx); break;
        case SCREEN_MENU_SUB: { const char** l; uint8_t c; GetCurrentMenu(&l,&c); UI_DrawMenuList(SUB_TITLES[ui_state.active_submenu], l, c, ui_state.menu_idx); break; }
        case SCREEN_WALLPAPER: UI_DrawWallpaper(&ui_state, g_current_bg_idx); break;
        case SCREEN_MIDDLE_AREA: UI_DrawMiddleArea(&ui_state, g_main_param_visible); break;
        case SCREEN_COLORS: UI_DrawColors(&ui_state); break;
        case SCREEN_CUSTOM_COLOR: UI_DrawCustomColor(&ui_state, &cp_state); break;
        case SCREEN_MEAS_BC1: UI_DrawMeas_Bc1(param_values); break;
        case SCREEN_MEAS_BC2: UI_DrawMeas_Bc2(param_values); break;
        case SCREEN_MEAS_GENERAL: UI_DrawMeas_General(param_values); break;
        case SCREEN_MEAS_EXTRA: UI_DrawMeas_Extra(); break;
        case SCREEN_MEAS_OIL: UI_DrawMeas_Oil(); break;
        case SCREEN_MEAS_POWER: UI_DrawMeas_Power(); break;
        case SCREEN_MEAS_ACCEL: UI_DrawMeas_Accel(); break;
        case SCREEN_MEAS_GRAPHS:  UI_DrawGraphsSettings(&ui_state); break;
        case SCREEN_GRAPHS_PICK:  UI_DrawGraphsPick(&ui_state); break;
        case SCREEN_GRAPHS_VIEW:  UI_DrawGraphsView(&ui_state); break;
        case SCREEN_MEAS_GAUGES: UI_DrawMeas_Gauges(); break;
        case SCREEN_PARAM_SPEED_LIM: UI_DrawParam_SpeedLim(); break;
        case SCREEN_PARAM_ECO: UI_DrawParam_Eco(); break;
        case SCREEN_PARAM_CALIB: UI_DrawParam_Calib(); break;
        case SCREEN_PARAM_GEARBOX: UI_DrawParam_Gearbox(); break;
        case SCREEN_PARAM_SHIFT: UI_DrawParam_Shift(); break;
        case SCREEN_PARAM_NAV: UI_DrawParam_Nav(); break;
        case SCREEN_PARAM_COFFEE: UI_DrawParam_Coffee(); break;
        case SCREEN_PARAM_MISC: UI_DrawParam_Misc(); break;
        case SCREEN_CALIB_MKPP: UI_DrawParam_CalibMkpp(); break;
        case SCREEN_GRAPH_TOP:   UI_DrawGraph_TopSettings(&ui_state); break;
        case SCREEN_TOP_PARAMS:  UI_DrawTopParams(&ui_state); break;
        case SCREEN_GRAPH_BOTTOM: UI_DrawGraph_BottomSettings(&ui_state); break;
        case SCREEN_BOT_PARAMS:   UI_DrawBotParams(&ui_state); break;
        case SCREEN_GRAPH_SCREENSAVER: UI_DrawGraph_Screensaver(); break;
        case SCREEN_GRAPH_ANIM: UI_DrawGraph_Anim(&ui_state); break;
        case SCREEN_DIAG_ECU: UI_DrawDiag_Ecu(); break;
        case SCREEN_DIAG_MEAS: UI_DrawDiag_Meas(); break;
        case SCREEN_DIAG_READ_ERR: UI_DrawDiag_ReadErr(); break;
        case SCREEN_DIAG_DTC_DETAIL: UI_DrawDiag_DtcDetail(); break;
        case SCREEN_DIAG_CLEAR_ERR: UI_DrawDiag_ClearErr(); break;
        case SCREEN_DIAG_KLINE: UI_DrawDiag_Kline(); break;
        case SCREEN_SERV_LANG: UI_DrawServ_Lang(); break;
        case SCREEN_SERV_UNITS: UI_DrawServ_Units(); break;
        case SCREEN_SERV_LIGHT: UI_DrawServ_Light(); break;
        case SCREEN_SERV_COMFORT: UI_DrawServ_Comfort(); break;
        case SCREEN_SERV_TPMS: UI_DrawServ_Tpms(); break;
        case SCREEN_TPMS_PRESS: UI_DrawTpms_Press(); break;
        case SCREEN_TPMS_PARAM: UI_DrawTpms_Param(); break;
        case SCREEN_TPMS_WHEEL: UI_DrawTpms_Wheel(); break;
        case SCREEN_SERV_PNEUMO: UI_DrawServ_Pneumo(); break;
        case SCREEN_SERV_CAN_MON_MENU: UI_DrawServ_CanMonMenu(); break;
        case SCREEN_SERV_CAN_MON_DATA: UI_DrawServ_CanMonData(); break;
        case SCREEN_SERV_SS: UI_DrawServ_Ss(); break;
        case SCREEN_SERV_MOTOR: UI_DrawServ_Motor(); break;
        case SCREEN_SERV_FW: UI_DrawServ_Fw(); break;
        case SCREEN_SERV_DEBUG: UI_DrawServ_Debug(); break;
        case SCREEN_MSG_LIST: UI_DrawMsgList(); break;
        case SCREEN_MSG_VIEW: UI_DrawMsgView(); break;
        default: UI_DrawMain(&ui_state, param_values); break;
    }
    ILI9481_FlushBuffer();
}
