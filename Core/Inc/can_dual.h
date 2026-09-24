#ifndef __CAN_DUAL_H
#define __CAN_DUAL_H
#include "main.h"
#include <stdint.h>
#include <stdbool.h>

#define CAN_TIMEOUT_MS          1000U

/* ============================================================================
   СТРУКТУРЫ ДАННЫХ
============================================================================ */
typedef struct {
    /* базовые величины */
    float speed_kmh, rpm, coolant_temp, oil_temp, intake_temp, outside_temp;
    float fuel_level, fuel_consumption_inst, fuel_consumption_avg, fuel_range;
    float voltage, throttle_position, boost_pressure;
    float torque_req;          /* <-- ДОБАВИТЬ: момент по желанию водителя, Нм (0x280, биты 56..63) */
    float coolant_kombi;       /* <-- ДОБАВИТЬ: темп. ОЖ от приборки, °C (0x420, биты 32..39) */
    float steering_angle;
    uint8_t gear_position;              /* 1=P 2=R 3=N 4=D 5=S 6..11="1".."6" */
    bool brake_pedal, clutch_pressed, handbrake;
    bool engine_running, mil_active;
    uint32_t last_frame_time;
    /* Motor (0x280/0x288/0x380/0x588) */
    float torque_inner, torque_driver, torque_loss;
    uint8_t torque_inaccurate;
    float pedal_raw, throttle_poti;
    float idle_target_rpm, desired_rpm, rpm_gradient;
    uint8_t cruise_status;              /* GRA enum */
    uint8_t brake_light;
    /* Kombi (0x320/0x420/0x520) */
    float speed_kombi, speed_displayed, coolant_temp_kombi, outside_temp_raw;
    float odometer_km, standzeit_sec;
    uint8_t key_info;
    uint8_t blink_left, blink_right;
    uint32_t outside_time;
    /* Bremse (0x1A0/0x4A0/0x4A8) */
    float speed_brake;
    float wheel_speed[4];               /* VL, VR, HL, HR */
    uint32_t wheel_time;
    float brake_pressure, yaw_rate, brake_temp_front, msr_torque;
    bool abs_active, esp_active, asr_req, abs_diag;
    bool esp_off;          /* НОВОЕ: ESP отключена кнопкой (0x1A0, d[1] bit1) */
    /* Lenkwinkel (0xC2) */
    float steering_speed;
    /* Getriebe (0x440/0x540) */
    uint8_t gear_selector, gear_target, converter;
    uint32_t at_frame_time;
    /* ZAS / Klima (0x572/0x5E0) */
    bool key_in, ignition_on, kl_x, starter_active, kl_p, kl_15sv;
    bool ac_on; float ac_pressure, fan_load, comp_load;
    /* Airbag (0x50) */
    uint8_t crash_flags, crash_intensity;
    bool belt_sw_drv, belt_sw_pas;
    /* RDK (0x343) / WFS (0x5B8) */
    uint8_t tire_warn_mask;
    uint8_t immo_text;

} MotorCAN_Data_t;

typedef struct {
    float cabin_temp;                   /* legacy, таблицей не заполняется */
    float outdoor_temp;
    bool ac_compressor;
    uint8_t fan_speed;
    bool door_driver, door_passenger, door_rear_left, door_rear_right;
    bool trunk, hood, central_lock;
    /* BSG / BatMan (0x470/0x570/0x578) */
    float battery_voltage;
    uint8_t battery_state_board, battery_state_starter;
    uint8_t start_mode, starter_charge;
    bool undervolt;
    uint32_t last_frame_time;
} ComfortCAN_Data_t;

