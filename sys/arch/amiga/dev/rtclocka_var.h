#ifndef _RTCLOCKA_H_
#define _RTCLOCKA_H_

/* this seems to be what the battery backed up clock looks like
   on the A3000. I think different clock chips are used in other Amiga
   models, so we'll probably have to differentiate between them.
   
   Thanks ALOT to Holger Emden for sending me demo-code on how
   to read the clock!! */



struct rtclock3000 {
  u_int  :28, second2:4;	/* 0x03  lower digit */
  u_int  :28, second1:4;	/* 0x07  upper digit */
  u_int  :28, minute2:4;	/* 0x0b  lower digit */
  u_int  :28, minute1:4;	/* 0x0f  upper digit */
  u_int  :28, hour2:4;		/* 0x13  lower digit */
  u_int  :28, hour1:4;		/* 0x17  upper digit */
  u_int  :28, weekday:4;	/* 0x1b */
  u_int  :28, day2:4;		/* 0x1f  lower digit */
  u_int  :28, day1:4;		/* 0x23  upper digit */
  u_int  :28, month2:4;		/* 0x27  lower digit */
  u_int  :28, month1:4;		/* 0x2b  upper digit */
  u_int  :28, year2:4;		/* 0x2f  lower digit */
  u_int  :28, year1:4;		/* 0x33  upper digit */
  u_int  :28, control1:4;	/* 0x37  control-byte 1 */
  u_int  :28, control2:4;	/* 0x3b  control-byte 2 */  
  u_int  :28, control3:4;	/* 0x3f  control-byte 3 */
};


/* commands written to control1, HOLD before reading the clock,
   FREE after done reading. */

#define CONTROL1_HOLD_CLOCK	0
#define CONTROL1_FREE_CLOCK	9

#define FEBRUARY	2
#define	STARTOFTIME	1970
#define SECDAY		86400L
#define SECYR		(SECDAY * 365)

#define BBC_SET_REG 	0xe0
#define BBC_WRITE_REG	0xc2
#define BBC_READ_REG	0xc3
#define NUM_BBC_REGS	12

#define	leapyear(y)		(((y)%4)==0 && ((y)%100)!=0 || ((y)%400) == 0)
#define	range_test(n, l, h)	if ((n) < (l) || (n) > (h)) return(0)
#define	days_in_year(a) 	(leapyear(a) ? 366 : 365)
#define	days_in_month(a) 	(month_days[(a) - 1])

#endif /* _RTCLOCKA_H_ */
