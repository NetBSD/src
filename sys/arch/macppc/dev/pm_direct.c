/*	$NetBSD: pm_direct.c,v 1.29.6.1 2006/04/22 02:40:57 simonb Exp $	*/

/*
 * Copyright (C) 1997 Takashi Hamada
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
 *  This product includes software developed by Takashi Hamada
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/* From: pm_direct.c 1.3 03/18/98 Takashi Hamada */

/*
 * TODO : Check bounds on PMData in pmgrop
 *		callers should specify how much room for data is in the buffer
 *		and that should be respected by the pmgrop
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pm_direct.c,v 1.29.6.1 2006/04/22 02:40:57 simonb Exp $");

#ifdef DEBUG
#ifndef ADB_DEBUG
#define ADB_DEBUG
#endif
#endif

/* #define	PM_GRAB_SI	1 */

#include <sys/param.h>
#include <sys/cdefs.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <machine/adbsys.h>
#include <machine/autoconf.h>
#include <machine/cpu.h>

#include <dev/ofw/openfirm.h>

#include <macppc/dev/adbvar.h>
#include <macppc/dev/pm_direct.h>
#include <macppc/dev/viareg.h>

extern int adb_polling;		/* Are we polling?  (Debugger mode) */

/* hardware dependent values */
#define ADBDelay 100		/* XXX */

/* useful macros */
#define PM_SR()			read_via_reg(VIA1, vSR)
#define PM_VIA_INTR_ENABLE()	write_via_reg(VIA1, vIER, 0x90)
#define PM_VIA_INTR_DISABLE()	write_via_reg(VIA1, vIER, 0x10)
#define PM_VIA_CLR_INTR()	write_via_reg(VIA1, vIFR, 0x90)

#define PM_SET_STATE_ACKON()	via_reg_or(VIA2, vBufB, 0x10)
#define PM_SET_STATE_ACKOFF()	via_reg_and(VIA2, vBufB, ~0x10)
#define PM_IS_ON		(0x08 == (read_via_reg(VIA2, vBufB) & 0x08))
#define PM_IS_OFF		(0x00 == (read_via_reg(VIA2, vBufB) & 0x08))

/*
 * Variables for internal use
 */
u_short	pm_existent_ADB_devices = 0x0;	/* each bit expresses the existent ADB device */
u_int	pm_LCD_brightness = 0x0;
u_int	pm_LCD_contrast = 0x0;
u_int	pm_counter = 0;			/* clock count */

static enum batt_type { BATT_COMET, BATT_HOOPER, BATT_SMART } pmu_batt_type;
static int	pmu_nbatt;
static int	strinlist(const char *, char *, int);
static enum pmu_type { PMU_UNKNOWN, PMU_OHARE, PMU_G3, PMU_KEYLARGO } pmu_type;

/* these values shows that number of data returned after 'send' cmd is sent */
signed char pm_send_cmd_type[] = {
	  -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
	  -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
	0x01, 0x01,   -1,   -1,   -1,   -1,   -1,   -1,
	0x00, 0x00,   -1,   -1,   -1,   -1,   -1, 0x00,
	  -1, 0x00, 0x02, 0x01, 0x01,   -1,   -1,   -1,
	0x00,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
	0x04, 0x14,   -1, 0x03,   -1,   -1,   -1,   -1,
	0x00, 0x00, 0x02, 0x02,   -1,   -1,   -1,   -1,
	0x01, 0x01,   -1,   -1,   -1,   -1,   -1,   -1,
	0x00, 0x00,   -1,   -1, 0x01,   -1,   -1,   -1,
	0x01, 0x00, 0x02, 0x02,   -1, 0x01, 0x03, 0x01,
	0x00, 0x01, 0x00, 0x00, 0x00,   -1,   -1,   -1,
	0x02,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   -1,   -1,
	0x01, 0x01, 0x01,   -1,   -1,   -1,   -1,   -1,
	0x00, 0x00,   -1,   -1,   -1,   -1, 0x04, 0x04,
	0x04,   -1, 0x00,   -1,   -1,   -1,   -1,   -1,
	0x00,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
	0x01, 0x02,   -1,   -1,   -1,   -1,   -1,   -1,
	0x00, 0x00,   -1,   -1,   -1,   -1,   -1,   -1,
	0x02, 0x02, 0x02, 0x04,   -1, 0x00,   -1,   -1,
	0x01, 0x01, 0x03, 0x02,   -1,   -1,   -1,   -1,
	  -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
	  -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
	  -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
	  -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
	0x00,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
	0x01, 0x01,   -1,   -1, 0x00, 0x00,   -1,   -1,
	  -1, 0x04, 0x00,   -1,   -1,   -1,   -1,   -1,
	0x03,   -1, 0x00,   -1, 0x00,   -1,   -1, 0x00,
	  -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
	  -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1
};

