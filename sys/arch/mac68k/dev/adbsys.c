/*-
 * Copyright (C) 1993, 1994	Allen K. Briggs, Chris P. Caputo,
 *			Michael L. Finch, Bradley A. Grantham, and
 *			Lawrence A. Kesteloot
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
 *	This product includes software developed by the Alice Group.
 * 4. The names of the Alice Group or any of its members may not be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE ALICE GROUP ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE ALICE GROUP BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $Id: adbsys.c,v 1.5 1994/09/12 03:54:05 briggs Exp $
 *
 */

/*
 * $Log: adbsys.c,v $
 * Revision 1.5  1994/09/12 03:54:05  briggs
 * Remove need for "bounds.h."  Brad can still use it if he wants ;-)
 *
 * Revision 1.4  1994/07/30  04:21:42  lkestel
 * Moved adbsys.h grfioctl.h and keyboard.h to include to make desktop
 * and X compile more cleanly.
 *
 * Revision 1.3  1994/07/21  06:36:51  lkestel
 * Fixed a few bugs in the key-repeat function and disabled key-repeat
 * when /dev/adb is closed to avoid infinite repeat problem.  Brad claims
 * that he's got this solved in his version...
 *
 * 
 * 12/22/93 ~10PM I wrote a new ADB system.
 */

/* I can't believe how much SOFTWARE these guys expected to write */
/* just to get the damn events from the keyboard and mouse!!! */
/* Why couldn't the data just arrive at my doorstep gift-wrapped? */


#include <sys/types.h>
#include <sys/param.h>
#include <machine/param.h>
#include <sys/device.h>
#include <sys/fcntl.h>
#include <sys/select.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include "../mac68k/via.h"
#include <machine/adbsys.h>
/* #include "bounds.h" */
#define CHECKBOUNDS(arg1, arg2)

extern unsigned char keyboard[128][3];

#define spladb splhigh


		/* Man, am I paranoid. */
#define ADB_DEBUG	1


		/* Fix adb_panic()s */
#if ADB_DEBUG
#define ADB_PANIC(a)	adb_panic(a)
#else /* not ADB_DEBUG */
#define ADB_PANIC(a)
#endif /* ADB_DEBUG */


	/* ADB subsystem possible states */
#define ADB_SYS_IDLE		0x500
#define ADB_SYS_RESTART		0x100
#define ADB_SYS_BUSY		0x200
#define ADB_SYS_LOOKING		0x300
#define ADB_SYS_NOTREADY	0x400
#define ADB_SYS_CHECKACTIVE	0x600
#define ADB_SYS_PROBING		0x700
#define ADB_SYS_MOVING		0x800


	/* state bits for shoving down VIA */
#define ADB_ST_MASK 0x30
#define ADB_ST_IDLE 0x30
#define ADB_ST_EVEN 0x10
#define ADB_ST_ODD 0x20
#define ADB_ST_CMD 0x00


	/* fields of ADB command byte */
#define ADB_ADDR_MASK 0xf0
#define ADB_CMD_MASK 0x0C
#define ADB_REG_MASK 0x03


	/* possible ADB commands */
#define ADB_CMD_TALK 0x0C
#define ADB_CMD_LISTEN 0x08
#define ADB_CMD_FLUSH 0x01
#define ADB_CMD_RESET 0x00


	/* macros for making/extracting ADB command data. */
#define ADB_CMD2ADDR(cmd)	(((cmd) & ADB_ADDR_MASK) >> 4)
#define ADB_CMD2CMD(cmd)	((cmd) & ADB_CMD_MASK)
#define ADB_CMD2REG(cmd)	((cmd) & ADB_REG_MASK)
	/* note that this macro does no bounds checking. */
#define ADB_MKCMD(addr, cmd, reg) (((addr) << 4) | (cmd) | (reg))


	/* ADB device table */
struct adb_dev_s adb_devs[ADB_MAX_DEVS];


	/* ADB subsystem state variables */
enum adb_system_e adb_system_type = MacIIADB;
int adb_bus_state = ADB_ST_IDLE;	/* What I think ADB bus state is */
int adb_sys_state = ADB_SYS_NOTREADY;	/* ADB subsystem state */
int adb_cur_cmd;			/* current command *bits* */
int adb_cur_reg;			/* current transaction register */
int adb_lastactive = -1;		/* last active dev (should we skip?) */
static struct adb_event_s cur_event;	/* current event package */
static unsigned char *cur_buf = cur_event.bytes;
					/* byte data buffer */
