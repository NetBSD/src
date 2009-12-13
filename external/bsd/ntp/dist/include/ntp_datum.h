/*	$NetBSD: ntp_datum.h,v 1.1.1.1 2009/12/13 16:54:50 kardel Exp $	*/

struct btfp_time                /* Structure for reading 5 time words   */
                                /* in one ioctl(2) operation.           */
{
  unsigned short btfp_time[5];  /* Time words 0,1,2,3, and 4. (16bit)*/
};

/***** Simple ioctl commands *****/

#define RUNLOCK   _IO('X',19)                   /* Release Capture Lockout */
#define RCR0      _IOR('X',22,unsigned int)     /* Read control register */
#define WCR0      _IOW('X',23,unsigned int)     /* Write control register */

/***** Compound ioctl commands *****/

/* Read all 5 time words in one call.   */
#define READTIME        _IOR('X',32,struct btfp_time)
#define VMEFD "/dev/btfp0"

 struct vmedate {               /* structure returned by get_vmetime.c */
         unsigned short year;
         unsigned short doy;
         unsigned short hr;
         unsigned short mn;
         unsigned short sec;
         unsigned long frac;
         unsigned short status;
         };

#define PRIO    120               /* set the realtime priority */
#define NREGS 7                    /* number of registers we will use */
