#ifndef __DS1501_H__
#define __DS1501_H__

/* Dallas Semiconductor DS1501/DS1511 Watchdog RTC */

#define DS_SECONDS	0x1ff9
#define DS_MINUTES	0x1ffa
#define DS_HOURS	0x1ffb
#define DS_DAY		0x1ffc
#define DS_DATE		0x1ffd
#define DS_MONTH	0x1ffe
#define DS_YEAR		0x1fff
#define DS_CENTURY	0x1ff8


#define DS_CTL_R	0x40	/* R bit in the century register */
#define DS_CTL_W	0x80	/* W bit in the century register */
#define DS_CTL_RW	(DS_CTL_R|DS_CTL_W)

#define DS_CTL_OSC	0x80	/* ~OSC BIT in the seconds register */

#define DS_CTL_BF	0x80	/* BF(battery failure) bit in the day register */
#define DS_CTL_FT	0x40	/* FT(frequency test) bit in the day register */

#define DS_RAM_SIZE	0x1ff8

#define DS_SIZE		0x2000

#endif /* __DS1501_H__ */