int adb_bytes_sent = 0;			/* bytes sent so far */
static int curaddr;			/* current probing address */
static int avail;			/* last available address */
static int lastmoved;			/* where last dev was moved */


#if !defined(ADB_MAX_EVENTS)
#define ADB_MAX_EVENTS 200	/* maybe should be higher for slower macs? */
#endif /* !defined(ADB_MAX_EVENTS) */

static adb_event_t adb_evq[ADB_MAX_EVENTS];	/* ADB event queue */
static int adb_evq_tail = 0;			/* event queue tail */
static int adb_evq_len = 0;			/* event queue length */


static int adb_isopen = 0;	/* Are we queuing events for adb_read? */
int adb_polling = 0;		/* Are we polling?  (Debugger mode) */
static struct selinfo adb_selinfo;	/* select() info */
static struct proc *adb_ioproc = NULL;	/* process to wakeup */


static int adb_rptdelay = 20;		/* ticks before auto-repeat */
static int adb_rptinterval = 6;		/* ticks between auto-repeat */
static int adb_repeating = -1;		/* key that is auto-repeating */
static adb_event_t adb_rptevent;	/* event to auto-repeat */


void adb_init_II();
void adb_init_PB();
void adb_init_SI();

long adb_intr_II(void);
long adb_intr_PB(void);
long adb_intr_SI(void);

void adb_start_xaction_II(int cmd, int sys_state);
void adb_start_xaction_PB(int cmd, int sys_state);
void adb_start_xaction_SI(int cmd, int sys_state);


extern long (*via1itab[7])();


/* External Entry Points ------------------------- */


void adb_init()
{
	/* Interrupt was already set up by machdep.c:set_machdep() */

	switch(adb_system_type){
		case MacIIADB:
			adb_init_II();
			break;

		case MacIIsiADB:
			adb_init_SI();
			break;

		case MacPBADB:
			adb_init_PB();
			break;
	}
}


long adb_intr()
{
	switch(adb_system_type){
		case MacIIADB:
			return(adb_intr_II());
			break;

		case MacIIsiADB:
			return(adb_intr_SI());
			break;

		case MacPBADB:
			return(adb_intr_PB());
			break;
	}
	return(0);
}


	/* Begin a transaction */
void adb_start_xaction(
	int cmd,
	int sys)
{
	switch(adb_system_type){
		case MacIIADB:
			adb_start_xaction_II(cmd, sys);
			break;

		case MacIIsiADB:
			adb_start_xaction_SI(cmd, sys);
			break;

		case MacPBADB:
			adb_start_xaction_PB(cmd, sys);
			break;
	}
}


	/* Send a device these bytes */
void adb_send(
	int dev,
	int reg,
	int bytecount,
	unsigned char *bytes)
{
	int i;

	while(adb_sys_state != ADB_SYS_IDLE);
	for(i = 0; i < bytecount; i++)
		cur_buf[i] = bytes[i];
	cur_event.byte_count = bytecount;
	adb_start_xaction(ADB_MKCMD(dev, ADB_CMD_LISTEN, reg), ADB_SYS_BUSY);
}


int adb_where[2000], adb_index = 0;
#if ADB_DEBUG	/* then define debug stuff */

#define W(w)	((adb_index < 2000) ? (adb_where[adb_index++] = (w)) : 0)
#define W_backup()	(adb_index--)

#else /* ! ADB_DEBUG */

#define W(w)
#define W_backup()

#endif /* ADB_DEBUG */


