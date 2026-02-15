#include "hardware.h"

// 3.45V: above LOW_THRESHOLD (3.4) so IDLE won't trigger; below RECOVERY_THRESHOLD (3.5) so LOW_BATTERY won't recover
static float mock_battery_voltage = 3.45f;

void hardware_init()      {}
void hardware_heartbeat() {}

float battery_get_voltage() { return mock_battery_voltage; }
bool  battery_is_low()      { return mock_battery_voltage < 3.4f; }
bool  battery_is_critical() { return mock_battery_voltage < 3.3f; }

void battery_set_mock_voltage(float voltage) { mock_battery_voltage = voltage; }