/* these values shows that number of data returned after 'receive' cmd is sent */
signed char pm_receive_cmd_type[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	  -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x02, 0x02,   -1,   -1,   -1,   -1,   -1, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	  -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x05, 0x15,   -1, 0x02,   -1,   -1,   -1,   -1,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x02, 0x02,   -1,   -1,   -1,   -1,   -1,   -1,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x02, 0x00, 0x03, 0x03,   -1,   -1,   -1,   -1,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x04, 0x04, 0x03, 0x09,   -1,   -1,   -1,   -1,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	  -1,   -1,   -1,   -1,   -1,   -1, 0x01, 0x01,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x06,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x02, 0x02,   -1,   -1,   -1,   -1,   -1,   -1,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x02, 0x00, 0x00, 0x00,   -1,   -1,   -1,   -1,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	  -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	  -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x02, 0x02,   -1,   -1, 0x02,   -1,   -1,   -1,
	0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
	  -1,   -1, 0x02,   -1,   -1,   -1,   -1, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	  -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
};


/*
 * Define the private functions
 */

/* for debugging */
#ifdef ADB_DEBUG
void	pm_printerr __P((const char *, int, int, const char *));
#endif

int	pm_wait_busy __P((int));
int	pm_wait_free __P((int));

static int	pm_receive __P((u_char *));
static int	pm_send __P((u_char));

/* these functions are called from adb_direct.c */
void	pm_setup_adb __P((void));
void	pm_check_adb_devices __P((int));
int	pm_adb_op __P((u_char *, adbComp *, volatile int *, int));

/* these functions also use the variables of adb_direct.c */
void	pm_adb_get_TALK_result __P((PMData *));
void	pm_adb_get_ADB_data __P((PMData *));


/*
 * These variables are in adb_direct.c.
 */
extern u_char	*adbBuffer;	/* pointer to user data area */
extern adbComp	*adbCompRout;	/* pointer to the completion routine */
extern volatile int *adbCompData;	/* pointer to the completion routine data */
extern int	adbWaiting;	/* waiting for return data from the device */
extern int	adbWaitingCmd;	/* ADB command we are waiting for */
extern int	adbStarting;	/* doing ADB reinit, so do "polling" differently */

#define	ADB_MAX_MSG_LENGTH	16
#define	ADB_MAX_HDR_LENGTH	8
struct adbCommand {
	u_char	header[ADB_MAX_HDR_LENGTH];	/* not used yet */
	u_char	data[ADB_MAX_MSG_LENGTH];	/* packet data only */
	u_char	*saveBuf;	/* where to save result */
	adbComp	*compRout;	/* completion routine pointer */
	volatile int	*compData;	/* completion routine data pointer */
	u_int	cmd;		/* the original command for this data */
	u_int	unsol;		/* 1 if packet was unsolicited */
	u_int	ack_only;	/* 1 for no special processing */
};
extern	void	adb_pass_up __P((struct adbCommand *));

#if 0
/*
 * Define the external functions
 */
extern int	zshard __P((int));		/* from zs.c */
#endif

#ifdef ADB_DEBUG
/*
 * This function dumps contents of the PMData
 */