void adb_dump()
{
	register int where;

	printf("ADB sequence strings: ");
	for(where = 0; where < adb_index; where++){
		switch(adb_where[where]){
			case 0:
				printf("Started transaction, device %d, cmd %d, reg %d.\n",
					ADB_CMD2ADDR(adb_where[where + 1]), ADB_CMD2CMD(adb_where[where + 1]),
					ADB_CMD2REG(adb_where[where + 1]));
				where++;
				break;

			case 0xdd:
				printf("Continuing talk, device %d, cmd %d, reg %d.\n",
					ADB_CMD2ADDR(adb_where[where + 1]),
					ADB_CMD2CMD(adb_where[where + 1]),
					ADB_CMD2REG(adb_where[where + 1]));
				where++;
				break;

			case 0xde:
				/* interrupt with no data. */
				printf("Interrupt with no service request.\n");
				break;

			case 0xff:
				printf("Made bus IDLE.\n");
				break;

			case 0xaa:
				printf("Xaction complete, ");
				switch(adb_where[where + 1]){
					case 0x91:
						printf("bus idle, calling continue_talk.\n");
						where++;
						break;

					case 0x12:
						printf("sys busy.\n");
						where++;
						break;

					case 0x23:
						printf("SYS_RESTART case.\n");
						where++;
						break;

					case 0x34:
						printf("SYS_LOOKING case.");
						if(adb_where[where + 2] == 0xed){
							printf(" (last possible device had no data!)");
							where++;
						}
						where++;
						printf("\n");
						break;

					case 0x45:
						printf("SYS_CHECKACTIVE case.\n");
						where++;
						break;
				}
				break;

			case 0xbb:
				printf("Interrupt: state|int = %02x, ", adb_where[where + 1]);
				where ++;
				switch(adb_where[where + 1]){
					case 0xf1:
						printf("ADBSYS not initialized.\n");
						where++;
						break;
	
					case 0x12:
						printf("ST_IDLE.\n");
						where++;
						break;
	
					case 0x23:
						printf("ST_CMD.\n");
						where++;
						break;
	
					case 0x34:
						printf("ST_EVEN, data = %d.\n", adb_where[where + 2]);
						where += 2;
						break;
	
					case 0x45:
						printf("ST_ODD, data = %d.\n", adb_where[where + 2]);
						where += 2;
						break;
	
					default:
						printf("unknown! 0x%x\n", adb_where[where + 1]);
						break;
				}
		}
	}
}


void adb_panic(char *str)
{
	register int where;

	splhigh();

	/* panic(str); */
	printf("%s\nADB Panicked\n", str);

#if ADB_DEBUG

	adb_dump();

	printf("waiting forever...\n", str);

	while(1);

#else /* not ADB_DEBUG */

	panic("ADB failed horribly");
	
#endif /* ADB_DEBUG */
}


/*----------------------------------------------------------------------------
	PowerBook ADB subsystem functions
 */


long adb_intr_PB(void){return(1);}
void adb_init_PB(void){};
void adb_start_xaction_PB(int cmd, int sys_state){};
void adb_finish_xaction_PB(){};


/*----------------------------------------------------------------------------
	Macintosh IIsi series ADB subsystem functions
 */


long adb_intr_SI(void){return(adb_intr_II());}		/* BARF */
void adb_init_SI(void){adb_init_II();}			/* BARF */
void adb_start_xaction_SI(int cmd, int sys_state){};
void adb_finish_xaction_SI(){};


/*----------------------------------------------------------------------------
	Macintosh II series ADB subsystem functions
 */


int adb_finish_xaction_II();
void adb_start_xaction_II(int cmd, int sys_state);
void adb_make_idle_II();


void adb_start_xaction_II(
	int cmd,
	int sys_state)
{
	register int s;
	struct timeval nowtime;

	s = spladb();
	W(0);
			/* make IDLE to kill any existing transactions */
	via_reg(VIA1, vDirB) = ADB_ST_MASK;
	via_reg(VIA1, vBufB) = ADB_ST_IDLE;
	cur_event.addr = ADB_CMD2ADDR(cmd);
	microtime(&cur_event.timestamp);
	adb_cur_cmd = ADB_CMD2CMD(cmd);
	adb_cur_reg = ADB_CMD2REG(cmd);
	W(cmd);
	if(adb_cur_cmd == ADB_CMD_TALK)
		cur_event.byte_count = 0;
	adb_sys_state = sys_state;
	via_SR_output(VIA1);
	via_reg(VIA1, vSR) = cmd;
	adb_bus_state = via_reg(VIA1, vBufB) = ADB_ST_CMD;
	splx(s);
}


void adb_continue_talk_II(
	unsigned char command)
{
	if(((via_reg(VIA1, vBufB) & DB1I_vFDBInt) == 0) &&
		(ADB_CMD2CMD(command) == ADB_CMD_TALK))
	{
		W(0xdd);
		W(command);
		adb_lastactive = cur_event.addr = ADB_CMD2ADDR(command);
		adb_cur_cmd = ADB_CMD2CMD(command);
		adb_cur_reg = ADB_CMD2REG(command);
		cur_event.byte_count = 0;
		microtime(&cur_event.timestamp);
		adb_sys_state = ADB_SYS_CHECKACTIVE;
		via_SR_input(VIA1);
		via_reg(VIA1, vDirB) = ADB_ST_MASK;
		adb_bus_state = via_reg(VIA1, vBufB) = ADB_ST_EVEN;
	}else{
		W(0xde);
		adb_make_idle_II();
	}
}


