/*
 * Copyright (c) 1993 Adam Glass
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Adam Glass.
 * 4. The name of the Author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Adam Glass ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Header: /cvsroot/src/sys/arch/sun3/sun3/clock.c,v 1.10 1993/10/12 05:25:13 glass Exp $
 */
/*
 * machine-dependent clock routines; intersil7170
 *               by Adam Glass
 *
 */

#include "systm.h"
#include "param.h"
#include "kernel.h"
#include "device.h"

#include <machine/autoconf.h>
#include <machine/psl.h>
#include <machine/cpu.h>
#include <machine/mon.h>
#include <machine/obio.h>

#include "intersil7170.h"
#include "interreg.h"

#define intersil_clock ((volatile struct intersil7170 *) CLOCK_VA)
#define intersil_command(run, interrupt) \
    (run | interrupt | INTERSIL_CMD_FREQ_32K | INTERSIL_CMD_24HR_MODE | \
     INTERSIL_CMD_NORMAL_MODE)

#define intersil_disable() \
    intersil_clock->command_reg = \
    intersil_command(INTERSIL_CMD_RUN, INTERSIL_CMD_IDISABLE)

#define intersil_enable() \
    intersil_clock->command_reg = \
    intersil_command(INTERSIL_CMD_RUN, INTERSIL_CMD_IENABLE)
#define intersil_clear() intersil_clock->interrupt_reg


#define SECS_HOUR               (60*60)
#define SECS_DAY                (SECS_HOUR*24)
#define SECS_PER_YEAR           (SECS_DAY*365)
#define SECS_PER_LEAP           (SECS_DAY*366)
#define SECS_PER_MONTH(month, year) \
    ((month == 2) && INTERSIL_LEAP_YEAR(year) \
     ? 29*SECS_DAY : month_days[month-1]*SECS_DAY)

#define SECS_YEAR(year) \
       (INTERSIL_LEAP_YEAR(year) ? SECS_PER_LEAP : SECS_PER_YEAR)


struct clock_softc {
    struct device clock_dev;
    caddr_t clock_va;
    int clock_level;
};

struct clock_softc *intersil_softc = NULL;

