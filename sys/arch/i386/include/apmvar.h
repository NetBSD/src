/*	$NetBSD: apmvar.h,v 1.9.2.1 2001/01/18 09:22:36 bouyer Exp $	*/
/*-
 * Copyright (c) 1995 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by John Kohl.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef __I386_APM_H__
#define __I386_APM_H__

/* Advanced Power Management (v1.0 and v1.1 specification)
 * functions/defines/etc.
 */
#define APM_BIOS_FNCODE	(0x53)
#define APM_SYSTEM_BIOS	(0x15)
#define APM_BIOS_FN(x)	((APM_BIOS_FNCODE<<8)|(x))

/*
 * APM info bits from BIOS
 */
#define APM_16BIT_SUPPORT	0x01
#define APM_32BIT_SUPPORT	0x02
#define APM_CPUIDLE_SLOW	0x04
#define APM_DISABLED		0x08
#define APM_DISENGAGED		0x10

#define	APM_ERR_CODE(regs)	(((regs)->AX & 0xff00) >> 8)
#define	APM_ERR_PM_DISABLED	0x01
#define	APM_ERR_REALALREADY	0x02
#define	APM_ERR_NOTCONN		0x03
#define	APM_ERR_16ALREADY	0x05
#define	APM_ERR_16NOTSUPP	0x06
#define	APM_ERR_32ALREADY	0x07
#define	APM_ERR_32NOTSUPP	0x08
#define	APM_ERR_UNRECOG_DEV	0x09
#define	APM_ERR_ERANGE		0x0A
#define	APM_ERR_NOTENGAGED	0x0B
#define	APM_ERR_EOPNOTSUPP	0x0C
#define	APM_ERR_RTIMER_DISABLED	0x0D
#define APM_ERR_UNABLE		0x60
#define APM_ERR_NOEVENTS	0x80
#define	APM_ERR_NOT_PRESENT	0x86

#define APM_DEV_APM_BIOS	0x0000
#define APM_DEV_ALLDEVS		0x0001
/* device classes are high byte; device IDs go in low byte */
#define		APM_DEV_DISPLAY(x)	(0x0100|((x)&0xff))
#define		APM_DEV_DISK(x)		(0x0200|((x)&0xff))
#define		APM_DEV_PARALLEL(x)	(0x0300|((x)&0xff))
#define		APM_DEV_SERIAL(x)	(0x0400|((x)&0xff))
#define		APM_DEV_NETWORK(x)	(0x0500|((x)&0xff))
#define		APM_DEV_PCMCIA(x)	(0x0600|((x)&0xff))
#define		APM_DEV_BATTERY(x)	(0x8000|((x)&0xff))	/* V1.2 */
#define		APM_DEV_ALLUNITS	0xff

#define	APM_INSTALLATION_CHECK	0x00	/* int15 only */
#define		APM_INSTALL_SIGNATURE	0x504d	/* %bh = 'P', %bl = 'M' */
#define	APM_REALMODE_CONNECT	0x01	/* int15 only */
#define	APM_16BIT_CONNECT	0x02	/* int15 only */
#define	APM_32BIT_CONNECT	0x03	/* int15 only */
#define APM_DISCONNECT		0x04	/* %bx = APM_DEV_APM_BIOS */
#define APM_CPU_IDLE		0x05
#define APM_CPU_BUSY		0x06
#define APM_SET_PWR_STATE	0x07
#define		APM_SYS_READY	0x0000	/* %cx */
#define		APM_SYS_STANDBY	0x0001
#define		APM_SYS_SUSPEND	0x0002
#define		APM_SYS_OFF	0x0003
#define		APM_LASTREQ_INPROG	0x0004
#define		APM_LASTREQ_REJECTED	0x0005

/* system standby is device ID (%bx) 0x0001, APM_SYS_STANDBY */
/* system suspend is device ID (%bx) 0x0001, APM_SYS_SUSPEND */

#define APM_PWR_MGT_ENABLE	0x08
#define		APM_MGT_ALL	0xffff	/* %bx */
#define		APM_MGT_DISABLE	0x0	/* %cx */
#define		APM_MGT_ENABLE	0x1

#define APM_SYSTEM_DEFAULTS	0x09
#define		APM_DEFAULTS_ALL	0xffff	/* %bx */
#define		APM_DEFAULTS_ALL_V12	0x0001	/* %bx */

#define APM_POWER_STATUS	0x0a
#define		APM_AC_OFF		0x00
#define		APM_AC_ON		0x01
#define		APM_AC_BACKUP		0x02
#define		APM_AC_UNKNOWN		0xff
/* the first set of battery constants is 1.0 style values;
   the second set is 1.1 style bit definitions */