void adb_make_idle_II()
{
	W(0xff);
	via_reg(VIA1, vDirB) = ADB_ST_MASK;
	adb_bus_state = via_reg(VIA1, vBufB) = ADB_ST_IDLE;
	adb_sys_state = ADB_SYS_IDLE;
	via_SR_input(VIA1);
	adb_lastactive = -1;
}


void adb_init_II()
{
	int hey;
	static int already = 0;
	int are_devs;

	if(already != 0){
		printf("adb_init(): called to init *again!*\n");
	}
	already = 1;

#if ADB_DEBUG
	printf("Desktop bus initializing...\n");
#endif

	for(hey = 0; hey < ADB_MAX_DEVS; hey++) /* 0 & 1 reserved, however */
		adb_devs[hey].handler_id = -1;	/* there is no device there. */

#if 1	/* This doesn't work on the Quadra 700?? */
	/* Not really necessary, anyway. */
	/* begin peeking! */
	adb_start_xaction_II(ADB_MKCMD(2, ADB_CMD_TALK, 3), ADB_SYS_RESTART);

	hey = 0;
	while((adb_sys_state == ADB_SYS_RESTART) && (hey < 6000000))
		hey++;
	if (hey >= 6000000) printf("adb_init(): timed out!!!!!\n");
#endif

#if 1	/* only kept around for old DESKTOP code */
	for(hey = 2; hey < ADB_MAX_DEVS; hey++)
		adb_devs[hey].handler_id == -1;

	adb_devs[2].handler_id = 2;
	adb_devs[2].default_addr = 2;
	adb_devs[3].handler_id = 1;
	adb_devs[3].default_addr = 3;
#endif

	adb_make_idle_II();

	are_devs = 0;
#if 0	/* only kept around for old desktop code */
	for(hey = 2; hey < ADB_MAX_DEVS; hey++){
		if(adb_devs[hey].handler_id != -1){
			printf("adb: ");
			switch(adb_devs[hey].default_addr){
				case 2:
					switch(adb_devs[hey].handler_id){
						case 1:
							printf("ADB keyboard");
							break;
						case 2:
							printf("ADB extended keyboard");
							break;
						default:
							printf("mapped device (%d)", adb_devs[hey].handler_id);
							break;
					}
					break;
				case 3:
					switch(adb_devs[hey].handler_id){
						case 1:
							printf("100 dpi mouse");
							break;
						case 2:
							printf("200 dpi mouse");
							break;
						default:
printf("relative positioning device (mouse?) (%d)", adb_devs[hey].handler_id);
							break;
					}
					break;
				case 4:
printf("absolute positioning device (tablet?) (%d)", adb_devs[hey].handler_id);
					break;
				default:
					printf("unknown type device, (def %d, handler %d)", adb_devs[hey].default_addr, adb_devs[hey].handler_id);
					break;
			}
			printf(" at %d\n", hey);
			are_devs = 1;
		}
	}

	if(!are_devs)
		printf("adb: no devices on bus?\n");
#endif
}


void adb_enqevent(
	adb_event_t *event)
{
	int s;

	if(adb_evq_tail < 0 || adb_evq_tail >= ADB_MAX_EVENTS)
		panic("adb: event queue tail is out of bounds");

	if(adb_evq_len < 0 || adb_evq_len > ADB_MAX_EVENTS)
		panic("adb: event queue len is out of bounds");

	s = splhigh();

	if(adb_evq_len == ADB_MAX_EVENTS){
		splx(s);
		return;	/* Oh, well... */
	}

	adb_evq[(adb_evq_len + adb_evq_tail) % ADB_MAX_EVENTS] =
		*event;
	adb_evq_len++;

	selwakeup(&adb_selinfo);
	if(adb_ioproc)
		psignal(adb_ioproc, SIGIO);

	splx(s);
}

void adb_handoff(
	adb_event_t *event)
{
	if(adb_isopen && !adb_polling){
		adb_enqevent(event);
	}else{
#if 1
#	if 0

		printf("adb:l %d %d %d %d %d %d\n", event->addr, event->hand_id,
			event->def_addr, event->byte_count, event->bytes[0],
			event->bytes[1]);
#	endif
		if(event->def_addr == 2)
			ite_intr(event);

#else /* ADB_EVENTS */

		desktop_intr(event);

#endif /* ADB_EVENTS */
	}
}

