#ifndef HARDWARE_H
#define HARDWARE_H

// Hardware Abstraction Layer Interface

void hardware_init();
void hardware_heartbeat();

float battery_get_voltage();
bool  battery_is_low();
bool  battery_is_critical();
#ifdef WOKWI_SIMULATION
void  battery_set_mock_voltage(float voltage);
#endif

#endif
