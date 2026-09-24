#ifndef __UI_SCREENS_H
#define __UI_SCREENS_H
#include "ui_graphics.h"
#include <stdint.h>

typedef struct { uint8_t r, g, b, focus; } ColorPicker_t;
extern UiState_t ui_state;

void UI_DrawMain(const UiState_t* state, const float* param_vals);
void UI_DrawMenuList(const char* title, const char** items, uint8_t count, uint8_t selected);
void UI_DrawWallpaper(const UiState_t* state, uint8_t selected_idx);
void UI_DrawMiddleArea(const UiState_t* state, const uint8_t* param_vis);
void UI_DrawColors(const UiState_t* state);
void UI_DrawCustomColor(const UiState_t* state, const ColorPicker_t* cp);

void UI_DrawMeas_Bc1(const float* vals);
void UI_DrawMeas_Bc2(const float* vals);
void UI_DrawMeas_General(const float* vals);
void UI_DrawMeas_Extra(void); void UI_DrawMeas_Oil(void); void UI_DrawMeas_Power(void);
void UI_DrawMeas_Accel(void); void UI_DrawMeas_Graphs(void); void UI_DrawMeas_Gauges(void);
void UI_DrawGraphsSettings(const UiState_t* state);
void UI_DrawGraphsPick(const UiState_t* state);
void UI_DrawGraphsView(const UiState_t* state);

void UI_DrawParam_SpeedLim(void); void UI_DrawParam_Eco(void); void UI_DrawParam_Calib(void);
void UI_DrawParam_Gearbox(void); void UI_DrawParam_Shift(void); void UI_DrawParam_Nav(void);
void UI_DrawParam_Coffee(void); void UI_DrawParam_Misc(void);void UI_DrawParam_CalibMkpp(void);

void UI_DrawGraph_TopSettings(const UiState_t* state);
void UI_DrawTopParams(const UiState_t* state);
void UI_DrawGraph_Ok(void); void UI_DrawGraph_BottomSettings(const UiState_t* state);
void UI_DrawBotParams(const UiState_t* state);
void UI_DrawGraph_Screensaver(void); void UI_DrawGraph_Anim(const UiState_t* state);

void UI_DrawDiag_Ecu(void);
void UI_DrawDiag_Meas(void);
void UI_DrawDiag_ReadErr(void);
void UI_DrawDiag_DtcDetail(void);
void UI_DrawDiag_ClearErr(void);

void UI_DrawMsgList(void);
void UI_DrawMsgView(void);
void UI_DrawTpms_Param(void);
void UI_DrawTpms_Wheel(void);
void UI_DrawTpms_Press(void);

void UI_DrawServ_Lang(void); void UI_DrawServ_Units(void); void UI_DrawServ_Light(void);
void UI_DrawServ_Comfort(void); void UI_DrawServ_Tpms(void); void UI_DrawServ_Pneumo(void);
void UI_DrawServ_CanMonMenu(void);
void UI_DrawServ_CanMonData(void);
void UI_DrawServ_Ss(void); void UI_DrawServ_Motor(void);
void UI_DrawServ_Fw(void); void UI_DrawServ_Debug(void);
void UI_DrawDiag_Kline(void);
#endif