void
pm_printerr(ttl, rval, num, data)
	const char *ttl;
	int rval;
	int num;
	const char *data;
{
	int i;

	printf("pm: %s:%04x %02x ", ttl, rval, num);
	for (i = 0; i < num; i++)
		printf("%02x ", data[i]);
	printf("\n");
}
#endif



/*
 * Check the hardware type of the Power Manager
 */
void
pm_setup_adb()
{
}

/*
 * Search for targ in list.  list is an area of listlen bytes
 * containing null-terminated strings.
 */
static int
strinlist(const char *targ, char *list, int listlen)
{
	char	*str;
	int	sl;
	int	targlen;

	str = list;
	targlen = strlen(targ);
	while (listlen > 0) {
		sl = strlen(str);
		if (sl == targlen && (strncmp(targ, str, sl) == 0))
			return 1;
		str += sl+1;
		listlen -= sl+1;
	}
	return 0;
}

/*
 * Check the hardware type of the Power Manager
 */
void
pm_init(void)
{
	uint32_t	regs[10];
	PMData		pmdata;
	char		compat[128];
	int		clen, node, pm_imask;

	node = OF_peer(0);
	if (node == -1) {
		printf("pmu: Failed to get root");
		return;
	}
	clen = OF_getprop(node, "compatible", compat, sizeof(compat));
	if (clen <= 0) {
		printf("pmu: failed to read root compatible data %d\n", clen);
		return;
	}

	pm_imask =
	    PMU_INT_PCEJECT | PMU_INT_SNDBRT | PMU_INT_ADB | PMU_INT_TICK;

	if (strinlist("AAPL,3500", compat, clen) ||
	    strinlist("AAPL,3400/2400", compat, clen)) {
		/* How to distinguish BATT_COMET? */
		pmu_nbatt = 1;
		pmu_batt_type = BATT_HOOPER;
		pmu_type = PMU_OHARE;
	} else if (strinlist("AAPL,PowerBook1998", compat, clen) ||
		   strinlist("PowerBook1,1", compat, clen)) {
		pmu_nbatt = 2;
		pmu_batt_type = BATT_SMART;
		pmu_type = PMU_G3;
	} else {
		pmu_nbatt = 1;
		pmu_batt_type = BATT_SMART;
		pmu_type = PMU_KEYLARGO;
		node = getnodebyname(0, "power-mgt");
		if (node == -1) {
			printf("pmu: can't find power-mgt\n");
			return;
		}
		clen = OF_getprop(node, "prim-info", regs, sizeof(regs));
		if (clen < 24) {
			printf("pmu: failed to read prim-info\n");
			return;
		}
		pmu_nbatt = regs[6] >> 16;
	}

	pmdata.command = PMU_SET_IMASK;
	pmdata.num_data = 1;
	pmdata.s_buf = pmdata.data;
	pmdata.r_buf = pmdata.data;
	pmdata.data[0] = pm_imask;	
	pmgrop(&pmdata);
}


/*
 * Check the existent ADB devices
 */
void
pm_check_adb_devices(id)
	int id;
{
	u_short ed = 0x1;

	ed <<= id;
	pm_existent_ADB_devices |= ed;
}


/*
 * Wait until PM IC is busy
 */
int
pm_wait_busy(delaycycles)
	int delaycycles;
{
	while (PM_IS_ON) {
#ifdef PM_GRAB_SI
#if 0
		zshard(0);		/* grab any serial interrupts */
#else
		(void)intr_dispatch(0x70);
#endif
#endif
		if ((--delaycycles) < 0)
			return 1;	/* timeout */
	}
	return 0;
}


/*
 * Wait until PM IC is free
 */
int
pm_wait_free(delaycycles)
	int delaycycles;
{
	while (PM_IS_OFF) {
#ifdef PM_GRAB_SI
#if 0
		zshard(0);		/* grab any serial interrupts */
#else
		(void)intr_dispatch(0x70);
#endif
#endif
		if ((--delaycycles) < 0)
			return 0;	/* timeout */
	}
	return 1;
}



