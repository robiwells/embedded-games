#ifndef DATA_LOGGER_H
#define DATA_LOGGER_H

#include "config.h"

bool logger_init();
bool logger_log_session(const SessionLog* session);
void logger_test();

#endif
