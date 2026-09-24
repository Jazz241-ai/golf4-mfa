#include "can_dual.h"
#include "ui_graphics.h"
#include <string.h>
extern FDCAN_HandleTypeDef hfdcan1;
extern FDCAN_HandleTypeDef hfdcan2;

MotorCAN_Data_t g_motor_can = {0};
ComfortCAN_Data_t g_comfort_can = {0};
uint32_t g_warn_lamps  = 0;
uint32_t g_fault_flags = 0;

CanMonitorFrame_t g_can_log[CAN_MONITOR_MAX_IDS];
uint8_t g_can_log_count = 0;
uint8_t g_can_mon_bus_select = 0;
static uint16_t g_frame_counter = 0;
static const uint16_t ID_PALETTE[] = {
    0xF800,0x07E0,0x001F,0xFFE0,0xF81F,0x07FF,0xFD20,0x8410,
    0x780F,0x0F0F,0x00FF,0xF00F,0xFF00,0x00F0,0xF0F0,0x0F00
};
static uint16_t GetColorForId(uint32_t id) { return ID_PALETTE[id % 16]; }

/* ============================================================================
   DBC-ЭКСТРАКЦИЯ СИГНАЛОВ
   physical = raw * factor + offset
   endian=1 -> Intel/LE, endian=0 -> Motorola/BE
   sign-bit: при sign=1 физическое значение инвертируется
============================================================================ */
static uint32_t SigRaw(const uint8_t *d, uint8_t len, uint16_t start, uint8_t bits, uint8_t endian) {
    if (endian) {                                   /* Intel / little-endian */
        uint64_t v = 0;
        for (uint8_t i = 0; i < len && i < 8; i++) v |= ((uint64_t)d[i]) << (8 * i);
        uint64_t mask = (bits >= 64) ? ~0ULL : ((1ULL << bits) - 1);
        return (uint32_t)((v >> start) & mask);
    } else {                                        /* Motorola / big-endian */
        uint32_t raw = 0; uint16_t cur = start;
        for (uint8_t k = 0; k < bits; k++) {
            uint8_t byte = cur / 8, bit = cur % 8;
            raw = (raw << 1) | ((byte < len) ? ((d[byte] >> bit) & 1) : 0);
            cur = (bit > 0) ? (cur - 1) : (cur + 15);
        }
        return raw;
    }
}
static float SigPhys(const uint8_t *d, uint8_t len, uint16_t start, uint8_t bits,
                     uint8_t endian, uint8_t signed_, float factor, float offset) {
    uint32_t raw = SigRaw(d, len, start, bits, endian);
    if (signed_ && bits < 32 && (raw & (1u << (bits - 1)))) raw |= ~((1u << bits) - 1);
    return (float)(int32_t)raw * factor + offset;
}
/* сигнал с отдельным sign-bit: при sign=1 инвертируем physical */
static float SigSignPhys(const uint8_t *d, uint8_t len, uint16_t start, uint8_t bits,
                         uint8_t endian, uint8_t signed_, float factor, float offset,
                         uint16_t signstart) {
    float v = SigPhys(d, len, start, bits, endian, signed_, factor, offset);
    if (SigRaw(d, len, signstart, 1, endian) & 1) v = -v;
    return v;
}
static uint8_t SigBool(const uint8_t *d, uint8_t len, uint16_t start, uint8_t endian) {
    return (uint8_t)(SigRaw(d, len, start, 1, endian) & 1);
}
static void LampSet(uint8_t bit, uint8_t on) {
    if (on) g_warn_lamps |= (1u << bit); else g_warn_lamps &= ~(1u << bit);
}
static void FaultSet(uint8_t bit, uint8_t on) {
    if (on) g_fault_flags |= (1u << bit); else g_fault_flags &= ~(1u << bit);
}

/* ============================================================================
   СКОРОСТЬ: приоритет источников Bremse -> Kombi -> Motor
============================================================================ */
static uint32_t t_sp_m = 0, t_sp_k = 0, t_sp_b = 0;
static float s_speed_motor = 0;
static void refresh_speed(void) {
    uint32_t now = HAL_GetTick();
    float v;
    if      (now - t_sp_b < 500) v = g_motor_can.speed_brake;
    else if (now - t_sp_k < 500) v = g_motor_can.speed_kombi;
    else if (now - t_sp_m < 500) v = s_speed_motor;
    else return;
    g_motor_can.speed_kmh = v;
    extern float param_values[PARAM_COUNT];
    param_values[PARAM_SPEED] = v;
}