/*
 * Receive data from PMU
 */
static int
pm_receive(data)
	u_char *data;
{
	int i;
	int rval;

	rval = 0xffffcd34;

	switch (1) {
	default:
		/* set VIA SR to input mode */
		via_reg_or(VIA1, vACR, 0x0c);
		via_reg_and(VIA1, vACR, ~0x10);
		i = PM_SR();

		PM_SET_STATE_ACKOFF();
		if (pm_wait_busy((int)ADBDelay*32) != 0)
			break;		/* timeout */

		PM_SET_STATE_ACKON();
		rval = 0xffffcd33;
		if (pm_wait_free((int)ADBDelay*32) == 0)
			break;		/* timeout */

		*data = PM_SR();
		rval = 0;

		break;
	}

	PM_SET_STATE_ACKON();
	via_reg_or(VIA1, vACR, 0x1c);

	return rval;
}	



/*
 * Send data to PMU
 */
static int
pm_send(data)
	u_char data;
{
	int rval;

	via_reg_or(VIA1, vACR, 0x1c);
	write_via_reg(VIA1, vSR, data);	/* PM_SR() = data; */

	PM_SET_STATE_ACKOFF();
	rval = 0xffffcd36;
	if (pm_wait_busy((int)ADBDelay*32) != 0) {
		PM_SET_STATE_ACKON();

		via_reg_or(VIA1, vACR, 0x1c);

		return rval;		
	}

	PM_SET_STATE_ACKON();
	rval = 0xffffcd35;
	if (pm_wait_free((int)ADBDelay*32) != 0)
		rval = 0;

	PM_SET_STATE_ACKON();
	via_reg_or(VIA1, vACR, 0x1c);

	return rval;
}



/*
 * The PMgrOp routine
 */
int
pmgrop(pmdata)
	PMData *pmdata;
{
	int i;
	int s;
	u_char via1_vIER;
	int rval = 0;
	int num_pm_data = 0;
	u_char pm_cmd;	
	short pm_num_rx_data;
	u_char pm_data;
	u_char *pm_buf;

	s = splhigh();

	/* disable all inetrrupts but PM */
	via1_vIER = 0x10;
	via1_vIER &= read_via_reg(VIA1, vIER);
	write_via_reg(VIA1, vIER, via1_vIER);
	if (via1_vIER != 0x0)
		via1_vIER |= 0x80;

	switch (pmdata->command) {
	default:
		/* wait until PM is free */
		pm_cmd = (u_char)(pmdata->command & 0xff);
		rval = 0xcd38;
		if (pm_wait_free(ADBDelay * 4) == 0)
			break;			/* timeout */

		/* send PM command */
		if ((rval = pm_send((u_char)(pm_cmd & 0xff))))
			break;				/* timeout */

		/* send number of PM data */
		num_pm_data = pmdata->num_data;
		if (pm_send_cmd_type[pm_cmd] < 0) {
			if ((rval = pm_send((u_char)(num_pm_data & 0xff))) != 0)
				break;		/* timeout */
			pmdata->command = 0;
		}
		/* send PM data */
		pm_buf = (u_char *)pmdata->s_buf;
		for (i = 0 ; i < num_pm_data; i++)
			if ((rval = pm_send(pm_buf[i])) != 0)
				break;			/* timeout */
		if (i != num_pm_data)
			break;				/* timeout */


		/* check if PM will send me data  */
		pm_num_rx_data = pm_receive_cmd_type[pm_cmd];
		pmdata->num_data = pm_num_rx_data;
		if (pm_num_rx_data == 0) {
			rval = 0;
			break;				/* no return data */
		}

		/* receive PM command */
		pm_data = pmdata->command;
		pm_num_rx_data--;
		if (pm_num_rx_data == 0)
			if ((rval = pm_receive(&pm_data)) != 0) {
				rval = 0xffffcd37;
				break;
			}
		pmdata->command = pm_data;

		/* receive number of PM data */
		if (pm_num_rx_data < 0) {
			if ((rval = pm_receive(&pm_data)) != 0)
				break;		/* timeout */
			num_pm_data = pm_data;
		} else
			num_pm_data = pm_num_rx_data;
		pmdata->num_data = num_pm_data;

		/* receive PM data */
		pm_buf = (u_char *)pmdata->r_buf;
		for (i = 0; i < num_pm_data; i++) {
			if ((rval = pm_receive(&pm_data)) != 0)
				break;			/* timeout */
			pm_buf[i] = pm_data;
		}

		rval = 0;
	}

	/* restore former value */
	write_via_reg(VIA1, vIER, via1_vIER);
	splx(s);

	return rval;
}


