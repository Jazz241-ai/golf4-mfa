#include "ui_screens.h"
#include "ili9481.h"
#include "can_dual.h"
#include "icons.h"
#include <stdio.h>
#include "kline.h"
#include "buttons.h"

static void DrawCheckbox(uint16_t x, uint16_t y, uint8_t checked);
static void DrawShiftArrow(uint16_t cx, uint16_t cy, uint8_t up, uint16_t color);   /* <-- добавить */
static uint16_t SevColor(uint8_t sev){ return (sev==MSG_SEV_CRIT)?COLOR_RED:(sev==MSG_SEV_WARN)?COLOR_ORANGE:COLOR_YELLOW; }
static void DrawMsgFull(uint8_t id, uint8_t from_menu);   /* было: (uint8_t id) */



static int16_t fi_cx, fi_cy; static uint8_t fi_s; static uint16_t fi_col;
static void fi_line(int8_t a,int8_t b,int8_t c,int8_t d){ ILI9481_DrawLine(fi_cx+a*fi_s,fi_cy+b*fi_s,fi_cx+c*fi_s,fi_cy+d*fi_s,fi_col); }
static void fi_rect(int8_t x,int8_t y,int8_t w,int8_t h){ ILI9481_DrawRect(fi_cx+x*fi_s,fi_cy+y*fi_s,(uint16_t)w*fi_s,(uint16_t)h*fi_s,fi_col); }
static void fi_circ(int8_t dx,int8_t dy,int8_t r){ ILI9481_DrawCircle(fi_cx+dx*fi_s,fi_cy+dy*fi_s,(uint16_t)r*fi_s,fi_col); }
static void fi_dot(int8_t x,int8_t y){ ILI9481_FillRect(fi_cx+x*fi_s,fi_cy+y*fi_s,fi_s,fi_s,fi_col); }
static void fi_txt(const char* t){
    uint8_t w=(fi_s==1)?FONT_16_CHAR_WIDTH:FONT_20_CHAR_WIDTH, h=(fi_s==1)?FONT_16_CHAR_HEIGHT:FONT_20_CHAR_HEIGHT;
    ILI9481_DrawString_QSPI(fi_cx-((int16_t)strlen(t)*w)/2, fi_cy-h/2, t, fi_col, COLOR_TRANSPARENT,
                            (fi_s==1)?FONT_16_ADDR:FONT_20_ADDR, w, h);
}
static void DrawFaultIcon(int16_t cx, int16_t cy, uint8_t icon, uint16_t color, uint8_t scale){
    fi_cx=cx; fi_cy=cy; fi_s=scale; fi_col=color;
    switch(icon){
    case FICON_OIL:   fi_rect(-6,-2,11,8); fi_line(-6,0,-11,-4); fi_line(-2,-2,-2,-5); fi_line(1,-2,4,-5); break;
    case FICON_TEMP:  fi_line(0,-9,0,4); fi_circ(0,6,3); fi_line(3,-6,6,-6); fi_line(3,-2,6,-2); fi_line(3,2,6,2); break;
    case FICON_BATT:  fi_rect(-9,-4,18,10); fi_dot(-7,-6); fi_dot(-6,-6); fi_dot(5,-6); fi_dot(6,-6);
                      fi_line(-6,0,-2,0); fi_line(3,-2,3,2); fi_line(1,0,5,0); break;
    case FICON_BRAKE: fi_circ(0,0,7); fi_line(-10,-6,-8,0); fi_line(-8,0,-10,6); fi_line(10,-6,8,0); fi_line(8,0,10,6);
                      fi_line(0,-4,0,1); fi_dot(0,4); break;
    case FICON_ABS:   fi_circ(0,0,9); fi_txt("ABS"); break;
    case FICON_ESP:   fi_circ(0,0,9); fi_txt("ESP"); break;
    case FICON_AIRBAG:fi_circ(-3,0,7); fi_circ(6,-5,2); fi_line(6,-2,3,6); fi_line(5,-1,0,3); break;
    case FICON_ENGINE:fi_rect(-8,-3,16,9); fi_rect(-3,-7,6,4); fi_dot(-11,0); fi_dot(-10,0); fi_dot(9,0); fi_dot(10,0); break;
    case FICON_TIRE:  fi_circ(0,0,8); fi_line(-5,-7,-6,-10); fi_line(0,-8,0,-11); fi_line(5,-7,6,-10);
                      fi_line(0,-4,0,1); fi_dot(0,4); break;
    case FICON_LIGHT: fi_circ(3,0,6); fi_line(-3,-6,-3,6); fi_line(-6,-4,-10,-6); fi_line(-6,0,-11,0); fi_line(-6,4,-10,6);
                      fi_line(3,-3,3,1); fi_dot(3,3); break;
    case FICON_BELT:  fi_circ(2,-6,2); fi_line(2,-3,0,5); fi_line(4,-2,-2,3); fi_line(0,5,5,5); break;
    case FICON_WASHER:fi_rect(-9,-6,10,12); fi_dot(3,-4); fi_dot(5,-1); fi_dot(3,2); fi_line(7,-5,9,-6); fi_line(8,0,10,0); fi_line(7,4,9,5); break;
    case FICON_STEER: fi_circ(0,0,8); fi_circ(0,0,2); fi_line(-8,0,-2,0); fi_line(2,0,8,0); fi_line(0,2,0,8); break;
    case FICON_AWD:   fi_circ(-6,-5,2); fi_circ(6,-5,2); fi_circ(-6,5,2); fi_circ(6,5,2); fi_line(-6,-5,6,-5); fi_line(-6,5,6,5); fi_line(0,-5,0,5); break;
    case FICON_LEVEL: fi_rect(-7,-1,14,6); fi_rect(-3,-5,8,4);
                      fi_line(-11,-6,-11,-11); fi_line(-12,-9,-11,-11); fi_line(-10,-9,-11,-11);
                      fi_line(11,-11,11,-6); fi_line(10,-8,11,-6); fi_line(12,-8,11,-6); break;
    case FICON_EPB:   fi_circ(0,0,7); fi_line(-10,-6,-8,0); fi_line(-8,0,-10,6); fi_line(10,-6,8,0); fi_line(8,0,10,6); fi_txt("P"); break;
    case FICON_GEAR:  fi_circ(0,0,8); fi_line(8,0,10,0); fi_line(-8,0,-10,0); fi_line(0,8,0,10); fi_line(0,-8,0,-10);
                      fi_line(6,6,7,7); fi_line(-6,-6,-7,-7); fi_line(6,-6,7,-7); fi_line(-6,6,-7,7);
                      fi_line(0,-4,0,1); fi_dot(0,4); break;
    case FICON_FUEL:  fi_rect(-8,-6,9,13); fi_line(1,-2,6,-2); fi_line(6,-2,6,5); fi_line(4,-6,6,-4); fi_rect(-6,-4,5,4); break;
    default:          fi_line(-9,7,0,-8); fi_line(0,-8,9,7); fi_line(9,7,-9,7); fi_line(0,-3,0,2); fi_dot(0,5); break;
    }
}
/* Иконка предупреждения: круг с "!" (big = крупная для оверлея) */
static void DrawWarnIcon(uint16_t cx, uint16_t cy, uint16_t color, uint8_t big){
    uint8_t r = big ? 28 : 10;
    ILI9481_DrawCircle(cx, cy, r, color);
    if(big) ILI9481_DrawString_QSPI(cx - FONT_40_CHAR_WIDTH/2 + 2, cy - FONT_40_CHAR_HEIGHT/2,
                                    "!", color, COLOR_TRANSPARENT,
                                    FONT_40_ADDR, FONT_40_CHAR_WIDTH, FONT_40_CHAR_HEIGHT);
    else    ILI9481_DrawString_QSPI(cx - FONT_16_CHAR_WIDTH/2 + 1, cy - FONT_16_CHAR_HEIGHT/2,
                                    "!", color, COLOR_TRANSPARENT,
                                    FONT_16_ADDR, FONT_16_CHAR_WIDTH, FONT_16_CHAR_HEIGHT);
}





extern Theme_t g_theme;
extern uint8_t g_current_bg_idx;
extern UiState_t ui_state;
extern uint8_t g_diag_addr; // Access to selected ECU address

/* ============================================================================
   TABLES
   ============================================================================ */
static const IconId_t PARAM_ICONS[PARAM_COUNT] = {
    ICON_SPEED, ICON_RPM, ICON_THERMO, ICON_DROP, ICON_DROP,
    ICON_DIST,  ICON_FUEL, ICON_DIST, ICON_SPEED, ICON_THERMO,
    ICON_THERMO, ICON_THERMO, ICON_BATTERY, ICON_SPEED, ICON_DIST
};
static const char* PARAM_LABELS[PARAM_COUNT] = {
    "Скорость","Обороты двигателя","Темп. двиг.","Текущий расход",
    "Средний расход","Пройденное расст.","Остаток топлива","Запас хода",
    "Время в пути","Темп. масла","Темп. на улице","Темп. на впуске",
    "Напряж. борт.сети","Угол поворота руля","Общий пробег"
};
static const char* PARAM_UNITS[PARAM_COUNT] = {
    "км/ч","об.","°C","л/100",
    "л/100","км","л","км",
    "ч:м","°C","°C","°C",
    "В","°","км"
};

static const char* UnitFor(MfaParam_t p, const float* vals){
    if(p == PARAM_CONSUMPTION_INSTANT)
        return (vals[PARAM_SPEED] > 2.0f) ? "л/100" : "л/ч";
    return PARAM_UNITS[p];
}

/* ============================================================================
   HELPERS
   ============================================================================ */
