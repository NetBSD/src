/*	$NetBSD: clock.c,v 1.5 1995/02/23 17:53:48 ragge Exp $	*/

/******************************************************************************

  clock.c

******************************************************************************/

#include <sys/param.h>
#include <sys/kernel.h>
#include "machine/mtpr.h"
#include "machine/sid.h"

#define SEC_PER_DAY (60*60*24)

extern int todrstopped;

static unsigned long year;     /*  start of current year in seconds */
static unsigned long year_len; /* length of current year in 100th of seconds */


/*******


*******/

void microtime(struct timeval *tod) {

  unsigned long int_time=mfpr(PR_TODR);
  unsigned long tmp_year;

  if(int_time>year_len) {
    mtpr(mfpr(PR_TODR)-year_len, PR_TODR);
    year+=year_len/100;
    tmp_year=year/SEC_PER_DAY/365+2;
    year_len=100*SEC_PER_DAY*((tmp_year%4&&tmp_year!=32)?365:366);
  }

  tod->tv_sec=year+(int_time/100);
  tod->tv_usec=int_time%100;
}


/*
 * Sets year to the year in fs_time and then calculates the number of
 * 100th of seconds in the current year and saves that info in year_len.
 * fs_time contains the time set in the superblock in the root filesystem.
 * If the clock is started, it then checks if the time is valid
 * compared with the time in fs_time. If the clock is stopped, an
 * alert is printed and the time is temporary set to the time in fs_time.
 */

void inittodr(time_t fs_time) {

  unsigned long tmp_year,sluttid=fs_time,year_ticks;
  int clock_stopped;

  year=(fs_time/SEC_PER_DAY/365)*365*SEC_PER_DAY;
  tmp_year=year/SEC_PER_DAY/365+2;
  year_len=100*SEC_PER_DAY*((tmp_year%4&&tmp_year!=32)?365:366);

  switch (cpunumber) {
#if VAX750
  case VAX_750:
    year_ticks = mfpr(PR_TODR);
    clock_stopped = todrstopped;
    break;
#endif
#if VAX630 || VAX410
  case VAX_78032:
    year_ticks = uvaxII_gettodr(&clock_stopped);
    break;
#endif
  default:
    year_ticks = 0;
    clock_stopped = 1;
  };

  if(clock_stopped){
    printf("Internal clock not started. Using time from file system.\n");
    switch (cpunumber) {
#if VAX750
    case VAX_750:
      mtpr((fs_time-year)*100+1, PR_TODR); /*+1 so the clock won't be stopped */
      break;
#endif
#if VAX630 || VAX410
    case VAX_78032:
      uvaxII_settodr((fs_time-year)*100+1);
      break;
#endif
    };
    todrstopped=0;
  } else if(year_ticks/100>fs_time-year+SEC_PER_DAY*3) {
    printf("WARNING: Clock has gained %d days - CHECK AND RESET THE DATE.\n",
         (year_ticks/100-(fs_time-year))/SEC_PER_DAY);
    sluttid=year+(year_ticks/100);
  } else if(year_ticks/100<fs_time-year) {
    printf("WARNING: Clock has lost time! CHECK AND RESET THE DATE.\n");
  } else sluttid=year+(year_ticks/100);
  time.tv_sec=sluttid;
}

/*   
 * Resettodr restores the time of day hardware after a time change.
 */

void resettodr(void) {

  unsigned long tmp_year;

  year=(time.tv_sec/SEC_PER_DAY/365)*365*SEC_PER_DAY;
  tmp_year=year/SEC_PER_DAY/365+2;
  year_len=100*SEC_PER_DAY*((tmp_year%4&&tmp_year!=32)?365:366);
  switch (cpunumber) {
#if VAX750
  case VAX_750:
    mtpr((time.tv_sec-year)*100+1, PR_TODR);
    break;
#endif
#if VAX630 || VAX410
  case VAX_78032:
    uvaxII_settodr((time.tv_sec-year)*100+1);
    break;
#endif
  };
  todrstopped=0;
}

/*
 * Unfortunately the 78032 cpu chip (MicroVAXII cpu) does not have a functional
 * todr register, so this function is necessary.
 * (the x and y variables are used to confuse the optimizer enough to ensure
 *  that the code actually loops:-)
 */
int
todr()
  {
      int delaycnt, x = 4, y = 4;
      static int todr_val;

      if (cpunumber != VAX_78032)
              return (mfpr(PR_TODR));

      /*
       * Loop for approximately 10msec and then return todr_val + 1.
       */
      delaycnt = 5000;
      while (delaycnt > 0)
              delaycnt = delaycnt - x + 3 + y - 4;
      return (++todr_val);
}