/* ============================================================================
   ЛАМПЫ (Faulth.txt, role = lamp)
============================================================================ */
typedef enum {
    LAMP_OIL_PRESS = 0, LAMP_OIL_DYN, LAMP_COOL_LEVEL, LAMP_COOL_HOT,
    LAMP_CHARGE, LAMP_MIL, LAMP_EPC, LAMP_CAT, LAMP_GLOW, LAMP_FUEL,
    LAMP_ABS, LAMP_ESP, LAMP_BRAKE, LAMP_HANDBRAKE, LAMP_DDS,
    LAMP_AIRBAG, LAMP_AIRBAG_OFF, LAMP_BELT_DRV, LAMP_BELT_PAS, LAMP_STEER,
    LAMP_TIRE, LAMP_IMMO, LAMP_UPSHIFT, LAMP_SHIFTLOCK, LAMP_CLUTCH,
    LAMP_AWD, LAMP_LEVEL, LAMP_EPB, LAMP_TRAILER, LAMP_LOWBEAM_L, LAMP_LOWBEAM_R,
    LAMP_COUNT
} WarnLamp_t;

/* ============================================================================
   НЕИСПРАВНОСТИ / СТАТУСЫ (Faulth.txt, role = error)
============================================================================ */
typedef enum {
    FAULT_OIL_TEMP = 0, FAULT_COOL_TEMP, FAULT_COOL_ECU, FAULT_OVERTEMP_PROT,
    FAULT_UNDERVOLT, FAULT_BATMAN_MEM, FAULT_ENGINE_MEM, FAULT_TANK,
    FAULT_ABS_DIAG, FAULT_BRAKE_MEM, FAULT_BRAKE_PRESS, FAULT_YAW_INV,
    FAULT_BOOSTER_SW, FAULT_BOOSTER_MEM, FAULT_AIRBAG_SYS, FAULT_AIRBAG_MEM,
    FAULT_STEER_MEM, FAULT_TIRE_SYS, FAULT_TIRE_RADIO, FAULT_GEAR_NOTLAUF,
    FAULT_GEAR_MEM, FAULT_AWD, FAULT_LEVEL_MEM, FAULT_DAMPER,
    FAULT_EPB_MEM, FAULT_PARK, FAULT_CLIMATE_MEM, FAULT_LIGHT_L, FAULT_LIGHT_R,
    FAULT_LIGHT_SENSOR, FAULT_WIPER_F, FAULT_WIPER_R, FAULT_ZAS_MEM, FAULT_CLUSTER_MEM,
    FAULT_COUNT, FAULT_WIPER_MEM,
} FaultFlag_t;

extern uint32_t g_warn_lamps;     /* битовая маска WarnLamp_t  */
extern uint32_t g_fault_flags;    /* битовая маска FaultFlag_t */

/* ============================================================================
   CAN-МОНИТОР
============================================================================ */
#define CAN_MONITOR_MAX_IDS  16
typedef struct {
    uint32_t id;
    uint8_t  data[8];
    uint8_t  len;
    uint8_t  bus;
    uint16_t timestamp;
    uint16_t color;
} CanMonitorFrame_t;

extern CanMonitorFrame_t g_can_log[CAN_MONITOR_MAX_IDS];
extern uint8_t           g_can_log_count;
extern uint8_t           g_can_mon_bus_select;
extern MotorCAN_Data_t   g_motor_can;
extern ComfortCAN_Data_t g_comfort_can;

/* ============================================================================
   API
============================================================================ */
void CAN_Dual_Init(void);
void CAN_ProcessMotorFrame(uint32_t id, uint8_t* data, uint8_t len);
void CAN_ProcessComfortFrame(uint32_t id, uint8_t* data, uint8_t len);
void CAN_LogFrame(uint8_t bus_id, uint32_t id, uint8_t* data, uint8_t len);
bool CAN_IsMotorAlive(void);
bool CAN_IsComfortAlive(void);
void CAN_UpdateGearEstimate(void);
void CAN_Simulation_Tick(void);
const char* CAN_GearSelectorLabel(uint8_t raw);   /* 5=D 6=N 7=R 8=P 9=U */

#endif /* __CAN_DUAL_H */