/* ============================================================================
   СИНХРОНИЗАЦИЯ ЛАМП С ПОДСИСТЕМОЙ СООБЩЕНИЙ (по фронту)
============================================================================ */
static uint32_t s_msg_state = 0;
static void sync_messages(void) {
    uint32_t desired = 0;
    if (g_warn_lamps & ((1u<<LAMP_OIL_PRESS)|(1u<<LAMP_OIL_DYN))) desired |= (1u<<MSG_OIL_PRESS);
    if (g_warn_lamps & (1u<<LAMP_COOL_HOT))    desired |= (1u<<MSG_COOLANT);
    if (g_warn_lamps & (1u<<LAMP_COOL_LEVEL))  desired |= (1u<<MSG_COOL_LEVEL);
    if (g_warn_lamps & (1u<<LAMP_CHARGE))      desired |= (1u<<MSG_CHARGE);
    if (g_warn_lamps & (1u<<LAMP_MIL))         desired |= (1u<<MSG_MIL);
    if (g_warn_lamps & (1u<<LAMP_EPC))         desired |= (1u<<MSG_EPC);
    if (g_warn_lamps & (1u<<LAMP_CAT))         desired |= (1u<<MSG_CAT);
    if (g_warn_lamps & (1u<<LAMP_FUEL))        desired |= (1u<<MSG_FUEL);
    if (g_warn_lamps & (1u<<LAMP_ABS))         desired |= (1u<<MSG_ABS);
    if (g_warn_lamps & (1u<<LAMP_ESP))         desired |= (1u<<MSG_ESP);
    if (g_warn_lamps & (1u<<LAMP_BRAKE))       desired |= (1u<<MSG_BRAKE);
    if (g_warn_lamps & (1u<<LAMP_AIRBAG))      desired |= (1u<<MSG_AIRBAG);
    if (g_warn_lamps & (1u<<LAMP_STEER))       desired |= (1u<<MSG_STEER);
    if (g_warn_lamps & (1u<<LAMP_AWD))         desired |= (1u<<MSG_AWD);
    if (g_warn_lamps & (1u<<LAMP_LEVEL))       desired |= (1u<<MSG_LEVEL);
    if (g_warn_lamps & (1u<<LAMP_EPB))         desired |= (1u<<MSG_EPB);
    if (g_fault_flags & (1u<<FAULT_GEAR_NOTLAUF)) desired |= (1u<<MSG_GEARBOX);
    if (g_warn_lamps & ((1u<<LAMP_TIRE)|(1u<<LAMP_DDS))) desired |= (1u<<MSG_TPMS_WARN);
    if (g_warn_lamps & ((1u<<LAMP_LOWBEAM_L)|(1u<<LAMP_LOWBEAM_R))) desired |= (1u<<MSG_BULB);
    uint32_t changed = desired ^ (uint32_t)s_msg_state;
    for (uint8_t m = 0; m < MSG_COUNT; m++)
        if (changed & (1u<<m)) { if (desired & (1u<<m)) Msg_Raise(m); else Msg_Clear(m); }
    s_msg_state = (uint8_t)desired;
}

