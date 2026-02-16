#ifndef HARDWARE_H
#define HARDWARE_H

// Hardware Abstraction Layer Interface

typedef struct {
    bool nfc_ok;
    bool sd_ok;
    bool audio_ok;
} HardwareInitResult;

HardwareInitResult hardware_init();
void hardware_heartbeat();
void hardware_enter_deep_sleep();
void battery_manager_update();

float battery_get_voltage();
bool  battery_is_low();
bool  battery_is_critical();
#ifdef WOKWI_SIMULATION
void  battery_set_mock_voltage(float voltage);
#endif

#endif
