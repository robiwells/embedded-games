#ifndef SESSION_MANAGER_H
#define SESSION_MANAGER_H

#include "config.h"

// Begin a new session. Records start time and initialises the session log entry.
void session_begin(MoodCategory mood, const Activity* activity, TimeOfDay time_of_day);

// End the current session. Calculates duration and calls logger_log_session().
// completed=true if the activity audio played to completion, false if interrupted.
void session_end(bool completed);

// Returns true if a session has been started and not yet ended.
bool session_is_active();

#endif // SESSION_MANAGER_H
