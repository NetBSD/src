/*
 * machine-dependent clock routines; intersil7170
 *
 * $Header: /cvsroot/src/sys/arch/sun3/sun3/clock.c,v 1.2 1993/06/25 23:07:14 glass Exp $
 */

#include "param.h"
#include "kernel.h"
#include "intersil7170.h"

#include "psl.h"
#include "cpu.h"
#include "obio.h"

#define intersil_clock (struct intersil7170 *) CLOCK_ADDR)
#define intersil_command(run, interrupt) \
    (run | interrupt | INTERSIL_CMD_FREQ_32K | INTERSIL_CMD_24HR_MODE | \
     INTERSIL_CMD_NORMAL_MODE)

#define SECS_HOUR               (60*60)
#define SECS_DAY                (SECS_HOUR*24)
#define SECS_PER_YEAR           (SECS_DAY*365)
#define SECS_PER_LEAP           (SECS_DAY*366)
#define SECS_PER_MONTH(month, year) \
    ((month == 2) && INTERSIL_LEAP_YEAR(year) \
    ? 29*SECS_DAY : month_days[month-1]*SECS_DAY)
    
static int month_days[12] = {
	31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

/*
 * Machine-dependent clock routines.
 *
 * Startrtclock restarts the real-time clock, which provides
 * hardclock interrupts to kern_clock.c.
 *
 * Inittodr initializes the time of day hardware which provides
 * date functions.
 *
 * Resettodr restores the time of day hardware after a time change.
 *
 * A note on the real-time clock:
 * We actually load the clock with CLK_INTERVAL-1 instead of CLK_INTERVAL.
 * This is because the counter decrements to zero after N+1 enabled clock
 * periods where N is the value loaded into the counter.
 */

/*
 * Start the real-time clock.
 */
startrtclock()
{
    intersil_clock->command_reg = intersil_command(INTERSIL_CMD_RUN,
						   INTERSIL_IDISABLE);
    intersil_clock->interrupt_reg = INTERSIL_INTER_CSECONDS;
}

enablertclock()
{
	/* make sure irq5/7 stuff is resolved :) */

    intersil_clock->command_reg = intersil_command(INTERSIL_CMD_RUN,
						   INTERSIL_IENABLE);
}

void intersil_counter_state(map)
     struct intersil_map *map;
{
    map->csecs = intersil_clock->counters.csecs;
    map->hours = intersil_clock->counters.hours;
    map->minutes = intersil_clock->counters.minutes;
    map->seconds = intersil_clock->counters.seconds;
    map->month = intersil_clock->counters.month;
    map->date = intersil_clock->counters.date;
    map->year = intersil_clock->counters.year;
    map->day = intersil_clock->counters.day;
}


struct timeval intersil_to_timeval()
{
    struct intersil_map now_state;
    struct timeval now;
    int i;

    intersil_counter_state(&now_state);

    now.tv_sec = 0;
    now.tv_usec = 0;

#define range_check_high(field, high)
    now_state->field > high
#define range_check(field, low, high)
	now_state->field < low || now_state->field > high

    if (range_check_high(csecs, 99) ||
	range_check_high(hours, 23) ||
	range_check_high(minutes, 59) ||
	range_check_high(seconds, 59) ||
	range_check(month, 1, 12) ||
	range_check(date, 1, 32) ||
	range_check_high(year, 99) ||
	range_check_high(day, 6)) return now;

    if ((INTERSIL_UNIX_BASE - INTERSIL_YEAR_BASE) > now_state->year)
	return now;

    for (i = INTERSIL_UNIX_BASE-INTERSIL_YEAR_BASE;
	 i < now_state->year, i++)
	if (INTERSIL_LEAP_YEAR(INTERSIL_YEAR_BASE+now_state->year))
	    now.tv_sec += SECS_PER_LEAP;
	else now.tv_sec += SECS_PER_YEAR;
    
    for (i = 1; i < now_state->month; i++) 
	now.tv_sec += SECS_PER_MONTH(i, INTERSIL_YEAR_BASE+now_state->year);
    now.tv_sec += SECS_DAY*(now_state->date-1);
    now.tv_sec += SECS_PER_HOUR * now_state->hours;
    now.tv_sec += 60 * now_state->minutes;
    now.tv_sec += 60 * now_state->seconds;
    return now;
}
/*
 * Initialize the time of day register, based on the time base which is, e.g.
 * from a filesystem.
 */
inittodr(base)
	time_t base;
{
    int s;
    struct timeval clock_time;
    long diff_time;

    clock_time = intersil_to_timeval();

    if (!clock_time.tv_sec && (base <= 0) goto set_time;
	
    if (clock_time.tv_sec < base) {
	printf("WARNING: real-time clock reports a time earlier than last\n");
	printf("         write to root filesystem.  Trusting filesystem..."\n);
	time.tv_sec = base;
	resettodr();
	return;
    }
    diff_time = clock_time.tv_sec - base;
    if (diff_time < 0) diff_time = - diff_time;
    if (diff_time >= (SECS_DAY*2))
	printf("WARNING: clock %s %d days\n",
	       (clock_time.tv_sec <base ? "lost" : "gained"),
	       diff_time % SECS_DAY);

set_time:
    time = clock_time;
}

resettodr()
{
    
}




