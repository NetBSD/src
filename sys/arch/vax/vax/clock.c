/*	$NetBSD: clock.c,v 1.4 1995/02/13 00:46:04 ragge Exp $	*/

/******************************************************************************

  clock.c

******************************************************************************/

#include <sys/param.h>
#include <sys/kernel.h>
#include "machine/mtpr.h"

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

  unsigned long tmp_year,sluttid=fs_time;

  year=(fs_time/SEC_PER_DAY/365)*365*SEC_PER_DAY;
  tmp_year=year/SEC_PER_DAY/365+2;
  year_len=100*SEC_PER_DAY*((tmp_year%4&&tmp_year!=32)?365:366);

  if(todrstopped){
    printf("Internal clock not started. Using time from file system.\n");
    mtpr((fs_time-year)*100+1, PR_TODR); /* +1 so the clock won't be stopped */
    todrstopped=0;
  } else if(mfpr(PR_TODR)/100>fs_time-year+SEC_PER_DAY*3) {
    printf("WARNING: Clock has gained %d days - CHECK AND RESET THE DATE.\n",
	   (mfpr(PR_TODR)/100-(fs_time-year))/SEC_PER_DAY);
    sluttid=year+(mfpr(PR_TODR)/100);
  } else if(mfpr(PR_TODR)/100<fs_time-year) {
    printf("WARNING: Clock has lost time! CHECK AND RESET THE DATE.\n");
  } else sluttid=year+(mfpr(PR_TODR)/100);
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
  mtpr((time.tv_sec-year)*100+1, PR_TODR);
  todrstopped=0;
}