void adb_autorepeat(
	void *key)
{
	int s;

	if (!adb_isopen) {
		return;
	}
	adb_rptevent.bytes[0] |= 0x80;
	microtime(&adb_rptevent.timestamp);
	adb_handoff(&adb_rptevent);	/* do key up */

	adb_rptevent.bytes[0] &= 0x7f;
	microtime(&adb_rptevent.timestamp);
	adb_handoff(&adb_rptevent);	/* do key down */

	s = splhigh();
	/*
	 * XXX LK: I don't like this "if" -- if false, then shouldn't
	 * hit key down above either.
	 */
	if(adb_repeating != -1){
		timeout(adb_autorepeat, (caddr_t)key,
			adb_rptinterval);
	}
	splx(s);
}

void adb_mungesend(
	adb_event_t *event)
{
	int adb_key, s;

	if(event->def_addr == 2 && adb_isopen){
		adb_key = event->u.k.key;
		if (keyboard[adb_key & 0x7f][0] != 0){
			if((adb_key & 0x80) == 0){
				/* ignore shift & control */
				if(adb_repeating != -1){
					s = splhigh();
					adb_repeating = -1;
					untimeout(adb_autorepeat,
						(caddr_t)adb_rptevent.u.k.key);
					splx(s);
				}
				adb_rptevent = *event;
				timeout(adb_autorepeat,
					(caddr_t)adb_key, adb_rptdelay);
				adb_repeating = adb_key;
			}else{
				if(adb_repeating != -1){
					adb_key = adb_repeating;
					adb_repeating = -1;
					untimeout(adb_autorepeat,
						(caddr_t)adb_key);
				}
				adb_rptevent = *event;
			}
		}
	}
	adb_handoff(event);
}


void adb_processevent(
	adb_event_t *event)
{
	int key;
	adb_event_t new_event;

	new_event = *event;

	switch(event->def_addr){
		case ADBADDR_KBD:
			new_event.u.k.key = event->bytes[0];
			new_event.bytes[1] = 0xff;
			adb_mungesend(&new_event);
			if(event->bytes[1] != 0xff){
				new_event.u.k.key = event->bytes[1];
				new_event.bytes[0] = event->bytes[1];
				new_event.bytes[1] = 0xff;
				adb_mungesend(&new_event);
			}
			break;

		case ADBADDR_MS:
			new_event.u.m.buttons = ((event->bytes[0] &
						0x80) >> 7) ^ 0x1;
			new_event.u.m.dx = ((signed int)(event->bytes[1] &
						0x3f)) - ((event->bytes[1] &
						0x40) ?  64 : 0);
			new_event.u.m.dy = ((signed int)(event->bytes[0] &
						0x3f)) - ((event->bytes[0] &
						0x40) ?  64 : 0);
			adb_mungesend(&new_event);
			break;

		default:		/* God only knows. */
			adb_mungesend(event);
	}

}


