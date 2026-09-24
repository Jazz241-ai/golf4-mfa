#ifndef __SETTINGS_H
#define __SETTINGS_H
#include <stdint.h>

#define SETTINGS_ADDR     0x081E0000UL   /* Bank 2, Sector 7 */
#define SETTINGS_MAGIC    0x42434647UL   /* 'BCFG' */
#define SETTINGS_VERSION 2   /* было 2: добавлен ratios[5] */

void Settings_Load(void);   // один раз при старте (после UI_Init)
void Settings_Tick(void);   // в главный цикл: сам видит изменения и сохраняет
void Settings_SaveNow(void);// принудительное сохранение
void Settings_CheckSave(void);
#endif