#define		APM_BATT_HIGH		0x00
#define		APM_BATT_LOW		0x01
#define		APM_BATT_CRITICAL	0x02
#define		APM_BATT_CHARGING	0x03
#define		APM_BATT_ABSENT		0x04 /* Software only--not in spec! */
#define		APM_BATT_UNKNOWN	0xff

#define		APM_BATT_FLAG_HIGH	0x01
#define		APM_BATT_FLAG_LOW	0x02
#define		APM_BATT_FLAG_CRITICAL	0x04
#define		APM_BATT_FLAG_CHARGING	0x08
#define		APM_BATT_FLAG_NOBATTERY	0x10
#define		APM_BATT_FLAG_NO_SYSTEM_BATTERY	0x80
#define		APM_BATT_FLAG_UNKNOWN	0xff

#define		APM_BATT_LIFE_UNKNOWN	0xff
#define		APM_BATT_STATE(regp) ((regp)->BX & 0xff)
#define		APM_BATT_FLAGS(regp) (((regp)->CX & 0xff00) >> 8)
#define		APM_AC_STATE(regp) (((regp)->BX & 0xff00) >> 8)
#define		APM_BATT_LIFE(regp) ((regp)->CX & 0xff) /* in % */
/* BATT_REMAINING returns minutes remaining */
#define		APM_BATT_REMAINING(regp) (((regp)->DX & 0x8000) ? \
					  ((regp)->DX & 0x7fff) : \
					  ((regp)->DX & 0x7fff)/60)
#define		APM_BATT_REM_VALID(regp) (((regp)->DX & 0xffff) != 0xffff)
#define		APM_BATTERY_COUNT(regp) ((regp)->SI)

#define	APM_GET_PM_EVENT	0x0b
#define		APM_STANDBY_REQ		0x0001 /* %bx on return */
#define		APM_SUSPEND_REQ		0x0002
#define		APM_NORMAL_RESUME	0x0003
#define		APM_CRIT_RESUME		0x0004 /* suspend/resume happened
						  without us */
     /* If set, the pccard sockets were powered off in the
	suspend/standby state (V1.2 only) */
#define		APM_PCCARD_POWEREDOFF(regp) ((regp)->CX & 0x0001)

#define		APM_BATTERY_LOW		0x0005
#define		APM_POWER_CHANGE	0x0006
#define		APM_UPDATE_TIME		0x0007
#define		APM_CRIT_SUSPEND_REQ	0x0008
#define		APM_USER_STANDBY_REQ	0x0009
#define		APM_USER_SUSPEND_REQ	0x000A
#define		APM_SYS_STANDBY_RESUME	0x000B
#define		APM_CAP_CHANGE		0x000C	/* V1.2 */

#define	APM_GET_POWER_STATE	0x0c
     /* device to query in %bx */
     /* returns power states in %cx */
#define	APM_DEVICE_MGMT_ENABLE	0x0d
     /* device to enable/disable in %bx */

#define	APM_DRIVER_VERSION	0x0e
/* %bx should be DEV value (APM_DEV_APM_BIOS)
   %ch = driver major vno
   %cl = driver minor vno
   return: %ah = conn major; %al = conn minor
   */
#define		APM_CONN_MINOR(regp) ((regp)->AX & 0xff)
#define		APM_CONN_MAJOR(regp) (((regp)->AX & 0xff00) >> 8)

#define APM_PWR_MGT_ENGAGE	0x0F
     /* device in %bx */
#define		APM_MGT_DISENGAGE	0x0	/* %cx */
#define		APM_MGT_ENGAGE		0x1

#define APM_GET_CAPABILITIES	0x10
     /* device in %bx (APM_DEV_APM_BIOS) */
     /* %bl: number of batteries */
#define		APM_NBATTERIES(regp) ((regp)->BX & 0xff)
     /* %cx capabilities */
#define		APM_GLOBAL_STANDBY	0x0001
#define		APM_GLOBAL_SUSPEND	0x0002
#define		APM_RTIMER_STANDBY	0x0004 /* resume timer wakes standby */
#define		APM_RTIMER_SUSPEND	0x0008 /* resume timer wakes suspend */
#define		APM_IRRING_SUSPEND	0x0010 /* internal ring wakes suspend */
#define		APM_IRRING_STANDBY	0x0020 /* internal ring wakes standby */
#define		APM_PCRING_SUSPEND	0x0040 /* pccard ring wakes suspend */
#define		APM_PCRING_STANDBY	0x0080 /* pccard ring wakes standby */