/*
 * My PMU interrupt routine
 */
int
pm_intr(void *arg)
{
	int s;
	int rval;
	PMData pmdata;

	s = splhigh();

	PM_VIA_CLR_INTR();			/* clear VIA1 interrupt */
						/* ask PM what happend */
	pmdata.command = PMU_INT_ACK;
	pmdata.num_data = 0;
	pmdata.s_buf = &pmdata.data[2];
	pmdata.r_buf = &pmdata.data[2];
	rval = pmgrop(&pmdata);
	if (rval != 0) {
#ifdef ADB_DEBUG
		if (adb_debug)
			printf("pm: PM is not ready. error code: %08x\n", rval);
#endif
		splx(s);
		return 0;
	}

	switch ((u_int)(pmdata.data[2] & 0xff)) {
	case 0x00:		/* no event pending? */
		break;
	case 0x80:		/* 1 sec interrupt? */
		pm_counter++;
		break;
	case 0x08:		/* Brightness/Contrast button on LCD panel */
		/* get brightness and contrast of the LCD */
		pm_LCD_brightness = (u_int)pmdata.data[3] & 0xff;
		pm_LCD_contrast = (u_int)pmdata.data[4] & 0xff;

		/* this is experimental code */
		pmdata.command = PMU_SET_BRIGHTNESS;
		pmdata.num_data = 1;
		pmdata.s_buf = pmdata.data;
		pmdata.r_buf = pmdata.data;
		pm_LCD_brightness = 0x7f - pm_LCD_brightness / 2;
		if (pm_LCD_brightness < 0x08)
			pm_LCD_brightness = 0x08;
		if (pm_LCD_brightness > 0x78)
			pm_LCD_brightness = 0x78;
		pmdata.data[0] = pm_LCD_brightness;
		rval = pmgrop(&pmdata);
		break;

	case 0x10:		/* ADB data requested by TALK command */
	case 0x14:
		pm_adb_get_TALK_result(&pmdata);
		break;
	case 0x16:		/* ADB device event */
	case 0x18:
	case 0x1e:
		pm_adb_get_ADB_data(&pmdata);
		break;
	default:
#ifdef ADB_DEBUG
		if (adb_debug)
			pm_printerr("driver does not support this event.",
			    pmdata.data[2], pmdata.num_data,
			    pmdata.data);
#endif
		break;
	}

	splx(s);

	return 1;
}


/*
 * Synchronous ADBOp routine for the Power Manager
 */