/* ============================================================================
   ОБЩИЙ ДИСПЕТЧЕР КАДРОВ (вызывается с обеих шин)
============================================================================ */
static void can_dispatch(uint32_t id, uint8_t* d, uint8_t len) {
    extern float param_values[PARAM_COUNT];
    uint32_t now = HAL_GetTick();
    switch (id) {
    /* ---------------- 0x280 Motor_1 ---------------- */
    case 0x280:
        if (len >= 8) {
            g_motor_can.rpm            = SigPhys(d,len,16,16,1,0,0.25f,0);
            g_motor_can.torque_inner   = SigPhys(d,len, 8, 8,1,0,0.39f,0);
            g_motor_can.torque_driver  = SigPhys(d,len,56, 8,1,0,0.39f,0);
            g_motor_can.torque_loss    = SigPhys(d,len,48, 8,1,0,0.39f,0);
            g_motor_can.throttle_position = SigPhys(d,len,40,8,1,0,0.4f,0);
            g_motor_can.torque_inaccurate = SigBool(d,len,7,1);
            param_values[PARAM_RPM] = g_motor_can.rpm;
        }
        break;
    /* ---------------- 0x288 Motor_2 ---------------- */
    case 0x288:
        if (len >= 4) {
            s_speed_motor = SigPhys(d,len,24,8,1,0,1.28f,0); t_sp_m = now;
            g_motor_can.coolant_temp = SigPhys(d,len,8,8,1,0,0.75f,-48.0f);
            g_motor_can.cruise_status = (uint8_t)SigRaw(d,len,22,2,1);
            g_motor_can.brake_light = SigBool(d,len,16,1);
            g_motor_can.brake_pedal = g_motor_can.brake_light != 0;
            FaultSet(FAULT_COOL_ECU, SigBool(d,len,18,1));
            param_values[PARAM_COOLANT_TEMP] = g_motor_can.coolant_temp;
            refresh_speed();
        }
        break;
    /* ---------------- 0x380 Motor_3 ---------------- */
    case 0x380:
        if (len >= 8) {
            g_motor_can.pedal_raw     = SigPhys(d,len,16,8,1,0,0.4f,0);
            g_motor_can.intake_temp   = SigPhys(d,len, 8,8,1,0,0.75f,-48.0f);
            g_motor_can.desired_rpm   = SigPhys(d,len,48,8,1,0,25.0f,0);
            g_motor_can.throttle_poti = SigPhys(d,len,56,8,1,0,0.4f,0);
            FaultSet(FAULT_OVERTEMP_PROT, SigBool(d,len,1,1));
            param_values[PARAM_INTAKE_TEMP] = g_motor_can.intake_temp;
        }
        break;
    /* ---------------- 0x588 Motor_7 ---------------- */
    case 0x588:
        if (len >= 5) {
            g_motor_can.boost_pressure = SigPhys(d,len,32,8,1,0,0.01f,0);
            g_motor_can.rpm_gradient   = SigPhys(d,len,24,7,1,0,1.0f,0);
            FaultSet(FAULT_ENGINE_MEM, SigBool(d,len,3,1));
        }
        break;
    /* ---------------- 0x320 Kombi_1 ---------------- */
    case 0x320:
        if (len >= 6) {
            g_motor_can.speed_kombi = SigPhys(d,len,25,15,1,0,0.01f,0); t_sp_k = now;
            g_motor_can.speed_displayed = SigPhys(d,len,46,10,1,0,0.32f,0);
            g_motor_can.fuel_level  = SigPhys(d,len,16,7,1,0,1.0f,0);
            g_motor_can.blink_left  = SigBool(d,len,44,1);
            g_motor_can.blink_right = SigBool(d,len,45,1);
            LampSet(LAMP_OIL_PRESS,  SigBool(d,len,2,1));
            LampSet(LAMP_OIL_DYN,    SigBool(d,len,3,1));
            LampSet(LAMP_COOL_LEVEL, SigBool(d,len,4,1));
            LampSet(LAMP_COOL_HOT,   SigBool(d,len,5,1));
            LampSet(LAMP_CHARGE,     SigBool(d,len,10,1));
            LampSet(LAMP_GLOW,       SigBool(d,len,7,1));
            LampSet(LAMP_FUEL,       SigBool(d,len,23,1) || SigBool(d,len,6,1));
            FaultSet(FAULT_TANK,     SigBool(d,len,1,1));
            param_values[PARAM_FUEL_LEFT] = g_motor_can.fuel_level;
            refresh_speed();
        }
        break;
    /* ---------------- 0x420 Kombi_2 ---------------- */
    case 0x420:
        if (len >= 5) {
            g_motor_can.outside_temp     = SigPhys(d,len, 8,8,1,0,0.5f,-50.0f);
            g_motor_can.outside_temp_raw = SigPhys(d,len,16,8,1,0,0.5f,-50.0f);
            g_motor_can.oil_temp         = SigPhys(d,len,24,8,1,0,1.0f,-60.0f);
            g_motor_can.coolant_temp_kombi = SigPhys(d,len,32,8,1,0,0.75f,-48.0f);
            g_motor_can.outside_time = now;
            FaultSet(FAULT_OIL_TEMP,  SigBool(d,len,1,1));
            FaultSet(FAULT_COOL_TEMP, SigBool(d,len,2,1));
            FaultSet(FAULT_CLUSTER_MEM, SigBool(d,len,7,1));
            param_values[PARAM_OUTSIDE_TEMP] = g_motor_can.outside_temp;
            param_values[PARAM_OIL_TEMP]     = g_motor_can.oil_temp;
        }
        break;
    /* ---------------- 0x520 Kombi_3 ---------------- */
    case 0x520:
        if (len >= 8) {
            g_motor_can.odometer_km = (float)SigRaw(d,len,40,20,1);
            g_motor_can.standzeit_sec = SigPhys(d,len,24,15,1,0,4.0f,0);
            g_motor_can.key_info = (uint8_t)SigRaw(d,len,16,4,1);
            static float odo_start = 0;
            if (g_motor_can.odometer_km > 0) {
                if (odo_start == 0) odo_start = g_motor_can.odometer_km;
                param_values[PARAM_ODOMETER] = g_motor_can.odometer_km;
                param_values[PARAM_DISTANCE] = g_motor_can.odometer_km - odo_start;
            }
        }
        break;
    /* ---------------- 0x1A0 Bremse_1 ---------------- */
    case 0x1A0:
        if (len >= 7) {
            g_motor_can.speed_brake = SigPhys(d,len,17,15,1,0,0.01f,0); t_sp_b = now;
            g_motor_can.msr_torque  = SigPhys(d,len,48,8,1,0,0.39f,0);
            g_motor_can.abs_diag    = SigBool(d,len,15,1);
            g_motor_can.esp_active  = SigBool(d,len,4,1);
            g_motor_can.abs_active  = SigBool(d,len,2,1);
            g_motor_can.asr_req     = SigBool(d,len,0,1);
            LampSet(LAMP_ABS,   SigBool(d,len,8,1));
            LampSet(LAMP_ESP,   SigBool(d,len,9,1));
            LampSet(LAMP_BRAKE, SigBool(d,len,10,1));
            FaultSet(FAULT_ABS_DIAG, g_motor_can.abs_diag);
            refresh_speed();
        }
        break;
    /* ---------------- 0x4A0 Bremse_3: скорости колёс ---------------- */
    case 0x4A0:
        if (len >= 8) {
            g_motor_can.wheel_speed[0] = SigPhys(d,len, 1,15,1,0,0.01f,0);
            g_motor_can.wheel_speed[1] = SigPhys(d,len,17,15,1,0,0.01f,0);
            g_motor_can.wheel_speed[2] = SigPhys(d,len,33,15,1,0,0.01f,0);
            g_motor_can.wheel_speed[3] = SigPhys(d,len,49,15,1,0,0.01f,0);
            g_motor_can.wheel_time = now;
        }
        break;
    /* ---------------- 0x4A8 Bremse_5 ---------------- */
    case 0x4A8:
        if (len >= 7) {
            g_motor_can.brake_pressure = SigPhys(d,len,16,12,1,0,0.1f,0);
            g_motor_can.yaw_rate = SigSignPhys(d,len,0,14,1,0,0.01f,0,15);
            g_motor_can.brake_temp_front = SigPhys(d,len,48,3,1,0,125.0f,125.0f);
            FaultSet(FAULT_BRAKE_PRESS, SigBool(d,len,29,1));
            FaultSet(FAULT_YAW_INV,     SigBool(d,len,14,1));
        }
        break;
    /* ---------------- 0x1A8 / 0x2A8 бустер ---------------- */
    case 0x1A8:
        if (len >= 2) FaultSet(FAULT_BRAKE_PRESS, SigBool(d,len,11,1) || (g_fault_flags & (1u<<FAULT_BRAKE_PRESS)));
        break;
    case 0x2A8:
        if (len >= 3) {
            FaultSet(FAULT_BOOSTER_SW,  SigBool(d,len,22,1));
            FaultSet(FAULT_BOOSTER_MEM, SigBool(d,len,23,1));
        }
        break;
    /* ---------------- 0xC2 Lenkwinkel_1 ---------------- */
    case 0xC2:
        if (len >= 4) {
            g_motor_can.steering_angle = SigSignPhys(d,len,0,15,1,0,0.04375f,0,15);
            g_motor_can.steering_speed = SigSignPhys(d,len,16,15,1,0,0.04375f,0,31);
            param_values[PARAM_STEERING_ANGLE] = g_motor_can.steering_angle;
        }
        break;
    /* ---------------- 0x440 Getriebe_1 (enum селектора) ---------------- */
    case 0x440:
        if (len >= 3) {
            g_motor_can.gear_selector = (uint8_t)SigRaw(d,len,12,4,1);
            g_motor_can.gear_target   = (uint8_t)SigRaw(d,len, 8,4,1);
            g_motor_can.converter     = (uint8_t)SigRaw(d,len, 3,2,1);
            g_motor_can.at_frame_time = now;
            FaultSet(FAULT_GEAR_NOTLAUF, SigRaw(d,len,40,4,1) != 0);
            FaultSet(FAULT_GEAR_MEM, SigBool(d,len,55,1));
            /* P=8 R=7 N=6 D=5 U=9 */
            switch (g_motor_can.gear_selector) {
            case 8: g_motor_can.gear_position = 1; break;
            case 7: g_motor_can.gear_position = 2; break;
            case 6: g_motor_can.gear_position = 3; break;
            case 5: case 9:
                if (g_motor_can.gear_target >= 1 && g_motor_can.gear_target <= 6)
                    g_motor_can.gear_position = 5 + g_motor_can.gear_target; /* 6..11 => "1".."6" */
                else g_motor_can.gear_position = 4;
                break;
            default: break;
            }
        }
        break;
    /* ---------------- 0x540 Getriebe_2 ---------------- */
    case 0x540:
        if (len >= 7) {
            LampSet(LAMP_UPSHIFT,   SigBool(d,len,48,1));
            LampSet(LAMP_SHIFTLOCK, SigBool(d,len,52,1));
            LampSet(LAMP_CLUTCH,    SigBool(d,len,55,1));
        }
        break;
    /* ---------------- 0x572 ZAS_1 ---------------- */
    case 0x572:
        if (len >= 1) {
            g_motor_can.key_in      = SigBool(d,len,0,1);
            g_motor_can.ignition_on = SigBool(d,len,1,1);
            g_motor_can.kl_x        = SigBool(d,len,2,1);
            g_motor_can.starter_active = SigBool(d,len,3,1);
            g_motor_can.kl_p        = SigBool(d,len,4,1);
            g_motor_can.kl_15sv     = SigBool(d,len,6,1);
            FaultSet(FAULT_ZAS_MEM, SigBool(d,len,15,1));
        }
        break;
    /* ---------------- 0x5E0 Klima_1 ---------------- */
    case 0x5E0:
        if (len >= 7) {
            g_motor_can.ac_pressure = SigPhys(d,len,16,8,1,0,0.2f,0);
            g_motor_can.comp_load   = SigPhys(d,len,24,8,1,0,0.25f,0);
            g_motor_can.fan_load    = SigPhys(d,len,32,8,1,0,0.4f,0);
            g_motor_can.ac_on       = SigBool(d,len,49,1);
            FaultSet(FAULT_CLIMATE_MEM, SigBool(d,len,55,1));
            if (now - g_motor_can.outside_time > 1500) {   /* резерв наружной температуры */
                g_motor_can.outside_temp = SigPhys(d,len,8,8,1,0,0.5f,-50.0f);
                param_values[PARAM_OUTSIDE_TEMP] = g_motor_can.outside_temp;
            }
        }
        break;
    /* ---------------- 0x50 Airbag_1 ---------------- */
    case 0x50:
        if (len >= 3) {
            g_motor_can.crash_flags = (uint8_t)SigRaw(d,len,0,5,1);
            g_motor_can.crash_intensity = (uint8_t)SigRaw(d,len,5,3,1);
            g_motor_can.belt_sw_drv = SigBool(d,len,12,1);
            g_motor_can.belt_sw_pas = SigBool(d,len,14,1);
            LampSet(LAMP_AIRBAG,     SigBool(d,len,8,1));
            LampSet(LAMP_AIRBAG_OFF, SigBool(d,len,9,1));
            LampSet(LAMP_BELT_DRV,   SigBool(d,len,13,1));
            LampSet(LAMP_BELT_PAS,   SigBool(d,len,15,1));
            FaultSet(FAULT_AIRBAG_SYS, SigBool(d,len,11,1));
            FaultSet(FAULT_AIRBAG_MEM, SigBool(d,len,19,1));
            g_belt_unfastened = SigBool(d,len,13,1) || SigBool(d,len,15,1);
        }
        break;
    /* ---------------- 0x3D0 Lenkhilfe_1 ---------------- */
    case 0x3D0:
        if (len >= 2) {
            LampSet(LAMP_STEER, SigBool(d,len,8,1));
            FaultSet(FAULT_STEER_MEM, SigBool(d,len,15,1));
        }
        break;
    /* ---------------- 0x343 RDK_Status ---------------- */
    case 0x343:
        if (len >= 3) {
            g_motor_can.tire_warn_mask = (uint8_t)SigRaw(d,len,0,5,1);
            LampSet(LAMP_TIRE, SigBool(d,len,5,1) || SigBool(d,len,6,1) || SigBool(d,len,16,1));
            LampSet(LAMP_DDS,  SigBool(d,len,16,1));
            FaultSet(FAULT_TIRE_SYS,   SigBool(d,len,7,1));
            FaultSet(FAULT_TIRE_RADIO, SigBool(d,len,12,1));
        }
        break;
    /* ---------------- 0x5B8 WFS_1 ---------------- */
    case 0x5B8:
        if (len >= 2) {
            LampSet(LAMP_IMMO, SigBool(d,len,0,1));
            g_motor_can.immo_text = (uint8_t)SigRaw(d,len,8,8,1);
        }
        break;
    /* ---------------- 0x548 / 0x5A0 тормоза/стояночный ---------------- */
    case 0x548:
        if (len >= 1) {
            LampSet(LAMP_HANDBRAKE, SigBool(d,len,1,1));
            g_motor_can.handbrake = SigBool(d,len,1,1);
        }
        break;
    case 0x5A0:
        if (len >= 7) {
            LampSet(LAMP_DDS, SigBool(d,len,53,1));
            FaultSet(FAULT_BRAKE_MEM, SigBool(d,len,52,1));
        }
        break;
    /* ---------------- 0x2C0 Allrad_1 ---------------- */
    case 0x2C0:
        if (len >= 4) {
            LampSet(LAMP_AWD, SigBool(d,len,5,1));
            FaultSet(FAULT_AWD, SigBool(d,len,0,1) || SigBool(d,len,1,1) ||
                                SigBool(d,len,2,1) || SigBool(d,len,4,1) || SigBool(d,len,31,1));
        }
        break;
    /* ---------------- 0x590 / 0x598 подвеска ---------------- */
    case 0x590:
        if (len >= 5) {
            LampSet(LAMP_LEVEL, SigBool(d,len,13,1));
            FaultSet(FAULT_LEVEL_MEM, SigBool(d,len,39,1));
        }
        break;
    case 0x598:
        if (len >= 1) FaultSet(FAULT_DAMPER, SigBool(d,len,7,1));
        break;
    /* ---------------- 0x5C0 EPB_1 ---------------- */
    case 0x5C0:
        if (len >= 7) {
            LampSet(LAMP_EPB, SigBool(d,len,48,1));
            FaultSet(FAULT_EPB_MEM, SigBool(d,len,32,1));
        }
        break;
    /* ---------------- 0x497 Parkhilfe ---------------- */
    case 0x497:
        if (len >= 8)
            FaultSet(FAULT_PARK, SigBool(d,len,56,1) || SigBool(d,len,57,1) || SigBool(d,len,63,1));
        break;
    /* ---------------- 0x390 / 0x392 свет ---------------- */
    case 0x390:
        if (len >= 8) {
            LampSet(LAMP_LOWBEAM_L, SigBool(d,len,52,1));
            LampSet(LAMP_LOWBEAM_R, SigBool(d,len,53,1));
            LampSet(LAMP_TRAILER,   SigBool(d,len,61,1));
            FaultSet(FAULT_LIGHT_L, SigBool(d,len,52,1));
            FaultSet(FAULT_LIGHT_R, SigBool(d,len,53,1));
        }
        break;
    case 0x392:
        if (len >= 2)
            FaultSet(FAULT_LIGHT_SENSOR, SigBool(d,len,14,1) || SigBool(d,len,15,1));
        break;
    /* ---------------- 0x538 Wischer_1 ---------------- */
    case 0x538:
        if (len >= 2) {
            FaultSet(FAULT_WIPER_F, SigBool(d,len,7,1));
            FaultSet(FAULT_WIPER_R, SigBool(d,len,15,1));
            FaultSet(FAULT_WIPER_MEM, SigBool(d,len,11,1));
        }
        break;
    /* ---------------- 0x470 BSG_Kombi (comfort) ---------------- */
    case 0x470:
        if (len >= 5) {
            LampSet(LAMP_CHARGE, SigBool(d,len,7,1));
            LampSet(LAMP_STEER,  SigBool(d,len,32,1));
            g_comfort_can.undervolt = SigBool(d,len,15,1);
            FaultSet(FAULT_UNDERVOLT, g_comfort_can.undervolt);
        }
        break;
    /* ---------------- 0x570 BSG_Last (comfort) ---------------- */
    case 0x570:
        if (len >= 3) {
            g_comfort_can.battery_voltage = SigPhys(d,len,16,8,1,0,0.05f,5.0f);
            g_comfort_can.battery_state_board   = (uint8_t)SigRaw(d,len,9,2,1);
            g_comfort_can.battery_state_starter = (uint8_t)SigRaw(d,len,11,2,1);
            g_motor_can.voltage = g_comfort_can.battery_voltage;
            param_values[PARAM_VOLTAGE] = g_motor_can.voltage;
        }
        break;
    /* ---------------- 0x578 BatMan_1 (comfort) ---------------- */
    case 0x578:
        if (len >= 1) {
            g_comfort_can.start_mode = SigBool(d,len,0,1);
            g_comfort_can.starter_charge = (uint8_t)SigRaw(d,len,1,2,1);
            FaultSet(FAULT_BATMAN_MEM, SigBool(d,len,7,1));
        }
        break;
    /* ---------------- 0x480 расход (аккумулятор топлива) ---------------- */
    case 0x480:
        if (len >= 6) {
            /* лампы двигателя из того же кадра */
            LampSet(LAMP_CHARGE, SigBool(d,len,8,1));
            LampSet(LAMP_GLOW,   SigBool(d,len,9,1));
            LampSet(LAMP_EPC,    SigBool(d,len,10,1));
            LampSet(LAMP_MIL,    SigBool(d,len,11,1));
            LampSet(LAMP_CAT,    SigBool(d,len,12,1));
            g_motor_can.mil_active = SigBool(d,len,11,1);
            /* счётчик расхода */
            #define FUEL_CONS_COEF 0.0035f
            uint64_t acc = ((uint64_t)d[5] << 32) | ((uint64_t)d[4] << 16)
                         | ((uint64_t)d[3] << 8)  | (uint64_t)d[2];
            static uint64_t prev_acc = 0; static uint8_t acc_ready = 0;
            static uint32_t last_calc = 0;
            if (!acc_ready || acc < prev_acc) { prev_acc = acc; acc_ready = 1; last_calc = now; break; }
            uint32_t dt = now - last_calc;
            if (dt >= 1000) {
                float delta = (float)(acc - prev_acc);
                prev_acc = acc; last_calc = now;
                float lh = delta * FUEL_CONS_COEF * (1000.0f / (float)dt);
                float l100 = (g_motor_can.speed_kmh > 2.0f) ? (lh * 100.0f / g_motor_can.speed_kmh) : 0.0f;
                g_motor_can.fuel_consumption_inst = l100;
                param_values[PARAM_CONSUMPTION_INSTANT] = (g_motor_can.speed_kmh > 2.0f) ? l100 : lh;
                static float avg_smooth = 0;
                if (l100 > 0.1f) avg_smooth = (avg_smooth < 0.1f) ? l100 : (avg_smooth * 0.9f + l100 * 0.1f);
                if (avg_smooth > 0.1f && g_motor_can.fuel_level > 0) {
                    g_motor_can.fuel_range = g_motor_can.fuel_level / avg_smooth * 100.0f;
                    param_values[PARAM_RANGE] = g_motor_can.fuel_range;
                }
            }
        }
        break;
    /* ---------------- 0x120 симуляция ---------------- */
    case 0x120:
        if (len >= 4) {
            s_speed_motor = SigPhys(d,len,0,16,1,0,0.01f,0); t_sp_m = now;
            g_motor_can.rpm = SigPhys(d,len,16,16,1,0,0.25f,0);
            param_values[PARAM_RPM] = g_motor_can.rpm;
            refresh_speed();
        }
        break;
    default: break;
    }
    /* производные состояния + синхронизация сообщений */
    g_motor_can.engine_running = g_motor_can.ignition_on && (g_motor_can.rpm > 50.0f);
    g_motor_can.mil_active = (g_warn_lamps & (1u << LAMP_MIL)) != 0;
    sync_messages();
}

