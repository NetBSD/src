#ifndef _RTCLOCKB_H_
#define _RTCLOCKB_H_

/* clock used in the A2000, according to info I got from
   Harald Backert. */



struct rtclock2000 {
  u_int  :28, second2:4;	/* lower digit */
  u_int  :28, second1:4;	/* upper digit */
  u_int  :28, minute2:4;	/* lower digit */
  u_int  :28, minute1:4;	/* upper digit */
  u_int  :28, hour2:4;		/* lower digit */
  u_int  :28, hour1:4;		/* upper digit */
  u_int  :28, day2:4;		/* lower digit */
  u_int  :28, day1:4;		/* upper digit */
  u_int  :28, month2:4;		/* lower digit */
  u_int  :28, month1:4;		/* upper digit */
  u_int  :28, year2:4;		/* lower digit */
  u_int  :28, year1:4;		/* upper digit */
  u_int	 :28, week:4;		/* week */
  u_int  :28, control1:4;	/* control-byte 1 */
  u_int  :28, control2:4;	/* control-byte 2 */  
  u_int  :28, control3:4;	/* control-byte 3 */
};


/* commands written to control1, HOLD before reading the clock,
   FREE after done reading. */

#define CONTROL1_HOLD		(1<<0)
#define CONTROL1_BUSY		(1<<1)
#define CONTROL3_24HMODE	(1<<2)
#define HOUR1_PM		(1<<2)

#define FEBRUARY	2
#define	STARTOFTIME	1970
#define SECDAY		86400L
#define SECYR		(SECDAY * 365)

#define	leapyear(y)		(((y)%4)==0 && ((y)%100)!=0 || ((y)%400) == 0)
#define	range_test(n, l, h)	if ((n) < (l) || (n) > (h)) return(0)
#define	days_in_year(a) 	(leapyear(a) ? 366 : 365)
#define	days_in_month(a) 	(month_days[(a) - 1])

#endif /* _RTCLOCKB_H_ */
