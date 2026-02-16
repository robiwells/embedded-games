#include "data_logger.h"
#include <string.h>

static SessionLog s_last_session;
static bool s_called = false;

bool logger_init()                               { return true; }

bool logger_log_session(const SessionLog* s) {
    if (s) s_last_session = *s;
    s_called = true;
    return true;
}

void logger_test() {}

const SessionLog* mock_get_last_session() { return &s_last_session; }
bool mock_logger_was_called()             { return s_called; }
void mock_logger_reset()                  { s_called = false; memset(&s_last_session, 0, sizeof(s_last_session)); }