/* ============================================================================
   ФИЛЬТРЫ / INIT / ЛОГ
============================================================================ */
static void CAN_Motor_SetFilters(void) {
    FDCAN_FilterTypeDef f;
    f.IdType = FDCAN_STANDARD_ID; f.FilterIndex = 0; f.FilterType = FDCAN_FILTER_RANGE;
    f.FilterConfig = FDCAN_FILTER_TO_RXFIFO0; f.FilterID1 = 0x000; f.FilterID2 = 0x7FF;
    HAL_FDCAN_ConfigFilter(&hfdcan1, &f);
    HAL_FDCAN_Start(&hfdcan1);
    HAL_FDCAN_ActivateNotification(&hfdcan1, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0);
}
static void CAN_Comfort_SetFilters(void) {
    FDCAN_FilterTypeDef f;
    f.IdType = FDCAN_STANDARD_ID; f.FilterIndex = 0; f.FilterType = FDCAN_FILTER_RANGE;
    f.FilterConfig = FDCAN_FILTER_TO_RXFIFO0; f.FilterID1 = 0x000; f.FilterID2 = 0x7FF;
    HAL_FDCAN_ConfigFilter(&hfdcan2, &f);
    HAL_FDCAN_Start(&hfdcan2);
    HAL_FDCAN_ActivateNotification(&hfdcan2, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0);
}
void CAN_Dual_Init(void) {
    memset(&g_motor_can, 0, sizeof(g_motor_can));
    memset(&g_comfort_can, 0, sizeof(g_comfort_can));
    g_warn_lamps = 0; g_fault_flags = 0;
    CAN_Motor_SetFilters();
    CAN_Comfort_SetFilters();
}
void CAN_LogFrame(uint8_t bus_id, uint32_t id, uint8_t* data, uint8_t len) {
    uint8_t l = (len > 8) ? 8 : len;
    for (uint8_t i = 0; i < g_can_log_count; i++) {
        if (g_can_log[i].id == id && g_can_log[i].bus == bus_id) {
            g_can_log[i].timestamp = g_frame_counter++;
            g_can_log[i].len = l;
            memcpy(g_can_log[i].data, data, l);
            return;
        }
    }
    if (g_can_log_count >= CAN_MONITOR_MAX_IDS) return;
    uint8_t idx = g_can_log_count;
    for (uint8_t i = 0; i < g_can_log_count; i++) { if (id < g_can_log[i].id) { idx = i; break; } }
    for (int8_t i = g_can_log_count; i > idx; i--) g_can_log[i] = g_can_log[i-1];
    g_can_log[idx].id = id; g_can_log[idx].bus = bus_id; g_can_log[idx].len = l;
    memcpy(g_can_log[idx].data, data, l);
    g_can_log[idx].timestamp = g_frame_counter++;
    g_can_log[idx].color = GetColorForId(id);
    g_can_log_count++;
}
static uint8_t DLC_ToBytes(uint32_t dataLength) {
    uint8_t dlc = (dataLength >> 16) & 0x0F;
    return (dlc > 8) ? 8 : dlc;
}
void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs) {
    FDCAN_RxHeaderTypeDef RxHeader = {0};
    uint8_t RxData[8] = {0};
    if (HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &RxHeader, RxData) == HAL_OK) {
        uint8_t len = DLC_ToBytes(RxHeader.DataLength);
        if (hfdcan->Instance == FDCAN1) CAN_ProcessMotorFrame(RxHeader.Identifier, RxData, len);
        else                            CAN_ProcessComfortFrame(RxHeader.Identifier, RxData, len);
    }
}
void CAN_ProcessMotorFrame(uint32_t id, uint8_t* data, uint8_t len) {
    g_motor_can.last_frame_time = HAL_GetTick();
    can_dispatch(id, data, len);
    CAN_LogFrame(0, id, data, len);
}
void CAN_ProcessComfortFrame(uint32_t id, uint8_t* data, uint8_t len) {
    g_comfort_can.last_frame_time = HAL_GetTick();
    can_dispatch(id, data, len);
    CAN_LogFrame(1, id, data, len);
}

