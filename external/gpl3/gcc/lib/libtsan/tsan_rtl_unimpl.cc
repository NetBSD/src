#include <cstdlib>

extern "C" void __tsan_report_race_thunk(void) { abort(); }
extern "C" void __tsan_trace_switch_thunk(void) { abort(); }