int adb_finish_xaction_II()
{
	register int cur_dev;
	register int address, handler_id;
	register int i;

	W(0xaa);
	switch(adb_sys_state){
		case ADB_SYS_IDLE:
			W(0x91);
			/* whoa. */
			ADB_PANIC("adb: timed out when system was IDLE?\n");
			break;

		case ADB_SYS_BUSY:
			W(0x12); /* well, this transaction is over */
			/* Tell the nice gentleman that his transaction */
			/* timed out */
			adb_make_idle_II();
			break;

		case ADB_SYS_RESTART:
			W(0x23); /* well, this guy timed out. */
			/* were we waiting for a command or a byte? */
#if 1
			if((adb_bus_state == ADB_ST_EVEN ||
				adb_bus_state == ADB_ST_ODD) &&
				(cur_event.byte_count > 0)){

				/* OOoo!  Must have completed talk reg 3! */
				if(cur_event.byte_count != 2){
					ADB_PANIC("adb: someone sent <> 2 bytes from reg 3!\n");
				}

				if(cur_buf[0] != 0xff){
					address = cur_event.addr;
					handler_id = cur_buf[1];
					CHECKBOUNDS(adb_devs, address);
					if(address > 4)	/* fake 3 butt mouse */
						adb_devs[address].default_addr
							= 2;
					else
						adb_devs[address].default_addr
							= address;
					adb_devs[address].handler_id = handler_id;
				}
			}

			cur_event.addr++;

			if(cur_event.addr == 5)
				cur_event.addr = 8;

			/* Better check out the next one. */
			if(cur_event.addr < 16 ){
adb_start_xaction_II(ADB_MKCMD(cur_event. addr, ADB_CMD_TALK, 3),
	ADB_SYS_RESTART);
			}else{
				adb_make_idle_II();
			}
#else /* not 1 */
			curaddr = 2;
			avail = 15;
			lastmoved = -1;
			adb_start_xaction_II(ADB_MKCMD(curaddr, ADB_CMD_TALK,
				3), ADB_SYS_PROBING);
			
#endif
			break;

		case ADB_SYS_LOOKING:
			W(0x34);

			/* device timed out, but after sending data? */
			cur_dev = cur_event.addr; 

			if(cur_event.byte_count != 0){ /* device had data */
				adb_make_idle_II();
				cur_event.hand_id =
					adb_devs[cur_dev].handler_id;
				cur_event.def_addr =
					adb_devs[cur_dev].default_addr;
				adb_processevent(&cur_event);
				adb_make_idle_II();
			}else{
				cur_dev++;
#if !ADB_DEBUG || 1
				while((cur_dev < ADB_MAX_DEVS) &&
					(adb_devs[cur_dev].handler_id == -1))
					cur_dev++;
#endif /* ADB_DEBUG */
				if(cur_dev < /*ADB_MAX_DEVS*/5){
					adb_start_xaction_II(ADB_MKCMD(cur_dev, ADB_CMD_TALK, 0),
						ADB_SYS_LOOKING);
				}else{
					/* ADB_PANIC("adb: I got a timeout on the last device"
						" that could have had data.\n"); */
					W(0xed);
					adb_make_idle_II();
				}
			}
			break;

		case ADB_SYS_CHECKACTIVE:
			W(0x45);
			cur_dev = cur_event.addr;
			if(cur_event.byte_count != 0){ /* device had data */
				adb_make_idle_II();
				cur_event.hand_id =
					adb_devs[cur_dev].handler_id;
				cur_event.def_addr =
					adb_devs[cur_dev].default_addr;
				adb_processevent(&cur_event);
				adb_make_idle_II();
			}else{
				adb_start_xaction_II(ADB_MKCMD(2, ADB_CMD_TALK, 0),
					ADB_SYS_LOOKING);
			}
			break;
		case ADB_SYS_PROBING:
			if(cur_event.byte_count == 2){
				/* cur_event.byte_count = 2; */
				cur_buf[1] = (cur_buf[0] & 0xf0)
					| avail;
				cur_buf[0] = adb_devs[avail].handler_id;
				lastmoved = avail;
				avail--;
				adb_start_xaction_II(ADB_MKCMD(curaddr,
					ADB_CMD_LISTEN, 3), ADB_SYS_MOVING);
			}else{
				if(lastmoved != -1){
					cur_buf[0] = adb_devs[avail].
						handler_id;
					cur_buf[1] = 0x20 | curaddr;
					avail++;
					curaddr++;
					adb_start_xaction_II(ADB_MKCMD(lastmoved,
						ADB_CMD_LISTEN, 3),
						ADB_SYS_MOVING);
					lastmoved = -1;
				}else{
					curaddr++;
					if(curaddr > 4)
						adb_make_idle_II();
					else
adb_start_xaction_II(ADB_MKCMD(curaddr, ADB_CMD_TALK, 3), ADB_SYS_PROBING);
				}
			}
			break;

		case ADB_SYS_MOVING:
			adb_devs[cur_buf[1] & 0xf].handler_id =
				adb_devs[cur_event.addr].handler_id;
			adb_devs[cur_buf[1] & 0xf].default_addr =
				adb_devs[cur_event.addr].default_addr;
			if(curaddr > 4)
				adb_make_idle_II();
			else
				adb_start_xaction_II(ADB_MKCMD(curaddr,
					ADB_CMD_TALK, 3), ADB_SYS_PROBING);
			
			break;
	}
}