int
pm_adb_op(buffer, compRout, data, command)
	u_char *buffer;
	adbComp *compRout;
	volatile int *data;
	int command;
{
	int i;
	int s;
	int rval;
	int timo;
	PMData pmdata;
	struct adbCommand packet;

	if (adbWaiting == 1)
		return 1;

	s = splhigh();
	write_via_reg(VIA1, vIER, 0x10);

 	adbBuffer = buffer;
	adbCompRout = compRout;
	adbCompData = data;

	pmdata.command = PMU_ADB_CMD;
	pmdata.s_buf = pmdata.data;
	pmdata.r_buf = pmdata.data;

	/* if the command is LISTEN, add number of ADB data to number of PM data */
	if ((command & 0xc) == 0x8) {
		if (buffer != (u_char *)0)
			pmdata.num_data = buffer[0] + 3;
	} else {
		pmdata.num_data = 3;
	}

	pmdata.data[0] = (u_char)(command & 0xff);
	pmdata.data[1] = 0;
	if ((command & 0xc) == 0x8) {		/* if the command is LISTEN, copy ADB data to PM buffer */
		if ((buffer != (u_char *)0) && (buffer[0] <= 24)) {
			pmdata.data[2] = buffer[0];		/* number of data */
			for (i = 0; i < buffer[0]; i++)
				pmdata.data[3 + i] = buffer[1 + i];
		} else
			pmdata.data[2] = 0;
	} else
		pmdata.data[2] = 0;

	if ((command & 0xc) != 0xc) {		/* if the command is not TALK */
		/* set up stuff for adb_pass_up */
		packet.data[0] = 1 + pmdata.data[2];
		packet.data[1] = command;
		for (i = 0; i < pmdata.data[2]; i++)
			packet.data[i+2] = pmdata.data[i+3];
		packet.saveBuf = adbBuffer;
		packet.compRout = adbCompRout;
		packet.compData = adbCompData;
		packet.cmd = command;
		packet.unsol = 0;
		packet.ack_only = 1;
		adb_polling = 1;
		adb_pass_up(&packet);
		adb_polling = 0;
	}

	rval = pmgrop(&pmdata);
	if (rval != 0) {
		splx(s);
		return 1;
	}

	delay(10000);

	adbWaiting = 1;
	adbWaitingCmd = command;

	PM_VIA_INTR_ENABLE();

	/* wait until the PM interrupt has occurred */
	timo = 0x80000;
	while (adbWaiting == 1) {
		if (read_via_reg(VIA1, vIFR) & 0x14)
			pm_intr(NULL);
#ifdef PM_GRAB_SI
#if 0
			zshard(0);		/* grab any serial interrupts */
#else
			(void)intr_dispatch(0x70);
#endif
#endif
		if ((--timo) < 0) {
			/* Try to take an interrupt anyway, just in case.
			 * This has been observed to happen on my ibook
			 * when i press a key after boot and before adb
			 * is attached;  For example, when booting with -d.
			 */
			pm_intr(NULL);
			if (adbWaiting) {
				printf("pm_adb_op: timeout. command = 0x%x\n",command);
				splx(s);
				return 1;
			}
#ifdef ADB_DEBUG
			else {
				printf("pm_adb_op: missed interrupt. cmd=0x%x\n",command);
			}
#endif
		}
	}

	/* this command enables the interrupt by operating ADB devices */
	pmdata.command = PMU_ADB_CMD;
	pmdata.num_data = 4;
	pmdata.s_buf = pmdata.data;
	pmdata.r_buf = pmdata.data;
	pmdata.data[0] = 0x00;	
	pmdata.data[1] = 0x86;	/* magic spell for awaking the PM */
	pmdata.data[2] = 0x00;	
	pmdata.data[3] = 0x0c;	/* each bit may express the existent ADB device */
	rval = pmgrop(&pmdata);

	splx(s);
	return rval;
}


void
pm_adb_get_TALK_result(pmdata)
	PMData *pmdata;
{
	int i;
	struct adbCommand packet;

	/* set up data for adb_pass_up */
	packet.data[0] = pmdata->num_data-1;
	packet.data[1] = pmdata->data[3];
	for (i = 0; i <packet.data[0]-1; i++)
		packet.data[i+2] = pmdata->data[i+4];

	packet.saveBuf = adbBuffer;
	packet.compRout = adbCompRout;
	packet.compData = adbCompData;
	packet.unsol = 0;
	packet.ack_only = 0;
	adb_polling = 1;
	adb_pass_up(&packet);
	adb_polling = 0;

	adbWaiting = 0;
	adbBuffer = (long)0;
	adbCompRout = (long)0;
	adbCompData = (long)0;
}