static void DrawMainSet(MfaParam_t p, const char* lbl, const char* val, const char* unt,
                        int16_t off, uint16_t color) {
    int16_t xl, xv, xu, yv, yu; uint32_t va, ua; uint8_t vw_f, vh_f, uw, uh;
    if(p == PARAM_ODOMETER) {
        va = FONT_60_ADDR; vw_f = FONT_60_CHAR_WIDTH; vh_f = FONT_60_CHAR_HEIGHT;
        yv = 180 + FONT_70_CHAR_HEIGHT - FONT_60_CHAR_HEIGHT;
        int16_t vw = (int16_t)strlen(val) * vw_f;
        int16_t uw20 = (int16_t)strlen(unt) * FONT_20_CHAR_WIDTH;
        const int16_t W = ILI9481_WIDTH, GAP = 8, M = 4;
        if(vw + GAP + uw20 <= W - 2*M) {
            ua = FONT_20_ADDR; uw = FONT_20_CHAR_WIDTH; uh = FONT_20_CHAR_HEIGHT;
            int16_t x0 = (W - (vw + GAP + uw20)) / 2;
            xv = x0 + off; xu = x0 + vw + GAP + off;
        } else {
            ua = FONT_16_ADDR; uw = FONT_16_CHAR_WIDTH; uh = FONT_16_CHAR_HEIGHT;
            int16_t uw16 = (int16_t)strlen(unt) * FONT_16_CHAR_WIDTH;
            xv = M + off; xu = xv + vw + 2;
            if(xu + uw16 > W) xu = W - uw16;
        }
        xl = (W - (int16_t)strlen(lbl) * FONT_20_CHAR_WIDTH) / 2 + off;
        yu = yv + (int16_t)vh_f - (int16_t)uh - 6;
    } else {
        va = FONT_70_ADDR; vw_f = FONT_70_CHAR_WIDTH; vh_f = FONT_70_CHAR_HEIGHT;
        xl = 150 - ((int16_t)strlen(lbl) / 2) * FONT_20_CHAR_WIDTH + off;
        xv = 220 - (int16_t)strlen(val) * FONT_70_CHAR_WIDTH + off;
        xu = 230 + off;
        ua = FONT_20_ADDR; uw = FONT_20_CHAR_WIDTH; uh = FONT_20_CHAR_HEIGHT;
        yv = 180; yu = 220;
    }
    ILI9481_DrawString_QSPI(xl,120,lbl,color,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
    ILI9481_DrawString_QSPI(xv,yv,val,color,COLOR_TRANSPARENT,va,vw_f,vh_f);
    ILI9481_DrawString_QSPI(xu,yu,unt,color,COLOR_TRANSPARENT,ua,uw,uh);
}

static void DrawBg(uint8_t idx){
    ILI9481_DrawImage_QSPI(0,0,320,480,(uint32_t[]){0x900D61C0U,0x901211D0U,0x9016C1E0U}[idx]);
}

static void DrawSlot(uint16_t x, uint16_t y, MfaParam_t idx, const float* vals, uint8_t mirror, uint8_t icons){
    char buf[32];
    snprintf(buf,sizeof(buf), (idx==PARAM_ODOMETER) ? "%.0f" : "%.1f", vals[idx]);
    const char* unt = UnitFor(idx, vals);
    const uint8_t GAP = 4;
    uint16_t vw = ILI9481_StringWidth_QSPI_Pro(buf, FONT_20_ADDR, FONT_20_CHAR_WIDTH, FONT_20_CHAR_HEIGHT, 2);
    uint16_t uw = ILI9481_StringWidth_QSPI_Pro(unt, FONT_16_ADDR, FONT_16_CHAR_WIDTH, FONT_16_CHAR_HEIGHT, 2);
    if(!mirror){
        if(icons) Icon_Draw(x+2, y-5, PARAM_ICONS[idx], COLOR_CYAN);
        int16_t xv = icons ? x+38 : x+4;
        ILI9481_DrawString_QSPI_Pro(xv, y+4, buf, COLOR_WHITE, COLOR_TRANSPARENT, FONT_20_ADDR, FONT_20_CHAR_WIDTH, FONT_20_CHAR_HEIGHT, 2);
        int16_t xu = xv + vw + GAP;
        if(xu + uw > ILI9481_WIDTH - 2) xu = ILI9481_WIDTH - 2 - uw;
        ILI9481_DrawString_QSPI_Pro(xu, y+7, unt, COLOR_WHITE, COLOR_TRANSPARENT, FONT_16_ADDR, FONT_16_CHAR_WIDTH, FONT_16_CHAR_HEIGHT, 2);
    } else {
        if(icons) Icon_Draw(x+93, y-5, PARAM_ICONS[idx], COLOR_CYAN);
        uint16_t xs = (icons ? x+90 : x+125) - (vw + GAP + uw);
        ILI9481_DrawString_QSPI_Pro(xs, y+4, buf, COLOR_WHITE, COLOR_TRANSPARENT, FONT_20_ADDR, FONT_20_CHAR_WIDTH, FONT_20_CHAR_HEIGHT, 2);
        ILI9481_DrawString_QSPI_Pro(xs + vw + GAP, y+7, unt, COLOR_WHITE, COLOR_TRANSPARENT, FONT_16_ADDR, FONT_16_CHAR_WIDTH, FONT_16_CHAR_HEIGHT, 2);
    }
}

static void DrawPlaceholder(const char* title){
    DrawBg(g_current_bg_idx); ILI9481_FillRect(0,0,ILI9481_WIDTH,40,g_theme.header_color);
    ILI9481_DrawString_QSPI(40,12,title,COLOR_WHITE,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
    ILI9481_DrawString_QSPI(40,200,"TODO: Ваша графика здесь",COLOR_WHITE,COLOR_TRANSPARENT,FONT_16_ADDR,FONT_16_CHAR_WIDTH,FONT_16_CHAR_HEIGHT);
}

static void DrawGearIndicator(uint16_t cx, uint16_t cy) {
    const uint16_t BW = 44, BH = 44;
    int16_t x = cx - BW/2, y = cy - BH/2;
    const char* gear;
    switch(g_motor_can.gear_position) {
        case 1:  gear = "P"; break; case 2:  gear = "R"; break; case 3:  gear = "N"; break;
        case 4:  gear = "D"; break; case 5:  gear = "S"; break; case 6:  gear = "1"; break;
        case 7:  gear = "2"; break; case 8:  gear = "3"; break; case 9:  gear = "4"; break;
        case 10: gear = "5"; break; case 11: gear = "6"; break; default: gear = "N"; break;
    }
    ILI9481_FillRect(x, y, BW, BH, 0x1082);
    ILI9481_DrawRect(x,   y,   BW,   BH,   g_motor_can.handbrake ? COLOR_RED : COLOR_GRAY);
    ILI9481_DrawRect(x+1, y+1, BW-2, BH-2, COLOR_WHITE);
    ILI9481_DrawString_QSPI(cx - FONT_40_CHAR_WIDTH/2 + 3, cy - FONT_40_CHAR_HEIGHT/2 +4,
                            gear, COLOR_WHITE, COLOR_TRANSPARENT,
                            FONT_40_ADDR, FONT_40_CHAR_WIDTH, FONT_40_CHAR_HEIGHT);
    if(g_motor_can.handbrake) {
        ILI9481_DrawString_QSPI(cx - 20, y - 20, "(P)", COLOR_RED, COLOR_TRANSPARENT,
                                FONT_16_ADDR, FONT_16_CHAR_WIDTH, FONT_16_CHAR_HEIGHT);
    }
}

/* Центрирование строки по горизонтали для моноширинного шрифта */
static int16_t CenterX(const char *s, uint8_t char_width) {
    int16_t w = (int16_t)strlen(s) * (int16_t)char_width;
    return (w >= ILI9481_WIDTH) ? 0 : (ILI9481_WIDTH - w) / 2;
}
/* ============================================================================
   MAIN SCREEN
   ============================================================================ */
void UI_DrawMain(const UiState_t* state, const float* param_vals){
    DrawBg(g_current_bg_idx); ILI9481_FillRect(0,0,ILI9481_WIDTH,35,g_theme.header_color);
    extern uint8_t g_anim_type;
    MfaParam_t p = state->main_param; char buf[32];
    snprintf(buf,sizeof(buf),"%.0f",param_vals[p]);

    if(g_anim_type == ANIM_NONE) {
        DrawMainSet(p, PARAM_LABELS[p], buf, UnitFor(p,param_vals), 0, COLOR_WHITE);
    } else if(g_anim_type == ANIM_SLIDE) {
        static MfaParam_t anim_curr=PARAM_COUNT, anim_prev=PARAM_COUNT;
        static int8_t anim_dir=0; static uint8_t anim_step=0;
        const uint8_t ANIM_FRAMES=30; const int16_t SLIDE_DIST=260;
        if(p!=anim_curr){ anim_prev=anim_curr; anim_curr=p; uint8_t d=(uint8_t)((anim_curr-anim_prev+PARAM_COUNT)%PARAM_COUNT); anim_dir=(d<=PARAM_COUNT/2)?1:-1; anim_step=ANIM_FRAMES; }
        float t=(anim_step>0)?(1.0f-(float)anim_step/ANIM_FRAMES):1.0f; t=1.0f-(1.0f-t)*(1.0f-t)*(1.0f-t); if(anim_step>0) anim_step--;
        int16_t on=(int16_t)(SLIDE_DIST*(1.0f-t)*anim_dir), oo=(int16_t)(SLIDE_DIST*t*(-anim_dir));
        if(anim_step>0 && anim_prev!=PARAM_COUNT && anim_prev!=anim_curr){
            char bo[32]; MfaParam_t po=anim_prev;
            snprintf(bo,sizeof(bo),"%.0f",param_vals[po]);
            DrawMainSet(po, PARAM_LABELS[po], bo, UnitFor(po,param_vals), oo, COLOR_GRAY);
        }
        DrawMainSet(p, PARAM_LABELS[p], buf, UnitFor(p,param_vals), on, COLOR_WHITE);
    } else if(g_anim_type == ANIM_FADE) {
        static MfaParam_t fade_curr=PARAM_COUNT, fade_prev=PARAM_COUNT; static uint8_t fade_step=0;
        const uint8_t FADE_FRAMES = 20;
        if(p != fade_curr){ fade_prev = fade_curr; fade_curr = p; fade_step = FADE_FRAMES; }
        float t = (fade_step > 0) ? (1.0f - (float)fade_step / FADE_FRAMES) : 1.0f;
        t = t * t * (3.0f - 2.0f * t); if(fade_step > 0) fade_step--;
        uint8_t br = 0; const char* lbl = ""; const char* val = ""; const char* unt = ""; char buf_fade[32] = {0};
        MfaParam_t draw_p = p;
        if(t < 0.5f && fade_prev != PARAM_COUNT && fade_prev != fade_curr) {
            br = (uint8_t)((1.0f - t * 2.0f) * 255.0f); lbl = PARAM_LABELS[fade_prev]; unt = UnitFor(fade_prev,param_vals); draw_p = fade_prev;
            snprintf(buf_fade,sizeof(buf_fade),"%.0f",param_vals[fade_prev]);
            val = buf_fade;
        } else { br = (uint8_t)((t - 0.5f) * 2.0f * 255.0f); lbl = PARAM_LABELS[p]; val = buf; unt = UnitFor(p,param_vals); }
        if(br < 10) br = 0;
        uint16_t c_fade = (uint16_t)(((br>>3)<<11) | ((br>>2)<<5) | (br>>3));
        if(br > 0) DrawMainSet(draw_p, lbl, val, unt, 0, c_fade);
        if(fade_step == 0) DrawMainSet(p, PARAM_LABELS[p], buf, UnitFor(p,param_vals), 0, COLOR_WHITE);
    }

    ILI9481_DrawHLine(20,350,280,g_theme.header_color);
    DrawSlot(0,   7, g_top_left,  param_vals, 0, g_top_icons);
    DrawSlot(190, 7, g_top_right, param_vals, 1, g_top_icons);
    DrawSlot(0,   370, g_bot_slots[0], param_vals, 0, g_bot_icons);
    DrawSlot(190, 370, g_bot_slots[1], param_vals, 1, g_bot_icons);
    DrawSlot(0,   425, g_bot_slots[2], param_vals, 0, g_bot_icons);
    DrawSlot(190, 425, g_bot_slots[3], param_vals, 1, g_bot_icons);
    DrawGearIndicator(160, 420);

    /* Область ОК: пиктограммы активных неисправностей либо подсказка ОК */
    {
        uint8_t ids[MSG_COUNT], cnt=0;
        for(uint8_t i=0;i<MSG_COUNT;i++) if(Msg_IsActive(i)) ids[cnt++]=i;
        if(cnt==0){
            ILI9481_DrawString_QSPI(CenterX("ОК",FONT_20_CHAR_WIDTH),318,"ОК",COLOR_GRAY,COLOR_TRANSPARENT,
                                    FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
        } else if(!g_misc.icon_blink || ((HAL_GetTick()/500)&1)){
            if(cnt>6) cnt=6;
            int16_t x0 = 160 - (int16_t)(cnt*44)/2 + 22;
            for(uint8_t k=0;k<cnt;k++)
                DrawFaultIcon(x0 + (int16_t)k*44, 322, Msg_Icon(ids[k]), SevColor(Msg_Sev(ids[k])), 1);
        }
    }
    /* полноэкранный оверлей непри подтвержденного сообщения */
    { uint8_t top = Msg_TopUnacked(); if(top) DrawMsgFull(top-1, 0); }   /* 0 = режим оверлея */

    /* Помощник переключения: мигающая стрелка над индикатором передачи */
    if(g_shift_assist_enabled && CAN_IsMotorAlive()) {
        float rpm = param_values[PARAM_RPM];
        float spd = param_values[PARAM_SPEED];
        if((HAL_GetTick()/400)&1) {   /* мигание 400 мс */
            if(rpm > (float)g_shift_crit_rpm)                DrawShiftArrow(160, 396, 1, COLOR_RED);
            else if(rpm > (float)g_shift_up_rpm)             DrawShiftArrow(160, 396, 1, COLOR_YELLOW);
            else if(spd > 10.0f && rpm < (float)g_shift_down_rpm) DrawShiftArrow(160, 396, 0, COLOR_YELLOW);
        }
    }
}

/* ============================================================================
   MENU LIST (With Dynamic ECU Name for Diagnostics)
   ============================================================================ */
void UI_DrawMenuList(const char* title, const char** items, uint8_t count, uint8_t selected){
    DrawBg(g_current_bg_idx); ILI9481_FillRect(0,0,ILI9481_WIDTH,40,g_theme.header_color);
    ILI9481_DrawString_QSPI(80,12,title,COLOR_WHITE,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
    const uint8_t ITEM_H=40, MAX_VIS=(ILI9481_HEIGHT-40)/ITEM_H;
    uint8_t scroll=(count>MAX_VIS && selected>=MAX_VIS)?selected-MAX_VIS+1:0;
    for(uint8_t i=0;i<count;i++){
        int16_t y=40+(int16_t)(i-scroll)*ITEM_H; if(y<40||y+ITEM_H>ILI9481_HEIGHT) continue;

        const char* text = items[i];
        char dynamic_text[32];

        /* ПОДМЕНА ТОЛЬКО ВНУТРИ ПОДМЕНЮ ДИАГНОСТИКИ, а не в корневом меню */
        if (ui_state.screen == SCREEN_MENU_SUB &&
            ui_state.active_submenu == SUB_DIAG && i == 0) {
            const char* ecu_name = "Не выбран";
            for(uint8_t k=0; k<KLINE_ECU_COUNT; k++) {
                if(KLINE_ECU_LIST[k].addr == g_diag_addr) {
                    ecu_name = KLINE_ECU_LIST[k].name;
                    break;
                }
            }
            snprintf(dynamic_text, sizeof(dynamic_text), "%02X %s", g_diag_addr, ecu_name);
            text = dynamic_text;
        }

        if(i==selected){
            ILI9481_FillRect(0,y,ILI9481_WIDTH,ITEM_H,g_theme.cursor_color);
            ILI9481_DrawString_QSPI(20,y+12,text,COLOR_WHITE,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
        } else {
            ILI9481_DrawString_QSPI(20,y+12,text,COLOR_WHITE,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
        }
    }
}

/* ============================================================================
   OTHER SCREENS (Placeholders & Implementations)
   ============================================================================ */
void UI_DrawWallpaper(const UiState_t* state, uint8_t selected_idx){
    DrawBg(selected_idx); ILI9481_FillRect(0,0,ILI9481_WIDTH,40,0x0000);
    ILI9481_DrawString_QSPI(100,12,"ПОДЛОЖКА",COLOR_WHITE,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
    const char* items[]={"Фон 1","Фон 2","Фон 3","Назад"}; uint16_t h=50,y=60;
    for(uint8_t i=0;i<4;i++){
        if(i==state->menu_idx){ ILI9481_FillRect(0,y,ILI9481_WIDTH,h,g_theme.cursor_color); ILI9481_DrawString_QSPI(20,y+15,items[i],COLOR_WHITE,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT); }
        else if(i==selected_idx){ ILI9481_DrawString_QSPI(20,y+15,items[i],COLOR_CYAN,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT); }
        else{ ILI9481_DrawString_QSPI(20,y+15,items[i],COLOR_WHITE,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT); }
        y+=h;
    }
}

void UI_DrawMiddleArea(const UiState_t* state, const uint8_t* param_vis){
    DrawBg(g_current_bg_idx); ILI9481_FillRect(0,0,ILI9481_WIDTH,40,g_theme.header_color);
    ILI9481_DrawString_QSPI(50,12,"СРЕДНЯЯ ОБЛАСТЬ",COLOR_WHITE,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
    uint16_t h=40,total=PARAM_COUNT+1;
    uint8_t max_vis=(ILI9481_HEIGHT-40)/h;
    uint8_t scroll=(state->menu_idx>=max_vis)?state->menu_idx-max_vis+1:0;
    for(uint8_t i=0;i<total;i++){
        int16_t y=40+(int16_t)(i-scroll)*h; if(y<40||y+h>ILI9481_HEIGHT) continue;
        uint8_t sel=(i==state->menu_idx), en=(i<PARAM_COUNT)?param_vis[i]:0;
        const char* name=(i<PARAM_COUNT)?PARAM_LABELS[i]:"Назад";
        if(sel){
            ILI9481_FillRect(0,y,ILI9481_WIDTH,h,g_theme.cursor_color);
            ILI9481_DrawString_QSPI(15,y+12,name,COLOR_WHITE,COLOR_TRANSPARENT,FONT_16_ADDR,FONT_16_CHAR_WIDTH,FONT_16_CHAR_HEIGHT);
            if(i<PARAM_COUNT) DrawCheckbox(280, y+10, en);
        } else {
            ILI9481_DrawString_QSPI(15,y+12,name,COLOR_WHITE,COLOR_TRANSPARENT,FONT_16_ADDR,FONT_16_CHAR_WIDTH,FONT_16_CHAR_HEIGHT);
            if(i<PARAM_COUNT) DrawCheckbox(280, y+10, en);
        }
    }
}

void UI_DrawColors(const UiState_t* state){
    static const char* names[]={"Красный","Синий","Зеленый","Оранжевый","Фиолетовый","Бирюзовый","Свой цвет","Назад"};
    DrawBg(g_current_bg_idx); ILI9481_FillRect(0,0,ILI9481_WIDTH,40,g_theme.header_color);
    ILI9481_DrawString_QSPI(100,12,"ЦВЕТА",COLOR_WHITE,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
    uint16_t y = 50; const uint16_t ITEM_H = 38;
    for(uint8_t i=0; i<8; i++){
        if(y + ITEM_H > ILI9481_HEIGHT - 30) break;
        if(i == state->menu_idx) ILI9481_FillRect(0,y,ILI9481_WIDTH,ITEM_H,g_theme.cursor_color);
        ILI9481_DrawString_QSPI(20,y+10,names[i],COLOR_WHITE,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
        y += ITEM_H;
    }
}

void UI_DrawCustomColor(const UiState_t* state, const ColorPicker_t* cp){
    DrawBg(g_current_bg_idx); ILI9481_FillRect(0,0,ILI9481_WIDTH,40,g_theme.header_color);
    ILI9481_DrawString_QSPI(60,12,"СВОЙ ЦВЕТ",COLOR_WHITE,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
    const char* lines[]={"Красный (R)","Зеленый (G)","Синий (B)","Назад"};
    uint16_t y = 60; const uint16_t ITEM_H = 40;
    for(uint8_t i=0; i<4; i++){
        if(i == cp->focus) ILI9481_FillRect(0,y,ILI9481_WIDTH,ITEM_H,g_theme.cursor_color);
        ILI9481_DrawString_QSPI(20,y+12,lines[i],COLOR_WHITE,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
        if(i < 3){ char v[8]; uint8_t val = (i==0)?cp->r:(i==1)?cp->g:cp->b; snprintf(v,sizeof(v)," = %d",val); ILI9481_DrawString_QSPI(160,y+12,v,COLOR_WHITE,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT); }
        y += ITEM_H;
    }
}

static void DrawMfaLine20(uint16_t y, const char* label, const char* value) {
    ILI9481_DrawString_QSPI_Pro(12, y, label, COLOR_WHITE, COLOR_TRANSPARENT, FONT_20_ADDR, FONT_20_CHAR_WIDTH, FONT_20_CHAR_HEIGHT, 1);
    uint16_t w = ILI9481_StringWidth_QSPI_Pro(value, FONT_20_ADDR, FONT_20_CHAR_WIDTH, FONT_20_CHAR_HEIGHT, 1);
    ILI9481_DrawString_QSPI_Pro(ILI9481_WIDTH - w - 10, y, value, COLOR_WHITE, COLOR_TRANSPARENT, FONT_20_ADDR, FONT_20_CHAR_WIDTH, FONT_20_CHAR_HEIGHT, 1);
}

void UI_DrawMeas_Bc1(const float* vals) {
    DrawBg(g_current_bg_idx); ILI9481_FillRect(0,0,ILI9481_WIDTH,40,g_theme.header_color);
    ILI9481_DrawString_QSPI(85,10,"ДАННЫЕ БК 1",COLOR_WHITE,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
    char buf[32]; uint16_t y=60; const uint16_t STEP=28;
    snprintf(buf,sizeof(buf),"%.0f км/ч",vals[PARAM_SPEED]); DrawMfaLine20(y,"Скорость:",buf); y+=STEP;
    snprintf(buf,sizeof(buf),"%.0f об/мин",vals[PARAM_RPM]); DrawMfaLine20(y,"Обороты:",buf); y+=STEP;
    snprintf(buf,sizeof(buf),"%.1f %s", vals[PARAM_CONSUMPTION_INSTANT], UnitFor(PARAM_CONSUMPTION_INSTANT, vals)); DrawMfaLine20(y,"Расход:",buf); y+=STEP;
    snprintf(buf,sizeof(buf),"%.1f °C",vals[PARAM_COOLANT_TEMP]); DrawMfaLine20(y,"Темп. ОЖ:",buf); y+=STEP;
    snprintf(buf,sizeof(buf),"%.1f °C",vals[PARAM_OIL_TEMP]); DrawMfaLine20(y,"Темп. Масла:",buf); y+=STEP;
    snprintf(buf,sizeof(buf),"%.1f °C",vals[PARAM_INTAKE_TEMP]); DrawMfaLine20(y,"Темп. Впуска:",buf);
    ILI9481_FillRect(0,440,ILI9481_WIDTH,40,g_theme.header_color);
    ILI9481_DrawString_QSPI(120,450,"Назад",COLOR_WHITE,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
}

void UI_DrawMeas_Bc2(const float* vals) {
    DrawBg(g_current_bg_idx); ILI9481_FillRect(0,0,ILI9481_WIDTH,40,g_theme.header_color);
    ILI9481_DrawString_QSPI(85,10,"ДАННЫЕ БК 2",COLOR_WHITE,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
    char buf[32]; uint16_t y=60; const uint16_t STEP=28;
    snprintf(buf,sizeof(buf),"%.0f км/ч",vals[PARAM_SPEED]); DrawMfaLine20(y,"Скорость:",buf); y+=STEP;
    snprintf(buf,sizeof(buf),"%.0f об/мин",vals[PARAM_RPM]); DrawMfaLine20(y,"Обороты:",buf); y+=STEP;
    snprintf(buf,sizeof(buf),"%.1f %s", vals[PARAM_CONSUMPTION_INSTANT], UnitFor(PARAM_CONSUMPTION_INSTANT, vals)); DrawMfaLine20(y,"Расход:",buf); y+=STEP;
    snprintf(buf,sizeof(buf),"%.1f °C",vals[PARAM_COOLANT_TEMP]); DrawMfaLine20(y,"Темп. ОЖ:",buf); y+=STEP;
    snprintf(buf,sizeof(buf),"%.1f °C",vals[PARAM_OIL_TEMP]); DrawMfaLine20(y,"Темп. Масла:",buf); y+=STEP;
    snprintf(buf,sizeof(buf),"%.1f °C",vals[PARAM_INTAKE_TEMP]); DrawMfaLine20(y,"Темп. Впуска:",buf);
    ILI9481_FillRect(0,440,ILI9481_WIDTH,40,g_theme.header_color);
    ILI9481_DrawString_QSPI(120,450,"Назад",COLOR_WHITE,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
}

void UI_DrawMeas_General(const float* vals) {
    DrawBg(g_current_bg_idx); ILI9481_FillRect(0,0,ILI9481_WIDTH,40,g_theme.header_color);
    ILI9481_DrawString_QSPI(80,10,"ОБЩИЕ ДАННЫЕ",COLOR_WHITE,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
    char buf[32]; uint16_t y=60; const uint16_t STEP=28;
    snprintf(buf,sizeof(buf),"%.0f км/ч",vals[PARAM_SPEED]); DrawMfaLine20(y,"Скорость:",buf); y+=STEP;
    snprintf(buf,sizeof(buf),"%.0f об/мин",vals[PARAM_RPM]); DrawMfaLine20(y,"Обороты:",buf); y+=STEP;
    snprintf(buf,sizeof(buf),"%.2f В",vals[PARAM_VOLTAGE]); DrawMfaLine20(y,"Напряжение:",buf); y+=STEP;
    snprintf(buf,sizeof(buf),"%.2f bar", g_motor_can.boost_pressure);   DrawMfaLine20(y,"Наддув:",buf); y+=STEP;
    snprintf(buf,sizeof(buf),"%.1f °C",vals[PARAM_COOLANT_TEMP]); DrawMfaLine20(y,"Темп. ОЖ двиг.:",buf); y+=STEP;
    snprintf(buf,sizeof(buf),"%.1f °C", g_motor_can.coolant_kombi);     DrawMfaLine20(y,"Темп. ОЖ приб.:",buf); y+=STEP;
    snprintf(buf,sizeof(buf),"%.1f °C",vals[PARAM_INTAKE_TEMP]); DrawMfaLine20(y,"Темп. на впуске:",buf); y+=STEP;
    snprintf(buf,sizeof(buf),"%.1f °C",vals[PARAM_OIL_TEMP]); DrawMfaLine20(y,"Темп. масла:",buf); y+=STEP;
    snprintf(buf,sizeof(buf),"%.0f Nm", g_motor_can.torque_req);        DrawMfaLine20(y,"Момент:",buf); y+=STEP;
    snprintf(buf,sizeof(buf),"%.1f %%", g_motor_can.throttle_position); DrawMfaLine20(y,"Дроссель:",buf); y+=STEP;
    snprintf(buf,sizeof(buf),"%.0f км",vals[PARAM_ODOMETER]); DrawMfaLine20(y,"Пробег общий:",buf); y+=STEP;
    snprintf(buf,sizeof(buf),"%.1f %s", vals[PARAM_CONSUMPTION_INSTANT], UnitFor(PARAM_CONSUMPTION_INSTANT, vals)); DrawMfaLine20(y,"Расход:",buf); y+=STEP;
    snprintf(buf,sizeof(buf),"%.0f °",vals[PARAM_STEERING_ANGLE]); DrawMfaLine20(y,"Угол руля:",buf);
    ILI9481_FillRect(0,440,ILI9481_WIDTH,40,g_theme.header_color);
    ILI9481_DrawString_QSPI(120,450,"Назад",COLOR_WHITE,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
}

void UI_DrawServ_CanMonMenu(void) {
    DrawBg(g_current_bg_idx); ILI9481_FillRect(0,0,ILI9481_WIDTH,40,g_theme.header_color);
    ILI9481_DrawString_QSPI(60,12,"CAN-МОНИТОР",COLOR_WHITE,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
    const char* items[] = {"Motor CAN", "Comfort CAN", "Назад"};
    uint16_t y = 60; const uint16_t ITEM_H = 40;
    for(uint8_t i=0; i<3; i++) {
        uint8_t is_sel = (i == ui_state.menu_idx);
        if(is_sel) ILI9481_FillRect(0,y,ILI9481_WIDTH,ITEM_H,g_theme.cursor_color);
        if(is_sel) ILI9481_DrawString_QSPI(10,y+12,"*",COLOR_WHITE,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
        ILI9481_DrawString_QSPI(30,y+12,items[i],is_sel?COLOR_WHITE:COLOR_GRAY,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
        y += ITEM_H;
    }
}

void UI_DrawServ_CanMonData(void) {
    ILI9481_FillRect(0,0,ILI9481_WIDTH,ILI9481_HEIGHT,COLOR_BLACK);
    ILI9481_FillRect(0,0,ILI9481_WIDTH,40,g_theme.header_color);
    const char* title = (g_can_mon_bus_select==0) ? "Motor CAN" : "Comfort CAN";
    ILI9481_DrawString_QSPI(10,12,title,COLOR_WHITE,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
    ILI9481_DrawString_QSPI(240,14, g_can_mon_bus_select==0 ? "[M] C" : "M [C]", COLOR_CYAN,COLOR_TRANSPARENT,FONT_16_ADDR,FONT_16_CHAR_WIDTH,FONT_16_CHAR_HEIGHT);
    uint16_t y = 50; const uint16_t ROW_H = 18; char buf[64];
    const uint16_t ID_PALETTE[] = {0xF800,0x07E0,0x001F,0xFFE0,0xF81F,0x07FF,0xFD20,0x8410,0x780F,0x0F0F,0x00FF,0xF00F,0xFF00,0x00F0,0xF0F0,0x0F00};
    uint8_t sorted_idx[CAN_MONITOR_MAX_IDS]; uint8_t valid_count = 0;
    for(uint8_t i=0; i<g_can_log_count; i++) {
        if(g_can_log[i].bus == g_can_mon_bus_select && (g_can_log[i].id != 0 || g_can_log[i].len > 0))
            sorted_idx[valid_count++] = i;
    }
    for(uint8_t i=0; i<valid_count-1; i++) {
        for(uint8_t j=0; j<valid_count-i-1; j++) {
            uint8_t a = sorted_idx[j], b = sorted_idx[j+1];
            if(g_can_log[a].id > g_can_log[b].id) {
                uint8_t tmp = sorted_idx[j]; sorted_idx[j] = sorted_idx[j+1]; sorted_idx[j+1] = tmp;
            }
        }
    }
    for(uint8_t i=0; i<valid_count; i++) {
        CanMonitorFrame_t* f = &g_can_log[sorted_idx[i]];
        snprintf(buf,sizeof(buf),"%03lX  %02X %02X %02X %02X %02X %02X %02X %02X",
                 f->id, f->data[0],f->data[1],f->data[2],f->data[3],f->data[4],f->data[5],f->data[6],f->data[7]);
        uint16_t col = ID_PALETTE[f->id % 16];
        ILI9481_DrawString_QSPI(10,y,buf,col,COLOR_TRANSPARENT,FONT_16_ADDR,FONT_16_CHAR_WIDTH,FONT_16_CHAR_HEIGHT);
        y += ROW_H;
        if(y >= 455) break;
    }
    ILI9481_DrawString_QSPI(30,462,"ВВЕРХ/ВНИЗ - шина   ОК - назад",COLOR_GRAY,COLOR_TRANSPARENT,FONT_16_ADDR,FONT_16_CHAR_WIDTH,FONT_16_CHAR_HEIGHT);
}

void UI_DrawGraph_Anim(const UiState_t* state){
    DrawBg(g_current_bg_idx); ILI9481_FillRect(0,0,ILI9481_WIDTH,40,g_theme.header_color);
    ILI9481_DrawString_QSPI(100,12,"АНИМАЦИЯ",COLOR_WHITE,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
    const char* items[] = {"Выключить", "Слайд", "Затухание"};
    uint16_t y = 60; const uint16_t ITEM_H = 40;
    for(uint8_t i=0; i<3; i++){
        if(i == state->menu_idx) ILI9481_FillRect(0,y,ILI9481_WIDTH,ITEM_H,g_theme.cursor_color);
        ILI9481_DrawString_QSPI(20,y+12,items[i],COLOR_WHITE,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
        y += ITEM_H;
    }
}

static void DrawRedGradientRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    for(uint16_t i = 0; i < h; i++) {
        uint8_t r = 255 - (185 * i) / (h - 1);
        ILI9481_FillRect(x, y + i, w, 1, RGB565(r, 0, 0));
    }
    ILI9481_DrawRect(x, y, w, h, g_theme.header_color);
}

static void DrawCheckbox(uint16_t x, uint16_t y, uint8_t checked) {
    ILI9481_FillRect(x, y, 20, 20, COLOR_BLACK);
    ILI9481_DrawRect(x, y, 20, 20, COLOR_WHITE);
    if(checked) {
        ILI9481_DrawLine(x+4, y+10, x+8, y+14, g_theme.cursor_color);
        ILI9481_DrawLine(x+4, y+11, x+8, y+15, g_theme.cursor_color);
        ILI9481_DrawLine(x+8, y+14, x+15, y+5, g_theme.cursor_color);
        ILI9481_DrawLine(x+8, y+15, x+15, y+6, g_theme.cursor_color);
    }
}

void UI_DrawGraph_TopSettings(const UiState_t* state){
    DrawBg(g_current_bg_idx); ILI9481_FillRect(0,0,ILI9481_WIDTH,40,g_theme.header_color);
    ILI9481_DrawString_QSPI(70,12,"ВЕРХНЯЯ СТРОКА",COLOR_WHITE,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
    const uint16_t PY = 60, PH = 40;
    ILI9481_FillRect(0, PY, ILI9481_WIDTH, PH, COLOR_BLACK);
    if(state->menu_idx == 0) DrawRedGradientRect(0,   PY+2, 160, PH-4);
    if(state->menu_idx == 1) DrawRedGradientRect(160, PY+2, 160, PH-4);
    DrawSlot(0,   PY+8, g_top_left,  param_values, 0, g_top_icons);
    DrawSlot(190, PY+8, g_top_right, param_values, 1, g_top_icons);
    uint16_t y = PY + PH + 10; const uint16_t ITEM_H = 40;
    if(2 == state->menu_idx) ILI9481_FillRect(0,y,ILI9481_WIDTH,ITEM_H,g_theme.cursor_color);
    DrawCheckbox(15, y+10, g_top_icons);
    ILI9481_DrawString_QSPI(45,y+12,"Иконки",COLOR_WHITE,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
    if(state->menu_idx == 3) ILI9481_FillRect(0, 440, ILI9481_WIDTH, 40, g_theme.cursor_color);
    ILI9481_DrawString_QSPI(120, 450, "Назад", COLOR_WHITE, COLOR_TRANSPARENT, FONT_20_ADDR, FONT_20_CHAR_WIDTH, FONT_20_CHAR_HEIGHT);
}

void UI_DrawTopParams(const UiState_t* state){
    DrawBg(g_current_bg_idx); ILI9481_FillRect(0,0,ILI9481_WIDTH,40,g_theme.header_color);
    ILI9481_DrawString_QSPI(60,12, g_top_edit_side ? "ПАРАМЕТР СПРАВА" : "ПАРАМЕТР СЛЕВА", COLOR_WHITE,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
    MfaParam_t cur = g_top_edit_side ? g_top_right : g_top_left;
    uint16_t h=40,total=PARAM_COUNT+1;
    uint8_t max_vis=(ILI9481_HEIGHT-40)/h;
    uint8_t scroll=(state->menu_idx>=max_vis)?state->menu_idx-max_vis+1:0;
    for(uint8_t i=0;i<total;i++){
        int16_t y=40+(int16_t)(i-scroll)*h; if(y<40||y+h>ILI9481_HEIGHT) continue;
        uint8_t sel=(i==state->menu_idx);
        const char* name=(i<PARAM_COUNT)?PARAM_LABELS[i]:"Назад";
        if(sel) ILI9481_FillRect(0,y,ILI9481_WIDTH,h,g_theme.cursor_color);
        ILI9481_DrawString_QSPI(15,y+12,name,COLOR_WHITE,COLOR_TRANSPARENT,FONT_16_ADDR,FONT_16_CHAR_WIDTH,FONT_16_CHAR_HEIGHT);
        if(i<PARAM_COUNT) DrawCheckbox(280, y+10, (MfaParam_t)i == cur);
    }
}

void UI_DrawGraph_BottomSettings(const UiState_t* state){
    DrawBg(g_current_bg_idx); ILI9481_FillRect(0,0,ILI9481_WIDTH,40,g_theme.header_color);
    ILI9481_DrawString_QSPI(60,12,"НИЖНЯЯ ОБЛАСТЬ",COLOR_WHITE,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
    const uint16_t PY = 75, PH = 96;
    ILI9481_FillRect(0, PY, ILI9481_WIDTH, PH, COLOR_BLACK);
    const uint16_t HX[4] = {0, 160, 0, 160};
    const uint16_t HY[4] = {PY, PY, PY+55, PY+55};
    if(state->menu_idx <= 3) DrawRedGradientRect(HX[state->menu_idx], HY[state->menu_idx], 160, 40);
    DrawSlot(0,   PY+8,  g_bot_slots[0], param_values, 0, g_bot_icons);
    DrawSlot(190, PY+8,  g_bot_slots[1], param_values, 1, g_bot_icons);
    DrawSlot(0,   PY+63, g_bot_slots[2], param_values, 0, g_bot_icons);
    DrawSlot(190, PY+63, g_bot_slots[3], param_values, 1, g_bot_icons);
    uint16_t y = PY + PH + 10; const uint16_t ITEM_H = 40;
    if(4 == state->menu_idx) ILI9481_FillRect(0,y,ILI9481_WIDTH,ITEM_H,g_theme.cursor_color);
    DrawCheckbox(15, y+10, g_bot_icons);
    ILI9481_DrawString_QSPI(45,y+12,"Иконки",COLOR_WHITE,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
    if(state->menu_idx == 5) ILI9481_FillRect(0, 440, ILI9481_WIDTH, 40, g_theme.cursor_color);
    ILI9481_DrawString_QSPI(120, 450, "Назад", COLOR_WHITE, COLOR_TRANSPARENT, FONT_20_ADDR, FONT_20_CHAR_WIDTH, FONT_20_CHAR_HEIGHT);
}

void UI_DrawBotParams(const UiState_t* state){
    DrawBg(g_current_bg_idx); ILI9481_FillRect(0,0,ILI9481_WIDTH,40,g_theme.header_color);
    static const char* names[4] = {"ЛЕВЫЙ ВЕРХНИЙ","ПРАВЫЙ ВЕРХНИЙ","ЛЕВЫЙ НИЖНИЙ","ПРАВЫЙ НИЖНИЙ"};
    ILI9481_DrawString_QSPI(40,12,names[g_bot_edit_slot],COLOR_WHITE,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
    MfaParam_t cur = g_bot_slots[g_bot_edit_slot];
    uint16_t h=40,total=PARAM_COUNT+1;
    uint8_t max_vis=(ILI9481_HEIGHT-40)/h;
    uint8_t scroll=(state->menu_idx>=max_vis)?state->menu_idx-max_vis+1:0;
    for(uint8_t i=0;i<total;i++){
        int16_t y=40+(int16_t)(i-scroll)*h; if(y<40||y+h>ILI9481_HEIGHT) continue;
        uint8_t sel=(i==state->menu_idx);
        const char* name=(i<PARAM_COUNT)?PARAM_LABELS[i]:"Назад";
        if(sel) ILI9481_FillRect(0,y,ILI9481_WIDTH,h,g_theme.cursor_color);
        ILI9481_DrawString_QSPI(15,y+12,name,COLOR_WHITE,COLOR_TRANSPARENT,FONT_16_ADDR,FONT_16_CHAR_WIDTH,FONT_16_CHAR_HEIGHT);
        if(i<PARAM_COUNT) DrawCheckbox(280, y+10, (MfaParam_t)i == cur);
    }
}

#define GRAPH_POINTS 320
static const uint16_t GCOLORS[5] = {0xF81F, 0x07FF, 0xFFE0, 0xFFFF, 0x07E0};
static const char* GSHORT[PARAM_COUNT] = {"СКОР","RPM","ОЖ","РАСХ","СР.Р","ДИСТ","ТОПЛ","ЗАПАС","ВРЕМЯ","МАСЛО","УЛИЦА","ВПУСК","НАПР","РУЛЬ","ПРОБЕГ"};
static const float GRANGE[PARAM_COUNT][2] = {
    {0,220},{0,7000},{0,130},{0,30},{0,30},{0,500},{0,80},{0,1000},{0,600},
    {0,150},{-40,60},{-20,100},{9,18},{-550,550},{0,100}
};

void UI_DrawGraphsSettings(const UiState_t* state){
    DrawBg(g_current_bg_idx); ILI9481_FillRect(0,0,ILI9481_WIDTH,40,g_theme.header_color);
    ILI9481_DrawString_QSPI(110,12,"Графики",COLOR_WHITE,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
    uint16_t y=50; const uint16_t H=40; char line[40];
    for(uint8_t i=0;i<5;i++){
        if(i==state->menu_idx) ILI9481_FillRect(0,y,ILI9481_WIDTH,H,g_theme.cursor_color);
        snprintf(line,sizeof(line),"%u.%s",i+1,(g_graph_slots[i]==PARAM_COUNT)?"Пусто":PARAM_LABELS[g_graph_slots[i]]);
        ILI9481_DrawString_QSPI(10,y+12,line,(g_graph_slots[i]==PARAM_COUNT)?COLOR_GRAY:GCOLORS[i],COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
        y+=H;
    }
    const char* tail[2]={"Показать","Назад"};
    for(uint8_t i=0;i<2;i++){
        if(5+i==state->menu_idx) ILI9481_FillRect(0,y,ILI9481_WIDTH,H,g_theme.cursor_color);
        ILI9481_DrawString_QSPI(10,y+12,tail[i],COLOR_WHITE,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
        y+=H;
    }
}

void UI_DrawGraphsPick(const UiState_t* state){
    DrawBg(g_current_bg_idx); ILI9481_FillRect(0,0,ILI9481_WIDTH,40,g_theme.header_color);
    ILI9481_DrawString_QSPI(110,12,"Графики",COLOR_WHITE,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
    uint16_t h=40, total=PARAM_COUNT+1;
    uint8_t max_vis=(ILI9481_HEIGHT-40)/h;
    uint8_t scroll=(state->menu_idx>=max_vis)?state->menu_idx-max_vis+1:0;
    for(uint8_t i=0;i<total;i++){
        int16_t y=40+(int16_t)(i-scroll)*h; if(y<40||y+h>ILI9481_HEIGHT) continue;
        if(i==state->menu_idx) ILI9481_FillRect(0,y,ILI9481_WIDTH,h,g_theme.cursor_color);
        const char* name=(i==0)?"< Пусто >":PARAM_LABELS[i-1];
        ILI9481_DrawString_QSPI(15,y+12,name,(i==0)?COLOR_GRAY:COLOR_WHITE,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
    }
}

void UI_DrawGraphsView(const UiState_t* state){
    static float buf[5][GRAPH_POINTS]; static uint16_t head=0, count=0; static uint32_t last_s=0;
    if(g_graphs_reset){ head=0; count=0; last_s=0; g_graphs_reset=0; }
    uint32_t now=HAL_GetTick();
    if(!g_graphs_paused && now-last_s>=200){
        last_s=now;
        for(uint8_t s=0;s<5;s++)
            if(g_graph_slots[s]<PARAM_COUNT) buf[s][head]=param_values[g_graph_slots[s]];
        head=(head+1)%GRAPH_POINTS; if(count<GRAPH_POINTS) count++;
    }
    ILI9481_FillRect(0,0,ILI9481_WIDTH,ILI9481_HEIGHT,COLOR_BLACK);
    ILI9481_FillRect(0,0,ILI9481_WIDTH,40,g_theme.header_color);
    ILI9481_DrawString_QSPI(110,12,"Графики",COLOR_WHITE,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
    const uint16_t GY=45, GH=310;
    for(uint8_t g=1; g<4; g++) ILI9481_DrawHLine(0, GY+GH*g/4, ILI9481_WIDTH, 0x2104);
    for(uint8_t s=0;s<5;s++){
        MfaParam_t p=g_graph_slots[s]; if(p>=PARAM_COUNT) continue;
        float mn=GRANGE[p][0], mx=GRANGE[p][1];
        uint8_t first=1; uint16_t x0=0, y0=0;
        for(uint16_t x=0;x<ILI9481_WIDTH;x++){
            if(x >= ILI9481_WIDTH-count) {
                uint16_t i=(head-count+(x-(ILI9481_WIDTH-count))+2*GRAPH_POINTS)%GRAPH_POINTS;
                float v=buf[s][i];
                if(v<mn)v=mn; if(v>mx)v=mx;
                uint16_t y=GY+GH-1-(uint16_t)((v-mn)/(mx-mn)*(GH-2));
                if(!first) ILI9481_DrawLine(x0,y0,x,y,GCOLORS[s]);
                else { ILI9481_DrawPixel(x,y,GCOLORS[s]); first=0; }
                x0=x; y0=y;
            }
        }
    }
    uint8_t col=0; char v[16];
    for(uint8_t s=0;s<5;s++){
        MfaParam_t p=g_graph_slots[s]; if(p>=PARAM_COUNT) continue;
        uint16_t x=col*64;
        ILI9481_DrawString_QSPI(x+2,360,GSHORT[p],GCOLORS[s],COLOR_TRANSPARENT,FONT_16_ADDR,FONT_16_CHAR_WIDTH,FONT_16_CHAR_HEIGHT);
        snprintf(v,sizeof(v),(p==PARAM_RPM||p==PARAM_SPEED||p==PARAM_ODOMETER)?"%.0f":"%.1f",param_values[p]);
        ILI9481_DrawString_QSPI(x+2,380,v,COLOR_WHITE,COLOR_TRANSPARENT,FONT_16_ADDR,FONT_16_CHAR_WIDTH,FONT_16_CHAR_HEIGHT);
        ILI9481_DrawString_QSPI(x+2,398,UnitFor(p,param_values),COLOR_GRAY,COLOR_TRANSPARENT,FONT_16_ADDR,FONT_16_CHAR_WIDTH,FONT_16_CHAR_HEIGHT);
        col++;
    }
    uint16_t c0=(state->menu_idx==0)?g_theme.cursor_color:0x1082;
    ILI9481_FillRect(0,418,ILI9481_WIDTH,30,c0);
    ILI9481_DrawString_QSPI(110,425,g_graphs_paused?"Продолжить":"Остановить",COLOR_WHITE,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
    uint16_t c1=(state->menu_idx==1)?g_theme.cursor_color:0x1082;
    ILI9481_FillRect(0,450,ILI9481_WIDTH,30,c1);
    ILI9481_DrawString_QSPI(120,457,"Назад",COLOR_WHITE,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
}

void UI_DrawMeas_Accel(void) {
    static uint8_t st=0; static uint32_t t0=0;
    static float t60=0, t80=0, t100=0, best=0;
    float spd = param_values[PARAM_SPEED];
    uint32_t el = HAL_GetTick() - t0;
    switch(st){
        case 0: if(spd > 3.0f) { t0=HAL_GetTick(); el=0; t60=t80=t100=0; st=1; } break;
        case 1:
            if(t60==0 && spd>=60.0f)  t60  = el/1000.0f;
            if(t80==0 && spd>=80.0f)  t80  = el/1000.0f;
            if(spd>=100.0f) { t100 = el/1000.0f; if(best==0 || t100<best) best=t100; st=2; }
            else if(spd<2.0f) st=0;
            break;
        case 2: if(spd<2.0f) st=0; break;
    }
    DrawBg(g_current_bg_idx);
    ILI9481_FillRect(0,0,ILI9481_WIDTH,40,g_theme.header_color);
    ILI9481_DrawString_QSPI(110,10,"РАЗГОН",COLOR_WHITE,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
    if(best>0){
        char best_str[16];
        snprintf(best_str,sizeof(best_str),"%.1f с",best);
        ILI9481_DrawString_QSPI(240,10,"BEST:",COLOR_GRAY,COLOR_TRANSPARENT,FONT_16_ADDR,FONT_16_CHAR_WIDTH,FONT_16_CHAR_HEIGHT);
        ILI9481_DrawString_QSPI(280,10,best_str,COLOR_YELLOW,COLOR_TRANSPARENT,FONT_16_ADDR,FONT_16_CHAR_WIDTH,FONT_16_CHAR_HEIGHT);
    }
    char buf[32];
    snprintf(buf,sizeof(buf),"%.0f",spd);
    uint16_t spd_w = strlen(buf)*FONT_70_CHAR_WIDTH;
    ILI9481_DrawString_QSPI(160-spd_w/2,50,buf,COLOR_WHITE,COLOR_TRANSPARENT,FONT_70_ADDR,FONT_70_CHAR_WIDTH,FONT_70_CHAR_HEIGHT);
    ILI9481_DrawString_QSPI(160+spd_w/2+5,90,"км/ч",COLOR_GRAY,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
    snprintf(buf,sizeof(buf),"%.1f с",(st==1)?el/1000.0f:((st==2)?t100:0.0f));
    ILI9481_DrawString_QSPI(160-((int16_t)strlen(buf)*FONT_40_CHAR_WIDTH)/2,140,buf,COLOR_YELLOW,COLOR_TRANSPARENT,FONT_40_ADDR,FONT_40_CHAR_WIDTH,FONT_40_CHAR_HEIGHT);
    const uint16_t bar_y=210, bar_h=30, bar_w=280;
    ILI9481_DrawRect(20,bar_y,bar_w,bar_h,COLOR_GRAY);
    ILI9481_FillRect(21,bar_y+1,bar_w-2,bar_h-2,0x1082);
    float progress = (spd<120.0f)?(spd/120.0f):1.0f;
    uint16_t fill_w = (uint16_t)(progress*(bar_w-4));
    if(fill_w>0) ILI9481_FillRect(22,bar_y+2,fill_w,bar_h-4,0x07E0);
    uint16_t m60=20+(uint16_t)(60.0f/120.0f*(bar_w-4));
    uint16_t m80=20+(uint16_t)(80.0f/120.0f*(bar_w-4));
    uint16_t m100=20+(uint16_t)(100.0f/120.0f*(bar_w-4));
    ILI9481_FillRect(m60,bar_y-5,2,bar_h+10,COLOR_WHITE);
    ILI9481_FillRect(m80,bar_y-5,2,bar_h+10,COLOR_WHITE);
    ILI9481_FillRect(m100,bar_y-5,2,bar_h+10,COLOR_WHITE);
    ILI9481_DrawString_QSPI(m60-8,bar_y-20,"60",COLOR_GRAY,COLOR_TRANSPARENT,FONT_16_ADDR,FONT_16_CHAR_WIDTH,FONT_16_CHAR_HEIGHT);
    ILI9481_DrawString_QSPI(m80-8,bar_y-20,"80",COLOR_GRAY,COLOR_TRANSPARENT,FONT_16_ADDR,FONT_16_CHAR_WIDTH,FONT_16_CHAR_HEIGHT);
    ILI9481_DrawString_QSPI(m100-12,bar_y-20,"100",COLOR_GRAY,COLOR_TRANSPARENT,FONT_16_ADDR,FONT_16_CHAR_WIDTH,FONT_16_CHAR_HEIGHT);
    const uint16_t card_y=280, card_w=90, card_h=70;
    uint16_t colors[3] = {(t60>0)?COLOR_GREEN:COLOR_GRAY, (t80>0)?COLOR_GREEN:COLOR_GRAY, (t100>0)?COLOR_GREEN:COLOR_GRAY};
    const char* labels[3] = {"0-60","0-80","0-100"};
    float times[3] = {t60, t80, t100};
    for(uint8_t i=0;i<3;i++){
        uint16_t cx=20+i*(card_w+10);
        ILI9481_DrawRect(cx,card_y,card_w,card_h,colors[i]);
        ILI9481_FillRect(cx+1,card_y+1,card_w-2,card_h-2,0x1082);
        ILI9481_DrawString_QSPI(cx+card_w/2-((int16_t)strlen(labels[i])*FONT_16_CHAR_WIDTH)/2,card_y+5,
                                labels[i],colors[i],COLOR_TRANSPARENT,FONT_16_ADDR,FONT_16_CHAR_WIDTH,FONT_16_CHAR_HEIGHT);
        snprintf(buf,sizeof(buf),"%.1f",times[i]);
        ILI9481_DrawString_QSPI(cx+card_w/2-((int16_t)strlen(buf)*FONT_20_CHAR_WIDTH)/2,card_y+30,
                                buf,COLOR_WHITE,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
        ILI9481_DrawString_QSPI(cx+card_w/2-((int16_t)strlen("с")*FONT_16_CHAR_WIDTH)/2,card_y+50,
                                "с",COLOR_GRAY,COLOR_TRANSPARENT,FONT_16_ADDR,FONT_16_CHAR_WIDTH,FONT_16_CHAR_HEIGHT);
    }
    const char* l1; const char* l2;
    if(st==0)      { l1="Трогайся с места -"; l2="замер начнется сам"; }
    else if(st==1) { l1="Замер...";           l2=""; }
    else           { l1="Готово!";            l2="Остановись для нового"; }
    ILI9481_DrawString_QSPI(160-((int16_t)strlen(l1)*FONT_16_CHAR_WIDTH)/2, 378, l1,
                            COLOR_CYAN, COLOR_TRANSPARENT, FONT_16_ADDR, FONT_16_CHAR_WIDTH, FONT_16_CHAR_HEIGHT);
    if(l2[0]) ILI9481_DrawString_QSPI(160-((int16_t)strlen(l2)*FONT_16_CHAR_WIDTH)/2, 398, l2,
                                      COLOR_CYAN, COLOR_TRANSPARENT, FONT_16_ADDR, FONT_16_CHAR_WIDTH, FONT_16_CHAR_HEIGHT);
    ILI9481_FillRect(0,440,ILI9481_WIDTH,40,g_theme.header_color);
    ILI9481_DrawString_QSPI(120,450,"Назад",COLOR_WHITE,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
}

#define CAR_MASS_KG 1330.0f
#define CRR         0.013f
#define CDA         0.64f
#define AIR_RHO     1.2f
#define TRANS_ETA   0.9f

void UI_DrawMeas_Power(void) {
    static float v0=-1.0f, a_sm=0.0f;
    static float peak_run=0.0f, max_hp=0.0f, max_tq=0.0f;
    static uint32_t t0=0;
    float spd = param_values[PARAM_SPEED];
    float v = spd / 3.6f;
    uint32_t now = HAL_GetTick();
    if(v0 < 0) { v0=v; t0=now; }
    if(now - t0 >= 500) {
        float a = (v - v0) / ((now - t0) / 1000.0f);
        a_sm += (a - a_sm) * 0.5f;
        v0 = v; t0 = now;
    }
    static uint8_t stopped = 1;
    if(spd < 2.0f) stopped = 1;
    if(stopped && spd > 3.0f) { peak_run = 0; stopped = 0; }
    float F  = CAR_MASS_KG*a_sm + CRR*CAR_MASS_KG*9.81f + 0.5f*AIR_RHO*CDA*v*v;
    float Pe = (F*v > 0 && spd > 10.0f) ? (F*v)/TRANS_ETA : 0.0f;
    float kw = Pe/1000.0f;
    float hp = kw*1.3596f;
    float rpm = param_values[PARAM_RPM];
    float tq = (rpm>800 && Pe>0) ? Pe/(rpm*6.2832f/60.0f) : 0;
    if(hp > peak_run) peak_run = hp;
    if(hp > max_hp)   max_hp   = hp;
    if(tq > max_tq)   max_tq   = tq;
    DrawBg(g_current_bg_idx);
    ILI9481_FillRect(0,0,ILI9481_WIDTH,40,g_theme.header_color);
    ILI9481_DrawString_QSPI(100,10,"МОЩНОСТЬ",COLOR_WHITE,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
    char buf[32];
    snprintf(buf,sizeof(buf),"%.0f",hp);
    uint16_t hp_w = strlen(buf)*FONT_70_CHAR_WIDTH;
    ILI9481_DrawString_QSPI(160-hp_w/2,50,buf,COLOR_WHITE,COLOR_TRANSPARENT,FONT_70_ADDR,FONT_70_CHAR_WIDTH,FONT_70_CHAR_HEIGHT);
    ILI9481_DrawString_QSPI(160+hp_w/2+5,90,"л.с.",COLOR_GRAY,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
    const uint16_t bar_y=140, bar_h=18, bar_w=280;
    ILI9481_DrawString_QSPI(20,bar_y-16,"МОЩНОСТЬ",COLOR_GRAY,COLOR_TRANSPARENT,FONT_16_ADDR,FONT_16_CHAR_WIDTH,FONT_16_CHAR_HEIGHT);
    ILI9481_DrawRect(20,bar_y,bar_w,bar_h,COLOR_GRAY);
    ILI9481_FillRect(21,bar_y+1,bar_w-2,bar_h-2,0x1082);
    float hp_progress = (hp<150.0f)?(hp/150.0f):1.0f;
    uint16_t hp_fill = (uint16_t)(hp_progress*(bar_w-4));
    if(hp_fill>0) {
        for(uint16_t x=0; x<hp_fill; x++) {
            float ratio = (float)x / (float)hp_fill;
            uint16_t color = (ratio<0.6f)?0x07E0:(ratio<0.85f)?0xFFE0:0xF800;
            ILI9481_FillRect(22+x, bar_y+2, 1, bar_h-4, color);
        }
    }
    snprintf(buf,sizeof(buf),"%.0f л.с.",hp);
    ILI9481_DrawString_QSPI(22,bar_y+2,buf,COLOR_WHITE,COLOR_TRANSPARENT,FONT_16_ADDR,FONT_16_CHAR_WIDTH,FONT_16_CHAR_HEIGHT);
    const uint16_t tq_bar_y=180;
    ILI9481_DrawString_QSPI(20,tq_bar_y-16,"МОМЕНТ",COLOR_GRAY,COLOR_TRANSPARENT,FONT_16_ADDR,FONT_16_CHAR_WIDTH,FONT_16_CHAR_HEIGHT);
    ILI9481_DrawRect(20,tq_bar_y,bar_w,bar_h,COLOR_GRAY);
    ILI9481_FillRect(21,tq_bar_y+1,bar_w-2,bar_h-2,0x1082);
    float tq_progress = (tq<250.0f)?(tq/250.0f):1.0f;
    uint16_t tq_fill = (uint16_t)(tq_progress*(bar_w-4));
    if(tq_fill>0) {
        for(uint16_t x=0; x<tq_fill; x++) {
            float ratio = (float)x / (float)tq_fill;
            uint16_t color = (ratio<0.6f)?0x07FF:(ratio<0.85f)?0x0450:0x001F;
            ILI9481_FillRect(22+x, tq_bar_y+2, 1, bar_h-4, color);
        }
    }
    snprintf(buf,sizeof(buf),"%.0f Нм",tq);
    ILI9481_DrawString_QSPI(22,tq_bar_y+2,buf,COLOR_WHITE,COLOR_TRANSPARENT,FONT_16_ADDR,FONT_16_CHAR_WIDTH,FONT_16_CHAR_HEIGHT);
    uint16_t y=230; const uint16_t STEP=28;
    snprintf(buf,sizeof(buf),"%.0f кВт",max_hp*0.7355f); DrawMfaLine20(y,"Пик. мощность:",buf); y+=STEP;
    snprintf(buf,sizeof(buf),"%.0f Нм",max_tq);        DrawMfaLine20(y,"Пик. момент:",buf);   y+=STEP;
    snprintf(buf,sizeof(buf),"%.0f л.с.",peak_run);    DrawMfaLine20(y,"Пик за замер:",buf);
    ILI9481_DrawString_QSPI(160-((int16_t)strlen("Газ в пол -")*FONT_16_CHAR_WIDTH)/2, 398,
                            "Газ в пол -", COLOR_CYAN, COLOR_TRANSPARENT, FONT_16_ADDR, FONT_16_CHAR_WIDTH, FONT_16_CHAR_HEIGHT);
    ILI9481_DrawString_QSPI(160-((int16_t)strlen("пики зафиксируются")*FONT_16_CHAR_WIDTH)/2, 418,
                            "пики зафиксируются", COLOR_CYAN, COLOR_TRANSPARENT, FONT_16_ADDR, FONT_16_CHAR_WIDTH, FONT_16_CHAR_HEIGHT);
    ILI9481_FillRect(0,440,ILI9481_WIDTH,40,g_theme.header_color);
    ILI9481_DrawString_QSPI(120,450,"Назад",COLOR_WHITE,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
}

void UI_DrawMeas_Oil(void) {
    DrawBg(g_current_bg_idx);
    ILI9481_FillRect(0,0,ILI9481_WIDTH,40,g_theme.header_color);
    ILI9481_DrawString_QSPI(70,10,"УРОВЕНЬ МАСЛА",COLOR_WHITE,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
    const uint16_t TX=60, TW=70, TY=70, TH=300;
    const uint16_t yMAX=TY+25, yMIN=TY+TH-25;
    ILI9481_DrawRect(TX-5,TY-5,TW+10,TH+10,COLOR_GRAY);
    ILI9481_DrawRect(TX-2,TY-2,TW+4,TH+4,0x2104);
    for(uint16_t y=0;y<TH;y++){
        uint8_t g=8+(y*10)/TH;
        ILI9481_FillRect(TX,TY+y,TW,1,RGB565(g,g+2,g+6));
    }
    float lvl=g_oil_level; if(lvl<0)lvl=0; if(lvl>1.15f)lvl=1.15f;
    uint16_t surf=yMIN-(uint16_t)(lvl*(yMIN-yMAX));
    uint32_t t=HAL_GetTick()/60;
    for(uint16_t x=0;x<TW;x++){
        int16_t sy=surf+(int16_t)(sinf((x+t*2)*0.18f)*2.0f);
        if(sy<TY+1)sy=TY+1; if(sy>TY+TH-2)sy=TY+TH-2;
        for(uint16_t y=sy;y<TY+TH-1;y++){
            uint16_t d=y-sy;
            ILI9481_DrawPixel(TX+x,y,RGB565(255-(d*110)/TH, 180-(d*110)/TH, 40-(d*30)/TH));
        }
        ILI9481_DrawPixel(TX+x,sy,RGB565(255,240,150));
    }
    for(uint8_t i=0;i<6;i++){
        uint16_t per=90+i*23;
        uint16_t pr=(t*3+i*41)%per;
        uint16_t by=TY+TH-4-(pr*(TY+TH-4-surf))/per;
        if(by>surf+6){
            uint16_t bx=TX+8+((i*37)%(TW-16))+(int16_t)(sinf((t+i*7)*0.3f)*3);
            ILI9481_DrawPixel(bx,by,RGB565(255,220,140));
            ILI9481_DrawPixel(bx+1,by,RGB565(255,220,140));
        }
    }
    ILI9481_FillRect(TX-12,yMAX,12,2,COLOR_WHITE);
    ILI9481_FillRect(TX-12,yMIN,12,2,COLOR_WHITE);
    ILI9481_FillRect(TX-8,(yMAX+yMIN)/2,8,1,COLOR_GRAY);
    ILI9481_DrawString_QSPI(10,yMAX-9,"MAX",COLOR_WHITE,COLOR_TRANSPARENT,FONT_16_ADDR,FONT_16_CHAR_WIDTH,FONT_16_CHAR_HEIGHT);
    ILI9481_DrawString_QSPI(12,yMIN-9,"MIN",COLOR_WHITE,COLOR_TRANSPARENT,FONT_16_ADDR,FONT_16_CHAR_WIDTH,FONT_16_CHAR_HEIGHT);
    char buf[24];
    snprintf(buf,sizeof(buf),"%u%%",(uint8_t)(lvl*100));
    ILI9481_DrawString_QSPI(170,120,buf,COLOR_WHITE,COLOR_TRANSPARENT,FONT_40_ADDR,FONT_40_CHAR_WIDTH,FONT_40_CHAR_HEIGHT);
    const char* st; uint16_t sc;
    if(lvl<0.15f)      { st="МАЛО!";   sc=COLOR_RED; }
    else if(lvl<0.35f) { st="Долей";   sc=COLOR_YELLOW; }
    else if(lvl<=1.05f){ st="Норма";   sc=COLOR_GREEN; }
    else               { st="Перелив"; sc=COLOR_YELLOW; }
    ILI9481_DrawString_QSPI(170,175,st,sc,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
    snprintf(buf,sizeof(buf),"%.1f л до MAX",(1.0f-lvl)>0?(1.0f-lvl):0.0f);
    ILI9481_DrawString_QSPI(170,215,buf,COLOR_GRAY,COLOR_TRANSPARENT,FONT_16_ADDR,FONT_16_CHAR_WIDTH,FONT_16_CHAR_HEIGHT);
    ILI9481_DrawString_QSPI(160-((int16_t)strlen("Датчик не подключен -")*FONT_16_CHAR_WIDTH)/2,395,
                            "Датчик не подключен -",COLOR_GRAY,COLOR_TRANSPARENT,FONT_16_ADDR,FONT_16_CHAR_WIDTH,FONT_16_CHAR_HEIGHT);
    ILI9481_DrawString_QSPI(160-((int16_t)strlen("значение демонстрационное")*FONT_16_CHAR_WIDTH)/2,415,
                            "значение демонстрационное",COLOR_GRAY,COLOR_TRANSPARENT,FONT_16_ADDR,FONT_16_CHAR_WIDTH,FONT_16_CHAR_HEIGHT);
    ILI9481_FillRect(0,440,ILI9481_WIDTH,40,g_theme.header_color);
    ILI9481_DrawString_QSPI(120,450,"Назад",COLOR_WHITE,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
}

void UI_DrawDiag_Kline(void) {
    DrawBg(g_current_bg_idx);
    ILI9481_FillRect(0, 0, ILI9481_WIDTH, 40, g_theme.header_color);
    ILI9481_DrawString_QSPI(80, 10, "K-LINE DEBUG", COLOR_WHITE, COLOR_TRANSPARENT,
                            FONT_20_ADDR, FONT_20_CHAR_WIDTH, FONT_20_CHAR_HEIGHT);
    char s[64];

    if (g_kl_state == 1) {
        ILI9481_DrawString_QSPI(60, 70, "Опрос ЭБУ...", COLOR_YELLOW, COLOR_TRANSPARENT,
                                FONT_20_ADDR, FONT_20_CHAR_WIDTH, FONT_20_CHAR_HEIGHT);
    } else if (g_kl_state == 3) {
        ILI9481_DrawString_QSPI(70, 70, "ЭБУ молчит", COLOR_RED, COLOR_TRANSPARENT,
                                FONT_20_ADDR, FONT_20_CHAR_WIDTH, FONT_20_CHAR_HEIGHT);
    } else if (g_kl_state == 2) {
        snprintf(s, sizeof(s), "Связь: KB1=%02X KB2=%02X", g_kline_kb1, g_kline_kb2);
        ILI9481_DrawString_QSPI(10, 50, s, COLOR_GREEN, COLOR_TRANSPARENT,
                                FONT_16_ADDR, FONT_16_CHAR_WIDTH, FONT_16_CHAR_HEIGHT);

        // Вывод идентификатора
        uint16_t y = 70;
        for (uint8_t off = 0; off < g_kl_ident_len && y < 160; off += 23) {
            char line[24];
            uint8_t l = (g_kl_ident_len - off > 23) ? 23 : (g_kl_ident_len - off);
            memcpy(line, (char *)g_kl_ident + off, l);
            line[l] = 0;
            ILI9481_DrawString_QSPI(10, y, line, COLOR_WHITE, COLOR_TRANSPARENT,
                                    FONT_16_ADDR, FONT_16_CHAR_WIDTH, FONT_16_CHAR_HEIGHT);
            y += 18;
        }
    } else {
        ILI9481_DrawString_QSPI(40, 70, "ОК = опросить ЭБУ", COLOR_WHITE, COLOR_TRANSPARENT,
                                FONT_20_ADDR, FONT_20_CHAR_WIDTH, FONT_20_CHAR_HEIGHT);
    }

    ILI9481_FillRect(0, 440, ILI9481_WIDTH, 40, g_theme.header_color);
    ILI9481_DrawString_QSPI(20, 450, "ОК=запрос   ВНИЗ 2с=назад", COLOR_WHITE, COLOR_TRANSPARENT,
                            FONT_16_ADDR, FONT_16_CHAR_WIDTH, FONT_16_CHAR_HEIGHT);
}

void UI_DrawDiag_Ecu(void) {
    DrawBg(g_current_bg_idx);
    ILI9481_FillRect(0,0,ILI9481_WIDTH,40,g_theme.header_color);
    ILI9481_DrawString_QSPI(90,12,"ВЫБОР ЭБУ",COLOR_WHITE,COLOR_TRANSPARENT,
                            FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);

    uint8_t total = KLINE_ECU_COUNT + 1;
    const uint8_t ITEM_H = 40;
    uint8_t max_vis = (ILI9481_HEIGHT - 40) / ITEM_H;
    uint8_t scroll  = (ui_state.menu_idx >= max_vis) ? ui_state.menu_idx - max_vis + 1 : 0;

    for (uint8_t i = 0; i < total; i++) {
        int16_t y = 40 + (int16_t)(i - scroll) * ITEM_H;
        if (y < 40 || y + ITEM_H > ILI9481_HEIGHT) continue;

        uint8_t sel = (i == ui_state.menu_idx);
        if (sel) ILI9481_FillRect(0, y, ILI9481_WIDTH, ITEM_H, g_theme.cursor_color);

        char line[40];
        if (i < KLINE_ECU_COUNT) {
            snprintf(line, sizeof(line), "%02X %s%s", KLINE_ECU_LIST[i].addr,
                     KLINE_ECU_LIST[i].name,
                     (KLINE_ECU_LIST[i].addr == g_diag_addr) ? " *" : "");
        } else {
            snprintf(line, sizeof(line), "Назад");
        }
        ILI9481_DrawString_QSPI(20, y+12, line, sel ? COLOR_WHITE : COLOR_GRAY, COLOR_TRANSPARENT,
                                FONT_20_ADDR, FONT_20_CHAR_WIDTH, FONT_20_CHAR_HEIGHT);
    }
}

void UI_DrawDiag_Meas(void) {
    DrawBg(g_current_bg_idx);
    ILI9481_FillRect(0,0,ILI9481_WIDTH,40,g_theme.header_color);
    ILI9481_DrawString_QSPI(CenterX("БЛОКИ ИЗМЕРЕНИЙ", FONT_20_CHAR_WIDTH), 12,
                            "БЛОКИ ИЗМЕРЕНИЙ", COLOR_WHITE, COLOR_TRANSPARENT,
                            FONT_20_ADDR, FONT_20_CHAR_WIDTH, FONT_20_CHAR_HEIGHT);
    char s[64];
    snprintf(s, sizeof(s), "Группа %03d   ЭБУ %02X", g_diag_group, g_diag_addr);
    ILI9481_DrawString_QSPI(CenterX(s, FONT_20_CHAR_WIDTH), 50, s,
                            COLOR_CYAN, COLOR_TRANSPARENT,
                            FONT_20_ADDR, FONT_20_CHAR_WIDTH, FONT_20_CHAR_HEIGHT);

    if (g_diag_req_meas) {
        ILI9481_DrawString_QSPI(CenterX("Опрос ЭБУ...", FONT_20_CHAR_WIDTH), 120,
                                "Опрос ЭБУ...", COLOR_YELLOW, COLOR_TRANSPARENT,
                                FONT_20_ADDR, FONT_20_CHAR_WIDTH, FONT_20_CHAR_HEIGHT);
        ILI9481_DrawString_QSPI(CenterX("подождите ~3 сек", FONT_16_CHAR_WIDTH), 150,
                                "подождите ~3 сек", COLOR_GRAY, COLOR_TRANSPARENT,
                                FONT_16_ADDR, FONT_16_CHAR_WIDTH, FONT_16_CHAR_HEIGHT);
    } else if (!g_diag_meas.ok) {
        ILI9481_DrawString_QSPI(CenterX("OK = прочитать группу", FONT_20_CHAR_WIDTH), 120,
                                "OK = прочитать группу", COLOR_WHITE, COLOR_TRANSPARENT,
                                FONT_20_ADDR, FONT_20_CHAR_WIDTH, FONT_20_CHAR_HEIGHT);
        ILI9481_DrawString_QSPI(CenterX("UP/DOWN = номер группы", FONT_16_CHAR_WIDTH), 150,
                                "UP/DOWN = номер группы", COLOR_GRAY, COLOR_TRANSPARENT,
                                FONT_16_ADDR, FONT_16_CHAR_WIDTH, FONT_16_CHAR_HEIGHT);
    } else {
        uint16_t y = 90;
        for (uint8_t ch = 0; ch < 4; ch++) {
            snprintf(s, sizeof(s), "%d:", ch + 1);
            ILI9481_DrawString_QSPI(16, y, s, COLOR_GRAY, COLOR_TRANSPARENT,
                                    FONT_20_ADDR, FONT_20_CHAR_WIDTH, FONT_20_CHAR_HEIGHT);
            snprintf(s, sizeof(s), "%.2f", g_diag_meas.val[ch]);
            ILI9481_DrawString_QSPI(48, y, s, COLOR_WHITE, COLOR_TRANSPARENT,
                                    FONT_20_ADDR, FONT_20_CHAR_WIDTH, FONT_20_CHAR_HEIGHT);
            ILI9481_DrawString_QSPI(150, y + 4, g_diag_meas.unit[ch], COLOR_CYAN,
                                    COLOR_TRANSPARENT,
                                    FONT_16_ADDR, FONT_16_CHAR_WIDTH, FONT_16_CHAR_HEIGHT);
            /* RAW: тип и 16-бит значение — для калибровки масштаба */
            snprintf(s, sizeof(s), "t=%02X %04X", g_diag_meas.type[ch], g_diag_meas.raw[ch]);
            ILI9481_DrawString_QSPI(220, y + 4, s, COLOR_GRAY, COLOR_TRANSPARENT,
                                    FONT_16_ADDR, FONT_16_CHAR_WIDTH, FONT_16_CHAR_HEIGHT);
            y += 40;
        }
        ILI9481_DrawString_QSPI(CenterX("UP/DOWN = другая группа", FONT_16_CHAR_WIDTH), 270,
                                "UP/DOWN = другая группа", COLOR_GRAY, COLOR_TRANSPARENT,
                                FONT_16_ADDR, FONT_16_CHAR_WIDTH, FONT_16_CHAR_HEIGHT);
    }

    ILI9481_FillRect(0,440,ILI9481_WIDTH,40,g_theme.header_color);
    ILI9481_DrawString_QSPI(10, 450, "OK=читать  ВНИЗ 2с=назад", COLOR_WHITE,
                            COLOR_TRANSPARENT, FONT_16_ADDR, FONT_16_CHAR_WIDTH, FONT_16_CHAR_HEIGHT);
}

void UI_DrawDiag_ReadErr(void) {
    DrawBg(g_current_bg_idx);
    ILI9481_FillRect(0,0,ILI9481_WIDTH,40,g_theme.header_color);

    // Заголовок с адресом ЭБУ — по центру
    char header[32];
    const char* ecu_name = "ЭБУ";
    for(uint8_t i=0; i<KLINE_ECU_COUNT; i++) {
        if(KLINE_ECU_LIST[i].addr == g_diag_addr) { ecu_name = KLINE_ECU_LIST[i].name; break; }
    }
    snprintf(header, sizeof(header), "%02X %s", g_diag_addr, ecu_name);
    ILI9481_DrawString_QSPI(CenterX(header, FONT_20_CHAR_WIDTH), 12, header,
                            COLOR_WHITE, COLOR_TRANSPARENT,
                            FONT_20_ADDR, FONT_20_CHAR_WIDTH, FONT_20_CHAR_HEIGHT);
    ILI9481_DrawString_QSPI(CenterX("Ошибки", FONT_20_CHAR_WIDTH), 45, "Ошибки",
                            COLOR_WHITE, COLOR_TRANSPARENT,
                            FONT_20_ADDR, FONT_20_CHAR_WIDTH, FONT_20_CHAR_HEIGHT);

    if (g_diag_dtc_busy) {
        ILI9481_DrawString_QSPI(CenterX("Опрос ЭБУ...", FONT_20_CHAR_WIDTH), 120, "Опрос ЭБУ...",
                                COLOR_YELLOW, COLOR_TRANSPARENT,
                                FONT_20_ADDR, FONT_20_CHAR_WIDTH, FONT_20_CHAR_HEIGHT);
        return;
    }
    if (!g_diag_dtc_result) {
        const char *m = "OK = прочитать ошибки";
        ILI9481_DrawString_QSPI(CenterX(m, FONT_20_CHAR_WIDTH), 120, m,
                                COLOR_WHITE, COLOR_TRANSPARENT,
                                FONT_20_ADDR, FONT_20_CHAR_WIDTH, FONT_20_CHAR_HEIGHT);
        ILI9481_FillRect(0,440,ILI9481_WIDTH,40,g_theme.header_color);
        ILI9481_DrawString_QSPI(CenterX("Назад", FONT_16_CHAR_WIDTH), 450, "Назад",
                                COLOR_WHITE, COLOR_TRANSPARENT,
                                FONT_16_ADDR, FONT_16_CHAR_WIDTH, FONT_16_CHAR_HEIGHT);
        return;
    }
    if (g_diag_dtc_result == 2) {
        const char *m = "ЭБУ молчит";
        ILI9481_DrawString_QSPI(CenterX(m, FONT_20_CHAR_WIDTH), 120, m,
                                COLOR_RED, COLOR_TRANSPARENT,
                                FONT_20_ADDR, FONT_20_CHAR_WIDTH, FONT_20_CHAR_HEIGHT);
        ILI9481_FillRect(0,440,ILI9481_WIDTH,40,g_theme.header_color);
        ILI9481_DrawString_QSPI(CenterX("Назад", FONT_16_CHAR_WIDTH), 450, "Назад",
                                COLOR_WHITE, COLOR_TRANSPARENT,
                                FONT_16_ADDR, FONT_16_CHAR_WIDTH, FONT_16_CHAR_HEIGHT);
        return;
    }
    if (g_diag_dtcs.count == 0) {
        const char *m = "Ошибок нет";
        ILI9481_DrawString_QSPI(CenterX(m, FONT_20_CHAR_WIDTH), 120, m,
                                COLOR_GREEN, COLOR_TRANSPARENT,
                                FONT_20_ADDR, FONT_20_CHAR_WIDTH, FONT_20_CHAR_HEIGHT);
        ILI9481_FillRect(0,440,ILI9481_WIDTH,40,g_theme.header_color);
        ILI9481_DrawString_QSPI(CenterX("Назад", FONT_16_CHAR_WIDTH), 450, "Назад",
                                COLOR_WHITE, COLOR_TRANSPARENT,
                                FONT_16_ADDR, FONT_16_CHAR_WIDTH, FONT_16_CHAR_HEIGHT);
        return;
    }

    // Список ошибок (крупный шрифт FONT_20), каждая строка по центру
    uint16_t y = 70;
    const uint16_t ROW_H = 30;
    uint8_t max_vis = 11;
    uint8_t scroll = 0;
    if (ui_state.menu_idx >= max_vis) scroll = ui_state.menu_idx - max_vis + 1;

    for (uint8_t i = 0; i < g_diag_dtcs.count; i++) {
        if (i < scroll || i >= scroll + max_vis) continue;
        int16_t draw_y = y + (i - scroll) * ROW_H;
        uint8_t is_sel = (i == ui_state.menu_idx);
        if (is_sel) {
            ILI9481_FillRect(0, draw_y, ILI9481_WIDTH, ROW_H, g_theme.cursor_color);
        }
        char line[32];
        snprintf(line, sizeof(line), "%05d-%02d-00",
                 g_diag_dtcs.list[i].code, g_diag_dtcs.list[i].type);
        ILI9481_DrawString_QSPI(CenterX(line, FONT_20_CHAR_WIDTH), draw_y + 5, line,
                                is_sel ? COLOR_WHITE : COLOR_GRAY,
                                COLOR_TRANSPARENT,
                                FONT_20_ADDR, FONT_20_CHAR_WIDTH, FONT_20_CHAR_HEIGHT);
    }

    // Кнопки внизу — тоже по центру
    ILI9481_FillRect(0, 400, ILI9481_WIDTH, 80, COLOR_BLACK);
    uint8_t btn_clear_idx = g_diag_dtcs.count;
    if (ui_state.menu_idx == btn_clear_idx) {
        ILI9481_FillRect(0, 400, ILI9481_WIDTH, 40, g_theme.cursor_color);
        ILI9481_DrawString_QSPI(CenterX("Стереть", FONT_20_CHAR_WIDTH), 410, "Стереть",
                                COLOR_WHITE, COLOR_TRANSPARENT,
                                FONT_20_ADDR, FONT_20_CHAR_WIDTH, FONT_20_CHAR_HEIGHT);
    } else {
        ILI9481_DrawString_QSPI(CenterX("Стереть", FONT_20_CHAR_WIDTH), 410, "Стереть",
                                COLOR_GRAY, COLOR_TRANSPARENT,
                                FONT_20_ADDR, FONT_20_CHAR_WIDTH, FONT_20_CHAR_HEIGHT);
    }
    uint8_t btn_back_idx = g_diag_dtcs.count + 1;
    if (ui_state.menu_idx == btn_back_idx) {
        ILI9481_FillRect(0, 440, ILI9481_WIDTH, 40, g_theme.cursor_color);
        ILI9481_DrawString_QSPI(CenterX("Назад", FONT_20_CHAR_WIDTH), 450, "Назад",
                                COLOR_WHITE, COLOR_TRANSPARENT,
                                FONT_20_ADDR, FONT_20_CHAR_WIDTH, FONT_20_CHAR_HEIGHT);
    } else {
        ILI9481_DrawString_QSPI(CenterX("Назад", FONT_20_CHAR_WIDTH), 450, "Назад",
                                COLOR_GRAY, COLOR_TRANSPARENT,
                                FONT_20_ADDR, FONT_20_CHAR_WIDTH, FONT_20_CHAR_HEIGHT);
    }
}

/* ============================================================================
ДЕТАЛИ ОШИБКИ (расшифровка из таблицы)
============================================================================ */
typedef struct { uint16_t code; const char *desc; } DtcEntry_t;

static const DtcEntry_t DTC_DB[] = {
    {   152, "Датчик температуры воздуха на впуске -G299-: слишком высокий уровень сигнала" },
    {   290, "Лямбда-зонд (ряд 1): недостоверный сигнал" },
    {   546, "Датчик температуры наружного воздуха: недостоверный сигнал" },
    {  1032, "Система вентиляции картера: механическая неисправность" },
    {  1165, "Блок управления дроссельной заслонкой -J338-: не выполнена базовая настройка. Выполните адаптацию дросселя (канал 060 или 098)" },
    {  2775, "Контрольная лампа MIL: электрическая неисправность" },
    {  8212, "Электромагнитный клапан 1 абсорбера (EVAP): обрыв цепи" },
    {  8482, "Клапан управления дроссельной заслонкой: электрическая неисправность" },
    {  8487, "Потенциометр дроссельной заслонки: недостоверный сигнал" },
    { 16505, "Датчик положения дроссельной заслонки -G69-: недостоверный сигнал (P0121). Проверьте потенциометр и проводку" },
    { 17952, "Датчик положения дроссельной заслонки 1 -G187/G69-: сигнал вне диапазона (P1544). Проверьте разъём и проводку дросселя" },
    { 17978, "Пуск двигателя заблокирован иммобилайзером (P1570). Проверьте ключ, кольцевую антенну иммобилайзера и привязку ЭБУ" },
};

void UI_DrawDiag_DtcDetail(void) {
    DrawBg(g_current_bg_idx);
    ILI9481_FillRect(0,0,ILI9481_WIDTH,40,g_theme.header_color);

    char header[32];
    const char* ecu_name = "ЭБУ";
    for(uint8_t i=0; i<KLINE_ECU_COUNT; i++) {
        if(KLINE_ECU_LIST[i].addr == g_diag_addr) { ecu_name = KLINE_ECU_LIST[i].name; break; }
    }
    snprintf(header, sizeof(header), "%02X %s", g_diag_addr, ecu_name);
    ILI9481_DrawString_QSPI(CenterX(header, FONT_20_CHAR_WIDTH), 12, header,
                            COLOR_WHITE, COLOR_TRANSPARENT,
                            FONT_20_ADDR, FONT_20_CHAR_WIDTH, FONT_20_CHAR_HEIGHT);
    ILI9481_DrawString_QSPI(CenterX("Ошибка", FONT_20_CHAR_WIDTH), 45, "Ошибка",
                            COLOR_WHITE, COLOR_TRANSPARENT,
                            FONT_20_ADDR, FONT_20_CHAR_WIDTH, FONT_20_CHAR_HEIGHT);

    if (g_diag_dtc_selected_idx >= g_diag_dtcs.count) {
        const char *m = "Ошибка не найдена";
        ILI9481_DrawString_QSPI(CenterX(m, FONT_20_CHAR_WIDTH), 120, m,
                                COLOR_RED, COLOR_TRANSPARENT,
                                FONT_20_ADDR, FONT_20_CHAR_WIDTH, FONT_20_CHAR_HEIGHT);
    } else {
        KLineDtc_t *d = &g_diag_dtcs.list[g_diag_dtc_selected_idx];

        // Код ошибки крупно — по центру
        char code_str[32];
        snprintf(code_str, sizeof(code_str), "%05d-%02d-00", d->code, d->type);
        ILI9481_DrawString_QSPI(CenterX(code_str, FONT_24_CHAR_WIDTH), 80, code_str,
                                COLOR_YELLOW, COLOR_TRANSPARENT,
                                FONT_24_ADDR, FONT_24_CHAR_WIDTH, FONT_24_CHAR_HEIGHT);

        // Поиск описания в таблице
        const char* desc = NULL;
        for (uint8_t i = 0; i < sizeof(DTC_DB)/sizeof(DTC_DB[0]); i++) {
            if (DTC_DB[i].code == d->code) { desc = DTC_DB[i].desc; break; }
        }
        if (!desc) desc = (strlen(d->txt) > 4) ? d->txt : "Нет описания в базе";

        // Перенос слов + ЦЕНТРИРОВАНИЕ каждой полученной строки
        uint16_t y = 125;
        const uint16_t max_width = ILI9481_WIDTH - 40;
        const uint16_t line_height = 22;
        char buffer[160];
        strncpy(buffer, desc, sizeof(buffer));
        buffer[sizeof(buffer)-1] = 0;

        char line_buf[64] = {0};
        char *word = strtok(buffer, " ");
        while (word != NULL) {
            if (strlen(line_buf) + strlen(word) + 1 < sizeof(line_buf)) {
                uint16_t cur_w = strlen(line_buf) * FONT_16_CHAR_WIDTH;
                uint16_t wrd_w = strlen(word)   * FONT_16_CHAR_WIDTH;
                if (cur_w + wrd_w > max_width && strlen(line_buf) > 0) {
                    ILI9481_DrawString_QSPI(CenterX(line_buf, FONT_16_CHAR_WIDTH), y, line_buf,
                                            COLOR_WHITE, COLOR_TRANSPARENT,
                                            FONT_16_ADDR, FONT_16_CHAR_WIDTH, FONT_16_CHAR_HEIGHT);
                    y += line_height;
                    line_buf[0] = 0;
                }
                if (strlen(line_buf) > 0) strcat(line_buf, " ");
                strcat(line_buf, word);
            }
            word = strtok(NULL, " ");
        }
        if (strlen(line_buf) > 0) {
            ILI9481_DrawString_QSPI(CenterX(line_buf, FONT_16_CHAR_WIDTH), y, line_buf,
                                    COLOR_WHITE, COLOR_TRANSPARENT,
                                    FONT_16_ADDR, FONT_16_CHAR_WIDTH, FONT_16_CHAR_HEIGHT);
        }
    }

    ILI9481_FillRect(0, 440, ILI9481_WIDTH, 40, g_theme.header_color);
    ILI9481_DrawString_QSPI(CenterX("Назад", FONT_16_CHAR_WIDTH), 450, "Назад",
                            COLOR_WHITE, COLOR_TRANSPARENT,
                            FONT_16_ADDR, FONT_16_CHAR_WIDTH, FONT_16_CHAR_HEIGHT);
}

void UI_DrawDiag_ClearErr(void) {
    DrawBg(g_current_bg_idx);
    ILI9481_FillRect(0,0,ILI9481_WIDTH,40,g_theme.header_color);
    ILI9481_DrawString_QSPI(60,12,"СТЕРЕТЬ ОШИБКИ",COLOR_WHITE,COLOR_TRANSPARENT,
                            FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
    if (g_diag_dtc_busy) {
        ILI9481_DrawString_QSPI(80,120,"Стирание...",COLOR_YELLOW,COLOR_TRANSPARENT,
                                FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
        ILI9481_DrawString_QSPI(80,150,"подождите ~3 сек",COLOR_GRAY,COLOR_TRANSPARENT,
                                FONT_16_ADDR,FONT_16_CHAR_WIDTH,FONT_16_CHAR_HEIGHT);
    } else if (g_diag_clr_result == 1) {
        ILI9481_DrawString_QSPI(50,120,"Ошибки стерты!",COLOR_GREEN,COLOR_TRANSPARENT,
                                FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
    } else if (g_diag_clr_result == 2) {
        ILI9481_DrawString_QSPI(60,120,"Ошибка стирания",COLOR_RED,COLOR_TRANSPARENT,
                                FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
        ILI9481_DrawString_QSPI(60,150,"OK = повторить",COLOR_GRAY,COLOR_TRANSPARENT,
                                FONT_16_ADDR,FONT_16_CHAR_WIDTH,FONT_16_CHAR_HEIGHT);
    } else {
        ILI9481_DrawString_QSPI(30,120,"OK = стереть все ошибки",COLOR_WHITE,COLOR_TRANSPARENT,
                                FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
        ILI9481_DrawString_QSPI(30,150,"ВНИЗ = назад",COLOR_GRAY,COLOR_TRANSPARENT,
                                FONT_16_ADDR,FONT_16_CHAR_WIDTH,FONT_16_CHAR_HEIGHT);
    }
    ILI9481_FillRect(0,440,ILI9481_WIDTH,40,g_theme.header_color);
    ILI9481_DrawString_QSPI(20,450,"OK=стереть   ВНИЗ=назад",COLOR_WHITE,COLOR_TRANSPARENT,
                            FONT_16_ADDR,FONT_16_CHAR_WIDTH,FONT_16_CHAR_HEIGHT);
}

extern uint8_t g_calib_edit;

static void DrawCalibRow(uint16_t y, uint8_t sel, uint8_t ed, const char* name, const char* val){
    if(sel) ILI9481_FillRect(0,y,ILI9481_WIDTH,40, ed?COLOR_YELLOW:g_theme.cursor_color);
    ILI9481_DrawString_QSPI(12, y+10, name, sel?(ed?COLOR_BLACK:COLOR_WHITE):COLOR_GRAY,
                            COLOR_TRANSPARENT, FONT_20_ADDR, FONT_20_CHAR_WIDTH, FONT_20_CHAR_HEIGHT);
    int16_t x = ILI9481_WIDTH - 10 - (int16_t)strlen(val)*FONT_20_CHAR_WIDTH;
    ILI9481_DrawString_QSPI(x, y+10, val, sel?(ed?COLOR_BLACK:COLOR_WHITE):COLOR_CYAN,
                            COLOR_TRANSPARENT, FONT_20_ADDR, FONT_20_CHAR_WIDTH, FONT_20_CHAR_HEIGHT);
}

void UI_DrawParam_Calib(void){
    DrawBg(g_current_bg_idx);
    ILI9481_FillRect(0,0,ILI9481_WIDTH,40,g_theme.header_color);
    ILI9481_DrawString_QSPI(CenterX("КАЛИБРОВКА", FONT_20_CHAR_WIDTH),12,"КАЛИБРОВКА",
                            COLOR_WHITE,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
    char v[24]; uint16_t y=50;
    snprintf(v,sizeof(v),"%+.2f В",g_calib.vbat_offset);
    DrawCalibRow(y,(ui_state.menu_idx==0),(g_calib_edit&&ui_state.menu_idx==0),"Напряж.АКБ",v); y+=40;
    DrawCalibRow(y,(ui_state.menu_idx==1),(g_calib_edit&&ui_state.menu_idx==1),"Ист.скорости",
                 (g_calib.speed_src==SPEED_SRC_KLINE)?"K-Line":"CAN"); y+=40;
    snprintf(v,sizeof(v),"%.1f%%",g_calib.speed_coef*100.0f);
    DrawCalibRow(y,(ui_state.menu_idx==2),(g_calib_edit&&ui_state.menu_idx==2),"Скорость",v); y+=40;
    snprintf(v,sizeof(v),"%.1f%%",g_calib.fuel_coef*100.0f);
    DrawCalibRow(y,(ui_state.menu_idx==3),(g_calib_edit&&ui_state.menu_idx==3),"Расход",v); y+=40;
    snprintf(v,sizeof(v),"пара %.3f",g_calib.final_drive);
    DrawCalibRow(y,(ui_state.menu_idx==4),0,"МКПП",v); y+=40;
    DrawCalibRow(y,(ui_state.menu_idx==5),0,"Назад",""); y+=40;
}

void UI_DrawParam_CalibMkpp(void){
    DrawBg(g_current_bg_idx);
    ILI9481_FillRect(0,0,ILI9481_WIDTH,40,g_theme.header_color);
    ILI9481_DrawString_QSPI(CenterX("КАЛИБРОВКА МКПП", FONT_20_CHAR_WIDTH),12,"КАЛИБРОВКА МКПП",
                            COLOR_WHITE,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
    char v[24]; uint16_t y=50;
    snprintf(v,sizeof(v),"%.3f",g_calib.final_drive);
    DrawCalibRow(y,(ui_state.menu_idx==0),(g_calib_edit&&ui_state.menu_idx==0),"Главная пара",v); y+=40;
    snprintf(v,sizeof(v),"%u мм",g_calib.wheel_mm);
    DrawCalibRow(y,(ui_state.menu_idx==1),(g_calib_edit&&ui_state.menu_idx==1),"Колесо",v); y+=40;
    for(uint8_t g=0; g<6; g++){
        snprintf(v,sizeof(v),"%.3f",g_calib.ratios[g]);
        char nm[16]; snprintf(nm,sizeof(nm),"%u передача",g+1);
        DrawCalibRow(y,(ui_state.menu_idx==2+g),(g_calib_edit&&ui_state.menu_idx==2+g),nm,v); y+=40;
    }
    DrawCalibRow(y,(ui_state.menu_idx==8),0,"Назад",""); y+=40;

}

/* Мигающая стрелка-подсказка (треугольник + ножка), cy = нижняя граница */
static void DrawShiftArrow(uint16_t cx, uint16_t cy, uint8_t up, uint16_t color){
    const uint16_t TH=16, TW=28;
    if(up){
        for(uint16_t i=0;i<TH;i++){
            uint16_t w=(TW*(i+1))/TH;
            ILI9481_FillRect(cx-w/2, cy-26+i, w, 1, color);
        }
        ILI9481_FillRect(cx-4, cy-10, 8, 10, color);
    } else {
        ILI9481_FillRect(cx-4, cy-26, 8, 10, color);
        for(uint16_t i=0;i<TH;i++){
            uint16_t w=(TW*(TH-i))/TH;
            ILI9481_FillRect(cx-w/2, cy-16+i, w, 1, color);
        }
    }
}

void UI_DrawParam_Shift(void){
    DrawBg(g_current_bg_idx);
    ILI9481_FillRect(0,0,ILI9481_WIDTH,40,g_theme.header_color);
    ILI9481_DrawString_QSPI(CenterX("Перекл. передач", FONT_20_CHAR_WIDTH),12,"Перекл. передач",
                            COLOR_WHITE,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
    uint8_t m   = ui_state.menu_idx;
    uint16_t bgc = g_shift_edit ? COLOR_YELLOW : g_theme.cursor_color;
    uint16_t fg  = g_shift_edit ? COLOR_BLACK  : COLOR_WHITE;

    /* 0: Включено */
    if(m==0) ILI9481_FillRect(0,50,ILI9481_WIDTH,40,bgc);
    DrawCheckbox(15,60,g_shift_assist_enabled);
    ILI9481_DrawString_QSPI(45,62,"Включено",(m==0)?fg:COLOR_WHITE,COLOR_TRANSPARENT,
                            FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);

    /* 1..3: пороги крупными цифрами */
    const char* lbl[3] = {"Переключение вверх","Переключение вниз","Критичные обороты"};
    uint16_t val[3]    = {g_shift_up_rpm, g_shift_down_rpm, g_shift_crit_rpm};
    uint16_t y = 95;
    for(uint8_t i=0;i<3;i++){
        if(m==i+1) ILI9481_FillRect(0,y-3,ILI9481_WIDTH,66,bgc);
        ILI9481_DrawString_QSPI(CenterX(lbl[i], FONT_20_CHAR_WIDTH),y,lbl[i],
                                (m==i+1)?fg:COLOR_WHITE,COLOR_TRANSPARENT,
                                FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
        char v[8]; snprintf(v,sizeof(v),"%u",val[i]);
        ILI9481_DrawString_QSPI(CenterX(v, FONT_40_CHAR_WIDTH),y+24,v,
                                (m==i+1)?fg:COLOR_WHITE,COLOR_TRANSPARENT,
                                FONT_40_ADDR,FONT_40_CHAR_WIDTH,FONT_40_CHAR_HEIGHT);
        y += 65;
    }

    /* 4: Назад */
    ILI9481_FillRect(0,440,ILI9481_WIDTH,40,COLOR_BLACK);
    if(m==4){
        ILI9481_FillRect(0,440,ILI9481_WIDTH,40,g_theme.cursor_color);
        ILI9481_DrawString_QSPI(CenterX("Назад", FONT_20_CHAR_WIDTH),450,"Назад",
                                COLOR_WHITE,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
    } else {
        ILI9481_DrawString_QSPI(CenterX("Назад", FONT_20_CHAR_WIDTH),450,"Назад",
        		COLOR_WHITE,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
    }
}

void UI_DrawParam_Misc(void){
    extern uint8_t g_misc_edit;
    DrawBg(g_current_bg_idx);
    ILI9481_FillRect(0,0,ILI9481_WIDTH,40,g_theme.header_color);
    ILI9481_DrawString_QSPI(CenterX("Разное", FONT_20_CHAR_WIDTH),12,"Разное",COLOR_WHITE,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
    uint8_t m = ui_state.menu_idx;
    uint16_t bgc = g_misc_edit ? COLOR_YELLOW : g_theme.cursor_color;
    uint16_t fg  = g_misc_edit ? COLOR_BLACK  : COLOR_WHITE;
    uint16_t y = 50;
    /* 0: Таймаут меню */
    if(m==0) ILI9481_FillRect(0,y,ILI9481_WIDTH,40,bgc);
    DrawCheckbox(15,y+10,g_misc.menu_to_en);
    ILI9481_DrawString_QSPI(45,y+12,"Таймаут меню",(m==0)?fg:COLOR_WHITE,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT); y+=40;
    /* 1: значение */
    { char v[12]; snprintf(v,sizeof(v),"%02u сек",g_misc.menu_to_sec);
      if(m==1) ILI9481_FillRect(0,y,ILI9481_WIDTH,36,bgc);
      ILI9481_DrawString_QSPI(CenterX(v,FONT_20_CHAR_WIDTH),y+8,v,(m==1)?fg:COLOR_WHITE,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT); y+=36; }
    /* 2: Таймаут сообщ. */
    if(m==2) ILI9481_FillRect(0,y,ILI9481_WIDTH,40,bgc);
    DrawCheckbox(15,y+10,g_misc.msg_to_en);
    ILI9481_DrawString_QSPI(45,y+12,"Таймаут сообщ.",(m==2)?fg:COLOR_WHITE,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT); y+=40;
    /* 3: значение */
    { char v[12]; snprintf(v,sizeof(v),"%02u сек",g_misc.msg_to_sec);
      if(m==3) ILI9481_FillRect(0,y,ILI9481_WIDTH,36,bgc);
      ILI9481_DrawString_QSPI(CenterX(v,FONT_20_CHAR_WIDTH),y+8,v,(m==3)?fg:COLOR_WHITE,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT); y+=36; }
    /* 4..6 */
    static const char* nm[3] = {"Мигание иконок","Ремень безоп.","Упр. зеркалом"};
    uint8_t fl[3] = {g_misc.icon_blink, g_misc.belt_en, g_misc.mirror_en};
    for(uint8_t i=0;i<3;i++){
        if(m==4+i) ILI9481_FillRect(0,y,ILI9481_WIDTH,40,bgc);
        DrawCheckbox(15,y+10,fl[i]);
        ILI9481_DrawString_QSPI(45,y+12,nm[i],(m==4+i)?fg:COLOR_WHITE,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
        y+=40;
    }
    ILI9481_FillRect(0,440,ILI9481_WIDTH,40,COLOR_BLACK);
    if(m==7){
        ILI9481_FillRect(0,440,ILI9481_WIDTH,40,g_theme.cursor_color);
        ILI9481_DrawString_QSPI(CenterX("Назад",FONT_20_CHAR_WIDTH),450,"Назад",
                                COLOR_WHITE,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
    } else {
        ILI9481_DrawString_QSPI(CenterX("Назад",FONT_20_CHAR_WIDTH),450,"Назад",
        		COLOR_WHITE,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
    }
}

static void DrawMsgFull(uint8_t id, uint8_t from_menu){
    uint16_t col = SevColor(Msg_Sev(id));
    ILI9481_FillRect(0,0,ILI9481_WIDTH,ILI9481_HEIGHT,COLOR_BLACK);
    ILI9481_FillRect(0,0,ILI9481_WIDTH,40,g_theme.header_color);
    ILI9481_DrawString_QSPI(CenterX(from_menu?"Сообщения":"ВНИМАНИЕ",FONT_20_CHAR_WIDTH),12,
                            from_menu?"Сообщения":"ВНИМАНИЕ",COLOR_WHITE,COLOR_TRANSPARENT,
                            FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
    DrawFaultIcon(160,110,Msg_Icon(id),col,2);                 /* крупная пиктограмма */
    /* текст цветом важности, перенос + центрирование */
    uint16_t y=190; const uint16_t maxw=ILI9481_WIDTH-40, lh=26;
    char buf[128]; strncpy(buf,Msg_Text(id),sizeof(buf)); buf[sizeof(buf)-1]=0;
    char line[64]={0}; char *w=strtok(buf," ");
    while(w){
        if(strlen(line)+strlen(w)+1<sizeof(line)){
            if(strlen(line)*FONT_20_CHAR_WIDTH+strlen(w)*FONT_20_CHAR_WIDTH>maxw && strlen(line)>0){
                ILI9481_DrawString_QSPI(CenterX(line,FONT_20_CHAR_WIDTH),y,line,col,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
                y+=lh; line[0]=0;
            }
            if(strlen(line)>0) strcat(line," ");
            strcat(line,w);
        }
        w=strtok(NULL," ");
    }
    if(strlen(line)>0) ILI9481_DrawString_QSPI(CenterX(line,FONT_20_CHAR_WIDTH),y,line,col,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
    ILI9481_FillRect(0,440,ILI9481_WIDTH,40,g_theme.header_color);
    char f[48];
    if(from_menu) snprintf(f,sizeof(f),"Назад");
    else if(Msg_Sev(id)==MSG_SEV_CRIT || !g_misc.msg_to_en) snprintf(f,sizeof(f),"ОК = сброс");
    else snprintf(f,sizeof(f),"ОК = сброс · авто через %u с",g_misc.msg_to_sec);
    ILI9481_DrawString_QSPI(CenterX(f,FONT_16_CHAR_WIDTH),452,f,COLOR_WHITE,COLOR_TRANSPARENT,FONT_16_ADDR,FONT_16_CHAR_WIDTH,FONT_16_CHAR_HEIGHT);
}
void UI_DrawMsgView(void){ DrawMsgFull(g_msg_view_id, 1); }

void UI_DrawMsgList(void){
    DrawBg(g_current_bg_idx);
    ILI9481_FillRect(0,0,ILI9481_WIDTH,40,g_theme.header_color);
    ILI9481_DrawString_QSPI(CenterX("Сообщения",FONT_20_CHAR_WIDTH),12,"Сообщения",COLOR_WHITE,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
    uint8_t ids[MSG_COUNT]; uint8_t n=0;
    for(uint8_t i=0;i<MSG_COUNT;i++) if(Msg_IsActive(i)) ids[n++]=i;
    uint16_t y=50; const uint16_t H=40;
    if(n==0){
        ILI9481_DrawString_QSPI(CenterX("Нет сообщений",FONT_20_CHAR_WIDTH),90,"Нет сообщений",COLOR_GREEN,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
    }
    for(uint8_t i=0;i<n;i++){
        if(ui_state.menu_idx==i) ILI9481_FillRect(0,y,ILI9481_WIDTH,H,g_theme.cursor_color);
        DrawWarnIcon(22,y+20,SevColor(Msg_Sev(ids[i])),0);
        char t[24]; strncpy(t,Msg_Text(ids[i]),20); t[20]=0; strcat(t,"...");
        ILI9481_DrawString_QSPI(40,y+12,t,COLOR_WHITE,COLOR_TRANSPARENT,FONT_16_ADDR,FONT_16_CHAR_WIDTH,FONT_16_CHAR_HEIGHT);
        if(Msg_IsAcked(ids[i])) ILI9481_DrawString_QSPI(290,y+12,"OK",COLOR_GRAY,COLOR_TRANSPARENT,FONT_16_ADDR,FONT_16_CHAR_WIDTH,FONT_16_CHAR_HEIGHT);
        y+=H;
    }
    if(ui_state.menu_idx==n) ILI9481_FillRect(0,y,ILI9481_WIDTH,H,g_theme.cursor_color);
    ILI9481_DrawString_QSPI(40,y+12,"Тестовое сообщение",COLOR_GRAY,COLOR_TRANSPARENT,FONT_16_ADDR,FONT_16_CHAR_WIDTH,FONT_16_CHAR_HEIGHT); y+=H;
    ILI9481_FillRect(0,440,ILI9481_WIDTH,40,COLOR_BLACK);
    if(ui_state.menu_idx==n+1){
        ILI9481_FillRect(0,440,ILI9481_WIDTH,40,g_theme.cursor_color);
        ILI9481_DrawString_QSPI(CenterX("Назад",FONT_20_CHAR_WIDTH),450,"Назад",
                                COLOR_WHITE,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
    } else {
        ILI9481_DrawString_QSPI(CenterX("Назад",FONT_20_CHAR_WIDTH),450,"Назад",
                                COLOR_GRAY,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
    }
}

static void DrawCarTop(uint16_t cx, uint16_t cy){
  ILI9481_FillRect(cx-32, cy-70, 64, 140, COLOR_WHITE);
  ILI9481_FillRect(cx-26, cy-40, 52, 24, 0x1082);
  ILI9481_FillRect(cx-26, cy+28, 52, 18, 0x1082);
  ILI9481_FillRect(cx-26, cy-8,  52, 30, 0x8410);
  ILI9481_FillRect(cx-38, cy-52, 6, 14, COLOR_WHITE);
  ILI9481_FillRect(cx+32, cy-52, 6, 14, COLOR_WHITE);
}
static const char* TPMS_WNAME[4] = {"Перед лев","Перед прав","Задн лев","Задн прав"};

void UI_DrawServ_Tpms(void){
  DrawBg(g_current_bg_idx);
  ILI9481_FillRect(0,0,ILI9481_WIDTH,40,g_theme.header_color);
  ILI9481_DrawString_QSPI(CenterX("TPMS",FONT_20_CHAR_WIDTH),12,"TPMS",COLOR_WHITE,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
  DrawCarTop(160,210);
  const int16_t XS[4]={24,232,24,232}, YS[4]={70,70,250,250};
  for(uint8_t i=0;i<4;i++){
    char v[16], t[16];
    uint16_t col = COLOR_WHITE;
    if(!g_tpms.en || !g_tpms.calibrated) snprintf(v,sizeof(v),"--");
    else { snprintf(v,sizeof(v),"%.1f",g_tpms_p_est[i]); if(g_tpms.warn[i]) col=COLOR_RED; }
    ILI9481_DrawString_QSPI(XS[i],YS[i],v,col,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
    snprintf(t,sizeof(t),"%.0f°C",g_tpms_temp);
    ILI9481_DrawString_QSPI(XS[i],YS[i]+26,t,COLOR_GRAY,COLOR_TRANSPARENT,FONT_16_ADDR,FONT_16_CHAR_WIDTH,FONT_16_CHAR_HEIGHT);
  }
  const char* st; uint16_t sc=COLOR_GRAY;
  if(!g_tpms.en) st="Выключено";
  else if(g_tpms.state==TPMS_ST_CAL_FULL)      { st="Обучение: 30-80 км/ч прямо"; sc=COLOR_YELLOW; }
  else if(g_tpms.state==TPMS_ST_CAL_WHEEL)     { st="Обучение после замены колеса"; sc=COLOR_YELLOW; }
  else if(!g_tpms.calibrated)                  { st="Нет эталона: Сохр. давление"; sc=COLOR_YELLOW; }
  else                                          { st="Контроль давления"; sc=COLOR_GREEN; }
  ILI9481_DrawString_QSPI(CenterX(st,FONT_16_CHAR_WIDTH),360,st,sc,COLOR_TRANSPARENT,FONT_16_ADDR,FONT_16_CHAR_WIDTH,FONT_16_CHAR_HEIGHT);
  uint8_t m=ui_state.menu_idx;
  if(m==0) ILI9481_FillRect(0,400,ILI9481_WIDTH,40,g_theme.cursor_color);
  ILI9481_DrawString_QSPI(CenterX("Параметры",FONT_20_CHAR_WIDTH),410,"Параметры",(m==0)?COLOR_WHITE:COLOR_GRAY,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
  ILI9481_FillRect(0,440,ILI9481_WIDTH,40,COLOR_BLACK);
  if(m==1){
      ILI9481_FillRect(0,440,ILI9481_WIDTH,40,g_theme.cursor_color);
      ILI9481_DrawString_QSPI(CenterX("Назад",FONT_20_CHAR_WIDTH),450,"Назад",COLOR_WHITE,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
  } else {
      ILI9481_DrawString_QSPI(CenterX("Назад",FONT_20_CHAR_WIDTH),450,"Назад",COLOR_WHITE,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
  }
}
void UI_DrawTpms_Param(void){
  DrawBg(g_current_bg_idx);
  ILI9481_FillRect(0,0,ILI9481_WIDTH,40,g_theme.header_color);
  ILI9481_DrawString_QSPI(CenterX("TPMS",FONT_20_CHAR_WIDTH),12,"TPMS",COLOR_WHITE,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
  const char* items[4]={"Включено","Сохр. давление","Замена колеса","Назад"};
  uint16_t y=50;
  for(uint8_t i=0;i<4;i++){
    if(i==ui_state.menu_idx) ILI9481_FillRect(0,y,ILI9481_WIDTH,40,g_theme.cursor_color);
    if(i==0) DrawCheckbox(15,y+10,g_tpms.en);
    ILI9481_DrawString_QSPI(i==0?45:20,y+12,items[i],COLOR_WHITE,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
    y+=40;
  }
}
void UI_DrawTpms_Wheel(void){
  DrawBg(g_current_bg_idx);
  ILI9481_FillRect(0,0,ILI9481_WIDTH,40,g_theme.header_color);
  ILI9481_DrawString_QSPI(CenterX("Какое колесо меняли?",FONT_20_CHAR_WIDTH),12,"Какое колесо меняли?",COLOR_WHITE,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
  uint16_t y=60;
  for(uint8_t i=0;i<5;i++){
    if(i==ui_state.menu_idx) ILI9481_FillRect(0,y,ILI9481_WIDTH,40,g_theme.cursor_color);
    const char* nm = (i<4)?TPMS_WNAME[i]:"Назад";
    ILI9481_DrawString_QSPI(20,y+12,nm,COLOR_WHITE,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
    y+=40;
  }
}

void UI_DrawTpms_Press(void){
    DrawBg(g_current_bg_idx);
    ILI9481_FillRect(0,0,ILI9481_WIDTH,40,g_theme.header_color);
    ILI9481_DrawString_QSPI(CenterX("Эталонное давление",FONT_20_CHAR_WIDTH),12,"Эталонное давление",
                            COLOR_WHITE,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
    uint16_t y=50; char v[16];
    for(uint8_t i=0;i<4;i++){
        uint8_t sel=(ui_state.menu_idx==i), ed=sel&&g_tpms_press_edit;
        if(sel) ILI9481_FillRect(0,y,ILI9481_WIDTH,40, ed?COLOR_YELLOW:g_theme.cursor_color);
        ILI9481_DrawString_QSPI(20,y+12,TPMS_WNAME[i], ed?COLOR_BLACK:(sel?COLOR_WHITE:COLOR_GRAY),
                                COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
        snprintf(v,sizeof(v),"%.1f бар",g_tpms.p_ref[i]);
        int16_t x = ILI9481_WIDTH-10-(int16_t)strlen(v)*FONT_20_CHAR_WIDTH;
        ILI9481_DrawString_QSPI(x,y+12,v, ed?COLOR_BLACK:(sel?COLOR_WHITE:COLOR_CYAN),
                                COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
        y+=40;
    }
    if(ui_state.menu_idx==4) ILI9481_FillRect(0,y,ILI9481_WIDTH,40,g_theme.cursor_color);
    ILI9481_DrawString_QSPI(20,y+12, g_tpms_press_mode?"Обучить это колесо":"Сохранить и обучить",
                            (ui_state.menu_idx==4)?COLOR_WHITE:COLOR_GRAY,
                            COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
    y+=44;
    ILI9481_DrawString_QSPI(CenterX("ОК=изм. / ОК=готово",FONT_16_CHAR_WIDTH),y,
                            "ОК=изм. / ОК=готово",COLOR_GRAY,COLOR_TRANSPARENT,
                            FONT_16_ADDR,FONT_16_CHAR_WIDTH,FONT_16_CHAR_HEIGHT);
    /* футер Назад: чёрный+серый когда не выбран, курсор+белый когда выбран */
    ILI9481_FillRect(0,440,ILI9481_WIDTH,40,COLOR_BLACK);
    if(ui_state.menu_idx==5){
        ILI9481_FillRect(0,440,ILI9481_WIDTH,40,g_theme.cursor_color);
        ILI9481_DrawString_QSPI(CenterX("Назад",FONT_20_CHAR_WIDTH),450,"Назад",COLOR_WHITE,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
    } else {
        ILI9481_DrawString_QSPI(CenterX("Назад",FONT_20_CHAR_WIDTH),450,"Назад",COLOR_GRAY,COLOR_TRANSPARENT,FONT_20_ADDR,FONT_20_CHAR_WIDTH,FONT_20_CHAR_HEIGHT);
    }
}








// Placeholders
void UI_DrawMeas_Extra(void){DrawPlaceholder("Дополн. данные");}
void UI_DrawMeas_Gauges(void){DrawPlaceholder("Приборы");}
void UI_DrawParam_SpeedLim(void){DrawPlaceholder("Ограничение скорости");}
void UI_DrawParam_Eco(void){DrawPlaceholder("ЭКО режим");}
void UI_DrawParam_Gearbox(void){DrawPlaceholder("Режим АКПП");}
void UI_DrawParam_Nav(void){DrawPlaceholder("Навигация");}
void UI_DrawParam_Coffee(void){DrawPlaceholder("Кофе-брэйк");}
void UI_DrawGraph_Ok(void){DrawPlaceholder("Область ОК");}
void UI_DrawGraph_Screensaver(void){DrawPlaceholder("Заставка");}
void UI_DrawGraph_Bright(void){DrawPlaceholder("яркость");}
void UI_DrawServ_Lang(void){DrawPlaceholder("язык");}
void UI_DrawServ_Units(void){DrawPlaceholder("Единицы");}
void UI_DrawServ_Light(void){DrawPlaceholder("Свет и обзор");}
void UI_DrawServ_Comfort(void){DrawPlaceholder("Комфорт");}
void UI_DrawServ_Pneumo(void){DrawPlaceholder("Пневмоподвеска");}
void UI_DrawServ_CanMon(void){DrawPlaceholder("КАН-монитор");}
void UI_DrawServ_Ss(void){DrawPlaceholder("Скринсэйв");}
void UI_DrawServ_Motor(void){DrawPlaceholder("МоторИнфо");}
void UI_DrawServ_Fw(void){DrawPlaceholder("Прошивка");}
void UI_DrawServ_Debug(void){DrawPlaceholder("Debug");}