/* Macintosh II, IIx, IIcx, SE/30, IIci ADB interrupt */
long adb_intr_II(void)
{
	unsigned char bytedata;
	int s;
	register int cur_dev;
	register int address, handler_id;
	struct timeval now_time, old_time;
	register int diff;
	int timedout;

	s = spladb();
	bytedata = via_reg(VIA1, vSR);	/* have to read to clear interrupt */
		/* see fig. 12 and 13 in SY6522 docs */
	W(0xbb); 
	W(via_reg(VIA1, vBufB));

	if(adb_sys_state == ADB_SYS_NOTREADY){
		W_backup(); 	/* No point in wasting paper. */
		W_backup(); 	/* No point in wasting paper. */
		/* W(0xf1); */
		splx(s);
		return(1);
	}

	switch(adb_bus_state){
		case ADB_ST_IDLE:
			W(0x12);
			if(adb_sys_state != ADB_SYS_IDLE){
				ADB_PANIC("adb: somehow, we got a not IDLE IDLE.\n");
				adb_make_idle_II();	/* We're trying to get work done here! */
			}else{
				adb_continue_talk_II(bytedata);
			}
			break;

		case ADB_ST_CMD:
			W(0x23);
			if((adb_cur_cmd == ADB_CMD_TALK) ||
				(adb_cur_cmd == ADB_CMD_LISTEN)){
				if(adb_cur_cmd == ADB_CMD_TALK){ /* Allow data to shift in */
					via_SR_input(VIA1);
					via_reg(VIA1, vDirB) = ADB_ST_MASK;
					adb_bus_state = via_reg(VIA1, vBufB) = ADB_ST_EVEN;
				}else{
					via_reg(VIA1, vDirB) = ADB_ST_MASK;
					via_reg(VIA1, vSR) = cur_buf[0];
					adb_bytes_sent = 1;
					adb_bus_state = via_reg(VIA1, vBufB) = ADB_ST_EVEN;
				}
			}else{
				adb_make_idle_II();
				/* if a reset was sent, we should initiate a RESTART */
				if(adb_cur_cmd == ADB_CMD_RESET){
					adb_start_xaction_II(ADB_MKCMD(2, ADB_CMD_TALK, 3),
						ADB_SYS_RESTART);
				}
			}
			break;

		case ADB_ST_EVEN:
			W(0x34);
			W(bytedata);
			if(adb_cur_cmd == ADB_CMD_TALK){
				if(((via_reg(VIA1, vBufB) & DB1I_vFDBInt) == 0) ||
					(cur_event.byte_count == 8)){
					/* There is no data for this byte. */
					adb_finish_xaction_II();
				}else{
					if(cur_event.byte_count < sizeof(cur_buf)){
						CHECKBOUNDS(cur_buf, cur_event.byte_count);
						cur_buf[cur_event.byte_count++] = bytedata;
						via_reg(VIA1, vDirB) = ADB_ST_MASK;
						adb_bus_state = via_reg(VIA1, vBufB) = ADB_ST_ODD;
					}else{
#if ADB_DEBUG
						adb_panic("adb: too many incoming bytes??\n");
#endif /* ADB_DEBUG */
						adb_finish_xaction_II();
					}
				}
			}else if(adb_cur_cmd == ADB_CMD_LISTEN){
				if(adb_bytes_sent >= cur_event.byte_count)
					adb_finish_xaction_II();
				via_reg(VIA1, vSR) = cur_buf[adb_bytes_sent++];
				via_reg(VIA1, vDirB) = ADB_ST_MASK;
				adb_bus_state = via_reg(VIA1, vBufB) =
					ADB_ST_ODD;
			}else{
				/* What to do if LISTEN or anything else? */
				adb_panic("adb: What am I doing"
					" (adb_intr, BUS_EVEN)\n");
			}
			break;

		case ADB_ST_ODD:
			W(0x45);
			W(bytedata);
			if(adb_cur_cmd == ADB_CMD_TALK){
				if(((via_reg(VIA1, vBufB) & DB1I_vFDBInt) == 0) ||
					(cur_event.byte_count == 8)){
					/* There is no data for this byte. */
					adb_finish_xaction_II();
				}else{
					if(cur_event.byte_count < sizeof(cur_buf)){
						CHECKBOUNDS(cur_buf, cur_event.byte_count);
						cur_buf[cur_event.byte_count++] = bytedata;
						via_reg(VIA1, vDirB) = ADB_ST_MASK;
						adb_bus_state = via_reg(VIA1, vBufB) = ADB_ST_EVEN;
					}else{
#if ADB_DEBUG
						adb_panic("adb: too many incoming bytes??\n");
#endif /* ADB_DEBUG */
						adb_finish_xaction_II();
					}
				}
			}else if(adb_cur_cmd == ADB_CMD_LISTEN){
				if(adb_bytes_sent >= cur_event.byte_count)
					adb_finish_xaction_II();
				via_reg(VIA1, vSR) =
					cur_buf[adb_bytes_sent++];
				via_reg(VIA1, vDirB) = ADB_ST_MASK;
				adb_bus_state = via_reg(VIA1, vBufB) =
					ADB_ST_EVEN;
			}else{
				/* What to do if LISTEN or anything else? */
				adb_panic("adb: What am I doing (adb_intr, BUS_ODD)\n");
			}
			break;

		default:
			adb_panic("adb: internal bus state defaulted in interrupt.\n");
	}

	splx(s);

	return(1);
}