void
pm_adb_get_ADB_data(pmdata)
	PMData *pmdata;
{
	int i;
	struct adbCommand packet;

	if (pmu_type == PMU_OHARE && pmdata->num_data == 4 &&
	    pmdata->data[1] == 0x2c && pmdata->data[3] == 0xff &&
	    ((pmdata->data[2] & ~1) == 0xf4)) {
		if (pmdata->data[2] == 0xf4) {
			pm_eject_pcmcia(0);
		} else {
			pm_eject_pcmcia(1);
		}
		return;
	}
	/* set up data for adb_pass_up */
	packet.data[0] = pmdata->num_data-1;	/* number of raw data */
	packet.data[1] = pmdata->data[3];	/* ADB command */
	for (i = 0; i <packet.data[0]-1; i++)
		packet.data[i+2] = pmdata->data[i+4];
	packet.unsol = 1;
	packet.ack_only = 0;
	adb_pass_up(&packet);
}


void
pm_adb_restart()
{
	PMData p;

	p.command = PMU_RESET_CPU;
	p.num_data = 0;
	p.s_buf = p.data;
	p.r_buf = p.data;
	pmgrop(&p);
}

void
pm_adb_poweroff()
{
	PMData p;

	p.command = PMU_POWER_OFF;
	p.num_data = 4;
	p.s_buf = p.data;
	p.r_buf = p.data;
	strcpy(p.data, "MATT");
	pmgrop(&p);
}

void
pm_read_date_time(t)
	u_long *t;
{
	PMData p;

	p.command = PMU_READ_RTC;
	p.num_data = 0;
	p.s_buf = p.data;
	p.r_buf = p.data;
	pmgrop(&p);

	memcpy(t, p.data, 4);
}

void
pm_set_date_time(t)
	u_long t;
{
	PMData p;

	p.command = PMU_SET_RTC;
	p.num_data = 4;
	p.s_buf = p.r_buf = p.data;
	memcpy(p.data, &t, 4);
	pmgrop(&p);
}

int
pm_read_brightness()
{
	PMData p;

	p.command = PMU_READ_BRIGHTNESS;
	p.num_data = 1;		/* XXX why 1? */
	p.s_buf = p.r_buf = p.data;
	p.data[0] = 0;
	pmgrop(&p);

	return p.data[0];
}

void
pm_set_brightness(val)
	int val;
{
	PMData p;

	val = 0x7f - val / 2;
	if (val < 0x08)
		val = 0x08;
	if (val > 0x78)
		val = 0x78;

	p.command = PMU_SET_BRIGHTNESS;
	p.num_data = 1;
	p.s_buf = p.r_buf = p.data;
	p.data[0] = val;
	pmgrop(&p);
}

void
pm_init_brightness()
{
	int val;

	val = pm_read_brightness();
	pm_set_brightness(val);
}

void
pm_eject_pcmcia(slot)
	int slot;
{
	PMData p;

	if (slot != 0 && slot != 1)
		return;

	p.command = PMU_EJECT_PCMCIA;
	p.num_data = 1;
	p.s_buf = p.r_buf = p.data;
	p.data[0] = 5 + slot;	/* XXX */
	pmgrop(&p);
}

/*
 * Thanks to Paul Mackerras and Fabio Riccardi's Linux implementation
 * for a clear description of the PMU results.
 */