/* ============================================================================
   ОЦЕНКА ПЕРЕДАЧИ (МКПП) — пропускаем, если АКПП сама шлёт селектор/передачу
============================================================================ */
static const float GEAR_RATIOS[5] = {3.300f, 1.944f, 1.308f, 1.029f, 0.837f};
#define FINAL_DRIVE   3.944f
#define WHEEL_CIRC_MM 1985.0f
void CAN_UpdateGearEstimate(void) {
    static uint32_t last = 0;
    static uint8_t cand = 0, cand_cnt = 0;
    uint32_t now = HAL_GetTick();
    if (now - last < 200) return;
    last = now;
    if (now - g_motor_can.at_frame_time < 1000) return;   /* АКПП даёт передачу сама */
    extern float param_values[PARAM_COUNT];
    float spd = param_values[PARAM_SPEED];
    float rpm = param_values[PARAM_RPM];
    if (spd < 8.0f || rpm < 300.0f) {
        g_motor_can.gear_position = 3;
        cand = 0; cand_cnt = 0;
        return;
    }
    float m = rpm / spd;
    const float wheel_k = 1000000.0f / (WHEEL_CIRC_MM * 60.0f);
    uint8_t best = 0; float best_err = 1e9f;
    for (uint8_t i = 0; i < 5; i++) {
        float exp_r = wheel_k * GEAR_RATIOS[i] * FINAL_DRIVE;
        float err = fabsf(m - exp_r) / exp_r;
        if (err < best_err) { best_err = err; best = i + 1; }
    }
    if (best_err < 0.12f) {
        if (best == cand) { if (++cand_cnt >= 3) g_motor_can.gear_position = 5 + best; }
        else { cand = best; cand_cnt = 1; }
    } else { cand = 0; cand_cnt = 0; g_motor_can.gear_position = 3; }
}
bool CAN_IsMotorAlive(void)   { return (HAL_GetTick() - g_motor_can.last_frame_time)   < CAN_TIMEOUT_MS; }
bool CAN_IsComfortAlive(void) { return (HAL_GetTick() - g_comfort_can.last_frame_time) < CAN_TIMEOUT_MS; }
void CAN_Simulation_Tick(void) { }
const char* CAN_GearSelectorLabel(uint8_t raw) {
    switch (raw) { case 5: return "D"; case 6: return "N"; case 7: return "R";
                   case 8: return "P"; case 9: return "U"; default: return "-"; }
}