static void
adbattach(parent, dev, aux)
	struct device	*parent, *dev;
	void		*aux;
{
	printf(" (adb event device)\n");
	/* adb_init();  -- called from autoconf... In case ddb needs it?  */
}


extern int matchbyname();

struct cfdriver adbcd =
      { NULL,
	"adb",
	matchbyname,
	adbattach,
	DV_DULL,
	sizeof(struct device),
	NULL,
	0 };


int adbopen(
	dev_t dev,
	int flag,
	int mode,
	struct proc *p)
{
	register int unit;
	int error = 0;
	int s;
 
	unit = minor(dev);
	if(unit != 0)
		return(ENXIO);
	
	s = splhigh();
	if (adb_isopen) {
		splx(s);
		return(EBUSY);
	}
	adb_evq_tail = 0;
	adb_evq_len = 0;
	adb_isopen = 1;
	adb_ioproc = p;
	splx(s);

	return (error);
}


int adbclose(
	dev_t dev,
	int flag,
	int mode,
	struct proc *p)
{
	register int unit;
	int s;
	int kbd;
 
	adb_isopen = 0;
	adb_ioproc = NULL;
	return (0);
}


int adbread(
	dev_t dev,
	struct uio *uio,
	int flag)
{
	int s, error;
	int willfit;
	int total;
	int firstmove;
	int moremove;

	if (uio->uio_resid < sizeof(adb_event_t))
		return (EMSGSIZE);	/* close enough. */

	s = splhigh();
	if(adb_evq_len == 0){
		splx(s);	
		return(0);
	}

	willfit = howmany(uio->uio_resid, sizeof(adb_event_t));
	total = (adb_evq_len < willfit) ? adb_evq_len : willfit;

	firstmove = (adb_evq_tail + total > ADB_MAX_EVENTS)
		? (ADB_MAX_EVENTS - adb_evq_tail) : total;
	
	if(error = uiomove((caddr_t)&adb_evq[adb_evq_tail],
		firstmove * sizeof(adb_event_t), uio))
	{
		splx(s);
		return(error);
	}
	moremove = total - firstmove;

	if (moremove > 0)
		if(error = uiomove((caddr_t)&adb_evq[0],
			moremove * sizeof(adb_event_t), uio))
		{
			splx(s);
			return(error);
		}

	adb_evq_tail = (adb_evq_tail + total) % ADB_MAX_EVENTS;
	adb_evq_len -= total;
	splx(s);
	return (0);
}

 
int adbwrite(
	dev_t dev,
	struct uio *uio,
	int flag)
{
	return 0;
}


int adbioctl(
	dev_t dev,
	int cmd,
	caddr_t data,
	int flag,
	struct proc *p)
{
	switch(cmd){
		case ADBIOC_GETDEVS: {
			adb_devinfo_t *di = (void *)data;
			int i;

			for(i = 0; i < ADB_MAX_DEVS; i++)
				di->dev[i] = adb_devs[i];
			break;
		}

		case ADBIOC_SETLIGHTS:{
			adb_setlights_t *sl = (void *)data;
			unsigned char lights[2];

			lights[1] = sl->lights;
			adb_send(sl->addr, 2, 2, lights);
			break;
		}

		case ADBIOC_GETREPEAT:{
			adb_rptinfo_t *ri = (void *)data;

			ri->delay_ticks = adb_rptdelay;
			ri->interval_ticks = adb_rptinterval;
			break;
		}

		case ADBIOC_SETREPEAT:{
			adb_rptinfo_t *ri = (void *)data;

			adb_rptdelay = ri->delay_ticks;
			adb_rptinterval = ri->interval_ticks;
			break;
		}

		default:
			return(EINVAL);
	}
	return(0);
}


int adbselect(
	dev_t dev,
	int rw,
	struct proc *p)
{
	switch (rw) {
		case FREAD:
			/* succeed if there is something to read */
			if (adb_evq_len > 0)
				return (1);
			selrecord(p, &adb_selinfo);
			break;

		case FWRITE:
			return (1);	/* always fails => never blocks */
			break;
	}

	return (0);
}