static int
pm_battery_info_smart(int battery, struct pmu_battery_info *info)
{
	PMData p;

	p.command = PMU_SMART_BATTERY_STATE;
	p.num_data = 1;
	p.s_buf = p.r_buf = p.data;
	p.data[0] = battery + 1;
	pmgrop(&p);

	info->flags = p.data[1];

	info->secs_remaining = 0;
	switch (p.data[0]) {
	case 3:
	case 4:
		info->cur_charge = p.data[2];
		info->max_charge = p.data[3];
		info->draw = *((signed char *)&p.data[4]);
		info->voltage = p.data[5];
		break;
	case 5:
		info->cur_charge = ((p.data[2] << 8) | (p.data[3]));
		info->max_charge = ((p.data[4] << 8) | (p.data[5]));
		info->draw = *((signed short *)&p.data[6]);
		info->voltage = ((p.data[8] << 8) | (p.data[7]));
		break;
	default:
		/* XXX - Error condition */
		info->cur_charge = 0;
		info->max_charge = 0;
		info->draw = 0;
		info->voltage = 0;
		break;
	}
	if (info->draw) {
		if (info->flags & PMU_PWR_AC_PRESENT && info->draw > 0) {
			info->secs_remaining =
				((info->max_charge - info->cur_charge) * 3600)
				/ info->draw;
		} else {
			info->secs_remaining =
				(info->cur_charge * 3600) / -info->draw;
		}
	}

	return 1;
}

static int
pm_battery_info_legacy(int battery, struct pmu_battery_info *info, int ty)
{
	PMData p;
	long pcharge=0, charge, vb, vmax, chargemax;
	long vmax_charging, vmax_charged, amperage, voltage;

	p.command = PMU_BATTERY_STATE;
	p.num_data = 0;
	p.s_buf = p.r_buf = p.data;
	pmgrop(&p);

	info->flags = p.data[0];

	if (info->flags & PMU_PWR_BATT_PRESENT) {
		if (ty == BATT_COMET) {
			vmax_charging = 213;
			vmax_charged = 189;
			chargemax = 6500;
		} else {
			/* Experimental values */
			vmax_charging = 365;
			vmax_charged = 365;
			chargemax = 6500;
		}
		vmax = vmax_charged;
		vb = (p.data[1] << 8) | p.data[2];
		voltage = (vb * 256 + 72665) / 10;
		amperage = (unsigned char) p.data[5];
		if ((info->flags & PMU_PWR_AC_PRESENT) == 0) {
			if (amperage > 200)
				vb += ((amperage - 200) * 15)/100;
		} else if (info->flags & PMU_PWR_BATT_CHARGING) {
			vb = (vb * 97) / 100;
			vmax = vmax_charging;
		}
		charge = (100 * vb) / vmax;
		if (info->flags & PMU_PWR_PCHARGE_RESET) {
			pcharge = (p.data[6] << 8) | p.data[7];
			if (pcharge > chargemax)
				pcharge = chargemax;
			pcharge *= 100;
			pcharge = 100 - pcharge / chargemax;
			if (pcharge < charge)
				charge = pcharge;
		}
		info->cur_charge = charge;
		info->max_charge = 100;
		info->draw = -amperage;
		info->voltage = voltage;
		if (amperage > 0)
			info->secs_remaining = (charge * 16440) / amperage;
		else
			info->secs_remaining = 0;
	} else {
		info->cur_charge = 0;
		info->max_charge = 0;
		info->draw = 0;
		info->voltage = 0;
		info->secs_remaining = 0;
	}

	return 1;
}

int
pm_battery_info(int battery, struct pmu_battery_info *info)
{

	if (battery > pmu_nbatt)
		return 0;

	switch (pmu_batt_type) {
	case BATT_COMET:
	case BATT_HOOPER:
		return pm_battery_info_legacy(battery, info, pmu_batt_type);

	case BATT_SMART:
		return pm_battery_info_smart(battery, info);
	}

	return 0;
}

int
pm_read_nvram(addr)
	int addr;
{
	PMData p;

	p.command = PMU_READ_NVRAM;
	p.num_data = 2;
	p.s_buf = p.r_buf = p.data;
	p.data[0] = addr >> 8;
	p.data[1] = addr;
	pmgrop(&p);

	return p.data[0];
}

void
pm_write_nvram(addr, val)
	int addr, val;
{
	PMData p;

	p.command = PMU_WRITE_NVRAM;
	p.num_data = 3;
	p.s_buf = p.r_buf = p.data;
	p.data[0] = addr >> 8;
	p.data[1] = addr;
	p.data[2] = val;
	pmgrop(&p);
}
