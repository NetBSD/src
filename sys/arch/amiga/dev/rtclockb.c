/* this provides a device interface to the realtime clock
   used on the A2000.

   To support different clocks: the config file defines a 
   realtime clock "rtclock". Write your own driver that if
   matched by existing hardware returns 1 from its init
   function, and set the function pointer "gettod" to 
   a function returning the number of seconds elapsed since
   1970-1-1.

   TODO: could add read-write interface to turn this into
         a real /dev/rtclock device, that would allow reading
	 and setting of the clock very easily. 
 *
 *	$Id: rtclockb.c,v 1.4 1994/02/13 21:10:54 chopps Exp $
 */

#include "rtclockb.h"
#if NRTCLOCKB > 0

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/dkstat.h>
#include <sys/disklabel.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/file.h>

#include <amiga/dev/device.h>
#include <amiga/dev/rtclockb_var.h>

int	rtclockbinit (register struct amiga_device *ad);

static long rtgettod ();
static int  rtsettod ();

/* amiga/clock.c calls thru this vector, if it is set, to read
   the realtime clock */
extern long (*gettod)();
extern int (*settod)();

/* since there's only one such clock on the A2000, we can
   savely store its address in a static variable */
static struct rtclock2000 *rt = 0;

struct driver rtclockbdriver = {
  (int (*)(void *)) rtclockbinit, "rtclock",
};


int
rtclockbinit (register struct amiga_device *ad)
{
  /* verify we're indeed present */
  if (ad->amiga_addr)
    {
      rt = (struct rtclock2000 *) ad->amiga_addr;
      if (rtgettod ())
	{
	  gettod = rtgettod;
	  settod = rtsettod;
	  printf ("Realtime clock A2000\n");
	  return 1;
	}
      else
	printf ("Realtime clock A2000 malfunctioning, ignored.\n");
    }

  return 0;
}


static int month_days[12] = {
	31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

static long
rtgettod ()
{
  register int i;
  register u_long tmp;
  int year, month, day, hour, min, sec;

  /* hold clock */
  rt->control1 |= CONTROL1_HOLD;
  while (rt->control1 & CONTROL1_BUSY) ;

  /* read it */
  sec   = rt->second1 * 10 + rt->second2;
  min   = rt->minute1 * 10 + rt->minute2;
  hour  = (rt->hour1 & 3)  * 10 + rt->hour2;
  day   = rt->day1    * 10 + rt->day2;
  month = rt->month1  * 10 + rt->month2;
  year  = rt->year1   * 10 + rt->year2   + 1900;

  if (! (rt->control3 & CONTROL3_24HMODE))
    {
      if (!(rt->hour1 & HOUR1_PM) && hour == 12)
	hour = 0;
      else if ((rt->hour1 & HOUR1_PM) && hour != 12)
	hour += 12;
    }


  /* let it run again.. */
  rt->control1 &= ~CONTROL1_HOLD;

#if 0
  printf ("rt: sec=%d, min=%d, hour=%d, day=%d, mon=%d, year=%d\n",
	  sec, min, hour, day, month, year);
#endif

  range_test(hour, 0, 23);
  range_test(day, 1, 31);
  range_test(month, 1, 12);
  range_test(year, STARTOFTIME, 2000);
  
  tmp = 0;
  
  for (i = STARTOFTIME; i < year; i++)
    tmp += days_in_year(i);
  if (leapyear(year) && month > FEBRUARY)
    tmp++;
  
  for (i = 1; i < month; i++)
    tmp += days_in_month(i);
  
  tmp += (day - 1);
  tmp = ((tmp * 24 + hour) * 60 + min) * 60 + sec;
  
  return tmp;
}


int
rtsettod (tim)
     long tim;
{
  /* I don't know if setting the clock is analogous
     to reading it, I don't have demo-code for setting.
     just give it a try.. */

  register int i;
  register long hms, day;
  u_char sec1, sec2;
  u_char min1, min2;
  u_char hour1, hour2;
  u_char day1, day2;
  u_char mon1, mon2;
  u_char year1, year2;

  /* there seem to be problems with the bitfield addressing
     currently used.. */
  return 0;

#if 0
  if (! rt)
    return 0;

  /* prepare values to be written to clock */
  day = tim / SECDAY;
  hms = tim % SECDAY;

  
  hour2 = hms / 3600;
  hour1 = hour2 / 10;
  hour2 %= 10;

  min2 = (hms % 3600) / 60;
  min1 = min2 / 10;
  min2 %= 10;


  sec2 = (hms % 3600) % 60;
  sec1 = sec2 / 10;
  sec2 %= 10;

  /* Number of years in days */
  for (i = STARTOFTIME - 1900; day >= days_in_year(i); i++)
    day -= days_in_year(i);
  year1 = i / 10;
  year2 = i % 10;

  /* Number of months in days left */
  if (leapyear(i))
    days_in_month(FEBRUARY) = 29;
  for (i = 1; day >= days_in_month(i); i++)
    day -= days_in_month(i);
  days_in_month(FEBRUARY) = 28;

  mon1 = i / 10;
  mon2 = i % 10;
  
  /* Days are what is left over (+1) from all that. */
  day ++;
  day1 = day / 10;
  day2 = day % 10;

  rt->control1 = CONTROL1_HOLD_CLOCK;
  rt->second1 = sec1;
  rt->second2 = sec2;
  rt->minute1 = min1;
  rt->minute2 = min2;
  rt->hour1   = hour1;
  rt->hour2   = hour2;
  rt->day1    = day1;
  rt->day2    = day2;
  rt->month1  = mon1;
  rt->month2  = mon2;
  rt->year1   = year1;
  rt->year2   = year2;
  rt->control2 = CONTROL1_FREE_CLOCK;

  return 1;
#endif
}
#endif /* NRTCLOCKB */