#define	APM_RESUME_TIMER_CTL	0x11	/* get/set/disable resume timer */
     /* device in %bx (APM_DEV_APM_BIOS) */
     /* function code in %cl */
#define		APM_RTIMER_DISABLE	0x00
#define		APM_RTIMER_GET		0x01
#define		APM_RTIMER_SET		0x02
     /* if %cl = set, or %cl = get, then: */
     /* %ch: seconds */
     /* %dh: hours */
     /* %dl: minutes */
     /* %sih: month (bcd), %sil: day (bcd) */
     /* %di: year (bcd, 4 digits) */
#define APM_RESUME_RING_CTL		0x12
     /* device in %bx (APM_DEV_APM_BIOS) */
     /* %cx: function code */
#define		APM_RRING_DISABLE	0x0000
#define		APM_RRING_ENABLE	0x0001
#define		APM_RRING_GET		0x0002
     /* returns state in %cx */
#define		APM_RRING_DISABLED	0x0000
#define		APM_RRING_ENABLED	0x0001

#define APM_TIMER_REQS_CTL	0x13
     /* device in %bx (APM_DEV_APM_BIOS) */
     /* %cx: function code */
#define		APM_TIMER_REQS_DISABLE	0x0000
#define		APM_TIMER_REQS_ENABLE	0x0001
#define		APM_TIMER_REQS_GET	0x0002
     /* returns state in %cx */
#define		APM_TIMER_REQS_DISABLED	0x0000
#define		APM_TIEMR_REQS_ENABLED	0x0001

#define APM_OEM			0x80

/*
 * APM info word from the real-mode handler is adjusted to put
 * major/minor version in low half and support bits in upper half.
 */
#define	APM_MAJOR_VERS(info) (((info)&0xff00)>>8)
#define	APM_MINOR_VERS(info) ((info)&0xff)

#define APM_16BIT_SUPPORTED	(APM_16BIT_SUPPORT << 16)
#define APM_32BIT_SUPPORTED	(APM_32BIT_SUPPORT << 16)
#define APM_IDLE_SLOWS		(APM_CPUIDLE_SLOW << 16)
#define APM_BIOS_PM_DISABLED	(APM_DISABLED << 16)
#define APM_BIOS_PM_DISENGAGED	(APM_DISENGAGED << 16)

/*
 * virtual & physical address of the trampoline
 * that we use: page 1.
 */
#define APM_BIOSTRAMP	NBPG


#ifndef _LOCORE

/* filled in by apmcall */ 

struct apm_connect_info {
	u_int apm_code32_seg_base;	/* real-mode style segment selector */
	u_int apm_code16_seg_base;
	u_int apm_data_seg_base;
	u_int apm_entrypt;
	u_short	apm_segsel;		/* segment selector for APM */
	u_short _pad1;
	u_int apm_code32_seg_len;
	u_int apm_code16_seg_len;
	u_int apm_data_seg_len;
	u_int apm_detail;
};

struct apm_event_info {
	u_int type;
	u_int index;
	u_int spare[8];
};

struct apm_power_info {
	u_char battery_state;
	u_char ac_state;
	u_char battery_life;
	u_char spare1;
	u_int minutes_left;		/* estimate */
	u_int spare2[6];
};

struct apm_ctl {
	u_int dev;
	u_int mode;
};

#define	APM_IOC_REJECT	_IOW('A', 0, struct apm_event_info) /* reject request # */
#define	APM_IOC_STANDBY	_IO('A', 1)	/* put system into standby */
#define	APM_IOC_SUSPEND	_IO('A', 2)	/* put system into suspend */
#define	APM_IOC_GETPOWER _IOR('A', 3, struct apm_power_info) /* fetch battery state */
#define	APM_IOC_NEXTEVENT _IOR('A', 4, struct apm_event_info) /* fetch event */
#define	APM_IOC_DEV_CTL	_IOW('A', 5, struct apm_ctl) /* put device into mode */

struct apm_attach_args {
	char *aaa_busname;
};

#ifdef _KERNEL
extern struct apm_connect_info apminfo;	/* in locore */
extern int apmpresent;
extern int apmcall __P((int function, struct bioscallregs *regs));
extern void bioscall __P((int function, struct bioscallregs *regs));
extern void apm_cpu_busy __P((void));
extern void apm_cpu_idle __P((void));
extern void apminit __P((void));
int apm_set_powstate __P((u_int devid, u_int powstate));
extern int apm_busprobe __P((void));
#endif /* _KERNEL */
#endif /* _LOCORE */
#endif /* __i386_apm_h__ */