static int month_days[12] = {
	31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

void clockattach __P((struct device *, struct device *, void *));
int clockmatch __P((struct device *, struct cfdata *, void *args));

struct cfdriver clockcd = 
{ NULL, "clock", always_match, clockattach, DV_DULL,
      sizeof(struct clock_softc), 0};

int clockmatch(parent, cf, args)
     struct device *parent;
     struct cfdata *cf;
     void *args;
{
    caddr_t intersil_addr;
    struct obio_cf_loc *obio_loc = (struct obio_cf_loc *) CFDATA_LOC(cf);

    intersil_addr =
	OBIO_DEFAULT_PARAM(caddr_t, obio_loc->obio_addr, OBIO_CLOCK);
    return /* probe */ 1;
}
#ifdef mine
void set_clock_level(level_code, enable_clock)
     unsigned int level_code;
     int enable_clock;
{
    unsigned int val, stupid_thing;
    vm_offset_t pte;

    val = get_interrupt_reg();	/* get interrupt register value */
    val &= ~(IREG_ALL_ENAB);
    set_interrupt_reg(val);	/* disable all "interrupts" */
    intersil_disable();		/* turn off clock interrupt source */
    stupid_thing = intersil_clear(); /* torch peinding interrupts on clock*/
    stupid_thing++;		/* defeat compiler? */
    val &= ~(IREG_CLOCK_ENAB_7 | IREG_CLOCK_ENAB_5);
    set_interrupt_reg(val);	/* clear any pending interrupts */
    val |= level_code;
    set_interrupt_reg(val);	/* enable requested interrupt level if any */
    val |= IREG_ALL_ENAB;
    set_interrupt_reg(val);	/* enable all interrupts */
    if (enable_clock)
	intersil_enable();
}
#endif

/*
 * Set and/or clear the desired clock bits in the interrupt
 * register.  We have to be extremely careful that we do it
 * in such a manner that we don't get ourselves lost.
 */
set_clk_mode(on, off, enable)
	u_char on, off;
        int enable;
{
	register u_char interreg, dummy;
	extern char *interrupt_reg;
	/*
	 * make sure that we are only playing w/ 
	 * clock interrupt register bits
	 */
	on &= (IREG_CLOCK_ENAB_7 | IREG_CLOCK_ENAB_5);
	off &= (IREG_CLOCK_ENAB_7 | IREG_CLOCK_ENAB_5);

	/*
	 * Get a copy of current interrupt register,
	 * turning off any undesired bits (aka `off')
	 */
	interreg = *interrupt_reg & ~(off | IREG_ALL_ENAB);
	*interrupt_reg &= ~IREG_ALL_ENAB;

	/*
	 * Next we turns off the CLK5 and CLK7 bits to clear
	 * the flip-flops, then we disable clock interrupts.
	 * Now we can read the clock's interrupt register
	 * to clear any pending signals there.
	 */
	*interrupt_reg &= ~(IREG_CLOCK_ENAB_7 | IREG_CLOCK_ENAB_5);
	intersil_clock->command_reg = intersil_command(INTERSIL_CMD_RUN,
						       INTERSIL_CMD_IDISABLE);
	dummy = intersil_clock->interrupt_reg;	/* clear clock */
#ifdef lint
	dummy = dummy;
#endif

	/*
	 * Now we set all the desired bits
	 * in the interrupt register, then
	 * we turn the clock back on and
	 * finally we can enable all interrupts.
	 */
	*interrupt_reg |= (interreg | on);		/* enable flip-flops */
	if (enable)
	    intersil_clock->command_reg =
		intersil_command(INTERSIL_CMD_RUN,
				 INTERSIL_CMD_IENABLE);
	*interrupt_reg |= IREG_ALL_ENAB;		/* enable interrupts */
}

void clockattach(parent, self, args)
     struct device *parent;
     struct device *self;
     void *args;
{
    struct clock_softc *clock = (struct clock_softc *) self;
    struct obio_cf_loc *obio_loc = OBIO_LOC(self);
    caddr_t clock_addr;
    int clock_intr();
    void level5intr_clock();
    vm_offset_t pte;

    clock_addr = OBIO_CLOCK;
    clock->clock_level = OBIO_DEFAULT_PARAM(int, obio_loc->obio_level, 5);
    clock->clock_va = (caddr_t) CLOCK_VA;
    if (!clock->clock_va) {
	printf(": not enough obio space\n");
	return;
    }
    obio_print(clock_addr, clock->clock_level);
    if (clock->clock_level != 5) {
	printf(": level != 5\n");
	return;
    }
    intersil_softc = clock;
    intersil_disable();
    set_clk_mode(0, IREG_CLOCK_ENAB_7, 0);
    isr_add_custom(clock->clock_level, level5intr_clock);
    set_clk_mode(IREG_CLOCK_ENAB_5, 0, 0);
    printf("\n");
}

int clock_count = 0;

void clock_intr(frame)
     clockframe frame;
{
    if ((clock_count % 1000) == 0) {
#if 0
	printf("clock_intr: total of %d interrupts received\n",
	       clock_count);
	printf("clock_intr: frame pc %x sr %x\n", frame.pc, frame.ps);
#endif
    }
    clock_count++;
    hardclock(frame);
}


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
 * also microtime support
 */

/*
 * Start the real-time clock.
 */
void startrtclock()
{
    char dummy;

    if (!intersil_softc)
	panic("clock: not initialized");

    intersil_clock->interrupt_reg = INTERSIL_INTER_CSECONDS;
    intersil_clock->command_reg = intersil_command(INTERSIL_CMD_RUN,
						   INTERSIL_CMD_IDISABLE);
    dummy = intersil_clear();
}

void enablertclock()
{
    unsigned char dummy;
	/* make sure irq5/7 stuff is resolved :) */

    dummy = intersil_clear();
    dummy++;
    intersil_clock->interrupt_reg = INTERSIL_INTER_CSECONDS;
    intersil_clock->command_reg = intersil_command(INTERSIL_CMD_RUN,
						   INTERSIL_CMD_IENABLE);
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

#define range_check_high(field, high) \
    now_state.field > high
#define range_check(field, low, high) \
	now_state.field < low || now_state.field > high

    if (range_check_high(csecs, 99) ||
	range_check_high(hours, 23) ||
	range_check_high(minutes, 59) ||
	range_check_high(seconds, 59) ||
	range_check(month, 1, 12) ||
	range_check(date, 1, 32) ||
	range_check_high(year, 99) ||
	range_check_high(day, 6)) return now;

    if ((INTERSIL_UNIX_BASE - INTERSIL_YEAR_BASE) > now_state.year)
	return now;

    for (i = INTERSIL_UNIX_BASE-INTERSIL_YEAR_BASE;
	 i < now_state.year; i++)
	if (INTERSIL_LEAP_YEAR(INTERSIL_YEAR_BASE+now_state.year))
	    now.tv_sec += SECS_PER_LEAP;
	else now.tv_sec += SECS_PER_YEAR;
    
    for (i = 1; i < now_state.month; i++) 
	now.tv_sec += SECS_PER_MONTH(i, INTERSIL_YEAR_BASE+now_state.year);
    now.tv_sec += SECS_DAY*(now_state.date-1);
    now.tv_sec += SECS_HOUR * now_state.hours;
    now.tv_sec += 60 * now_state.minutes;
    now.tv_sec += 60 * now_state.seconds;
    return now;
}
/*
 * Initialize the time of day register, based on the time base which is, e.g.
 * from a filesystem.
 */
void inittodr(base)
	time_t base;
{
    struct timeval clock_time;
    long diff_time;
    void resettodr();

    clock_time = intersil_to_timeval();

    if (!base) goto set_time;
    if (!clock_time.tv_sec && (base <= 0)) goto set_time;
	
    if (clock_time.tv_sec < base) {
	printf("WARNING: real-time clock reports a time earlier than last\n");
	printf("         write to root filesystem.  Trusting filesystem...\n");
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

void timeval_to_intersil(now, map)
     struct timeval now;
     struct intersil_map *map;
{

    for (map->year = INTERSIL_UNIX_BASE-INTERSIL_YEAR_BASE;
	 now.tv_sec > SECS_YEAR(map->year);
	 map->year++)
	now.tv_sec -= SECS_YEAR(map->year);

    for (map->month = 1; now.tv_sec >=0; map->month++)
	now.tv_sec -= SECS_PER_MONTH(map->month, INTERSIL_YEAR_BASE+map->year);

    map->month--;
    now.tv_sec -= SECS_PER_MONTH(map->month, INTERSIL_YEAR_BASE+map->year);

    map->date = now.tv_sec % SECS_DAY ;
    now.tv_sec /= SECS_DAY;
    map->minutes = now.tv_sec %60;
    now.tv_sec /= 60;
    map->seconds = now.tv_sec %60;
    now.tv_sec /= 60;
    map->day = map->date %7;
    map->csecs = now.tv_usec / 10000;
}


/*   
 * Resettodr restores the time of day hardware after a time change.
 */
void resettodr()
{
    struct intersil_map hdw_format;

    timeval_to_intersil(time, &hdw_format);
    intersil_clock->command_reg = intersil_command(INTERSIL_CMD_STOP,
						   INTERSIL_CMD_IDISABLE);

    intersil_clock->counters.csecs    =    hdw_format.csecs   ;
    intersil_clock->counters.hours    =    hdw_format.hours   ;
    intersil_clock->counters.minutes  =    hdw_format.minutes ;
    intersil_clock->counters.seconds  =    hdw_format.seconds ;
    intersil_clock->counters.month    =    hdw_format.month   ;
    intersil_clock->counters.date     =    hdw_format.date    ;
    intersil_clock->counters.year     =    hdw_format.year    ;
    intersil_clock->counters.day      =    hdw_format.day     ;
    
    intersil_clock->command_reg = intersil_command(INTERSIL_CMD_RUN,
						   INTERSIL_CMD_IENABLE);
}

void microtime(tvp)
     register struct timeval *tvp;
{
    int s;
    static struct timeval lasttime;

    /* as yet...... this makes little sense*/
    
    *tvp = time;
    if (tvp->tv_sec == lasttime.tv_sec &&
	tvp->tv_usec <= lasttime.tv_usec &&
	(tvp->tv_usec = lasttime.tv_usec + 1) > 1000000) {
	tvp->tv_sec++;
	tvp->tv_usec -= 1000000;
    }
    lasttime = *tvp;
    splx(s);
}


