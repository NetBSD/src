/*	$NetBSD: adbsys.c,v 1.40 1999/02/11 06:41:08 ender Exp $	*/

/*-
 * Copyright (C) 1994	Bradley A. Grantham
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
 *	This product includes software developed by Bradley A. Grantham.
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

#include "opt_adb.h"

#include <sys/param.h>
#include <sys/systm.h>

#include <machine/cpu.h>
#include <machine/viareg.h>

#include <mac68k/mac68k/macrom.h>
#include <mac68k/dev/adbvar.h>

extern	struct mac68k_machine_S mac68k_machine;

/* from adb.c */
void    adb_processevent __P((adb_event_t * event));

extern void adb_jadbproc __P((void));

void 
adb_complete(buffer, data_area, adb_command)
	caddr_t buffer;
	caddr_t data_area;
	int adb_command;
{
	adb_event_t event;
	ADBDataBlock adbdata;
	int adbaddr;
	int error;
#ifdef ADB_DEBUG
	int i;

	if (adb_debug)
		printf("adb: transaction completion\n");
#endif

	adbaddr = (adb_command & 0xf0) >> 4;
	error = GetADBInfo(&adbdata, adbaddr);
#ifdef ADB_DEBUG
	if (adb_debug)
		printf("adb: GetADBInfo returned %d\n", error);
#endif

	event.addr = adbaddr;
	event.hand_id = (int)(adbdata.devType);
	event.def_addr = (int)(adbdata.origADBAddr);
	event.byte_count = buffer[0];
	memcpy(event.bytes, buffer + 1, event.byte_count);

#ifdef ADB_DEBUG
	if (adb_debug) {
		printf("adb: from %d at %d (org %d) %d:", event.addr,
		    event.hand_id, event.def_addr, buffer[0]);
		for (i = 1; i <= buffer[0]; i++)
			printf(" %x", buffer[i]);
		printf("\n");
	}
#endif

	microtime(&event.timestamp);

	adb_processevent(&event);
}

void 
adb_msa3_complete(buffer, data_area, adb_command)
	caddr_t buffer;
	caddr_t data_area;
	int adb_command;
{
	adb_event_t event;
	ADBDataBlock adbdata;
	int adbaddr;
	int error;
#ifdef ADB_DEBUG
	int i;

	if (adb_debug)
		printf("adb: transaction completion\n");
#endif

	adbaddr = (adb_command & 0xf0) >> 4;
	error = GetADBInfo(&adbdata, adbaddr);
#ifdef ADB_DEBUG
	if (adb_debug)
		printf("adb: GetADBInfo returned %d\n", error);
#endif

	event.addr = adbaddr;
	event.hand_id = ADBMS_MSA3;
	event.def_addr = (int)(adbdata.origADBAddr);
	event.byte_count = buffer[0];
	memcpy(event.bytes, buffer + 1, event.byte_count);

#ifdef ADB_DEBUG
	if (adb_debug) {
		printf("adb: from %d at %d (org %d) %d:",
		    event.addr, event.hand_id, event.def_addr, buffer[0]);
		for (i = 1; i <= buffer[0]; i++)
			printf(" %x", buffer[i]);
		printf("\n");
	}
#endif

	microtime(&event.timestamp);

	adb_processevent(&event);
}

void 
adb_mm_nonemp_complete(buffer, data_area, adb_command)
	caddr_t buffer;
	caddr_t data_area;
	int adb_command;
{
	adb_event_t event;
	ADBDataBlock adbdata;
	int adbaddr;
	int error;

#ifdef ADB_DEBUG
	int i;

	if (adb_debug)
		printf("adb: transaction completion\n");
#endif

	adbaddr = (adb_command & 0xf0) >> 4;
	error = GetADBInfo(&adbdata, adbaddr);
#ifdef ADB_DEBUG
	if (adb_debug)
		printf("adb: GetADBInfo returned %d\n", error);
#endif

#if 0
	printf("adb: from %d at %d (org %d) %d:", event.addr,
		event.hand_id, event.def_addr, buffer[0]);
	for (i = 1; i <= buffer[0]; i++)
		printf(" %x", buffer[i]);
	printf("\n");
#endif

	/* massage the data to look like EMP data */
	if ((buffer[3] & 0x04) == 0x04)
		buffer[1] &= 0x7f;
	else
		buffer[1] |= 0x80;
	if ((buffer[3] & 0x02) == 0x02)
		buffer[2] &= 0x7f;
	else
		buffer[2] |= 0x80;
	if ((buffer[3] & 0x01) == 0x01)
		buffer[3] = 0x00;
	else
		buffer[3] = 0x80;

	event.addr = adbaddr;
	event.hand_id = (int)(adbdata.devType);
	event.def_addr = (int)(adbdata.origADBAddr);
	event.byte_count = buffer[0];
	memcpy(event.bytes, buffer + 1, event.byte_count);

#ifdef ADB_DEBUG
	if (adb_debug) {
		printf("adb: from %d at %d (org %d) %d:", event.addr,
		    event.hand_id, event.def_addr, buffer[0]);
		for (i = 1; i <= buffer[0]; i++)
			printf(" %x", buffer[i]);
		printf("\n");
	}
#endif

	microtime(&event.timestamp);

	adb_processevent(&event);
}

static volatile int extdms_done;

/*
 * initialize extended mouse - probes devices as
 * described in _Inside Macintosh, Devices_.
 */
void
extdms_init(totaladbs)
	int totaladbs;
{
	ADBDataBlock adbdata;
	int adbindex, adbaddr, count;
	short cmd;
	u_char buffer[9];

	for (adbindex = 1; adbindex <= totaladbs; adbindex++) {
		/* Get the ADB information */
		adbaddr = GetIndADB(&adbdata, adbindex);
		if (adbdata.origADBAddr == ADBADDR_MS &&
		    (adbdata.devType == ADBMS_USPEED ||
		     adbdata.devType == ADBMS_UCONTOUR)) {
			/* Found MicroSpeed Mouse Deluxe Mac or Contour Mouse */
			cmd = ((adbaddr<<4)&0xF0)|0x9;	/* listen 1 */

			/*
			 * To setup the MicroSpeed or the Contour, it appears
			 * that we can send the following command to the mouse
			 * and then expect data back in the form:
			 *  buffer[0] = 4 (bytes)
			 *  buffer[1], buffer[2] as std. mouse
			 *  buffer[3] = buffer[4] = 0xff when no buttons
			 *   are down.  When button N down, bit N is clear.
			 * buffer[4]'s locking mask enables a
			 * click to toggle the button down state--sort of
			 * like the "Easy Access" shift/control/etc. keys.
			 * buffer[3]'s alternative speed mask enables using 
			 * different speed when the corr. button is down
			 */
			buffer[0] = 4;
			buffer[1] = 0x00;	/* Alternative speed */
			buffer[2] = 0x00;	/* speed = maximum */
			buffer[3] = 0x10;	/* enable extended protocol,
						 * lower bits = alt. speed mask
						 *            = 0000b
						 */
			buffer[4] = 0x07;	/* Locking mask = 0000b,
						 * enable buttons = 0111b
						 */
			extdms_done = 0;
			ADBOp((Ptr)buffer, (Ptr)extdms_complete,
				(Ptr)&extdms_done, cmd);
			while (!extdms_done)
				/* busy wait until done */;
		}
		if (adbdata.origADBAddr == ADBADDR_MS &&
		    (adbdata.devType == ADBMS_100DPI ||
		    adbdata.devType == ADBMS_200DPI)) {
			/* found a mouse */
			cmd = ((adbaddr << 4) & 0xf0) | 0x3;

			extdms_done = 0;
			cmd = (cmd & 0xf3) | 0x0c; /* talk command */
			ADBOp((Ptr)buffer, (Ptr)extdms_complete,
			      (Ptr)&extdms_done, cmd);

			/* Wait until done, but no more than 2 secs */
			count = 40000;
			while (!extdms_done && count-- > 0)
				delay(50);

			if (!extdms_done) {
#ifdef ADB_DEBUG
				if (adb_debug)
					printf("adb: extdms_init timed out\n");
#endif
				return;
			}

			/* Attempt to initialize Extended Mouse Protocol */
			buffer[2] = '\004'; /* make handler ID 4 */
			extdms_done = 0;
			cmd = (cmd & 0xf3) | 0x08; /* listen command */
			ADBOp((Ptr)buffer, (Ptr)extdms_complete,
			      (Ptr)&extdms_done, cmd);
			while (!extdms_done)
				/* busy wait until done */;

			/*
			 * Check to see if successful, if not
			 * try to initialize it as other types
			 */
			cmd = ((adbaddr << 4) & 0xf0) | 0x3;
			extdms_done = 0;
			cmd = (cmd & 0xf3) | 0x0c; /* talk command */
			ADBOp((Ptr)buffer, (Ptr)extdms_complete,
			      (Ptr)&extdms_done, cmd);
			while (!extdms_done)
				/* busy wait until done */;
				
			if (buffer[2] != ADBMS_EXTENDED) {
				/* Attempt to initialize as an A3 mouse */
				buffer[2] = 0x03; /* make handler ID 3 */
				extdms_done = 0;
				cmd = (cmd & 0xf3) | 0x08; /* listen command */
				ADBOp((Ptr)buffer, (Ptr)extdms_complete,
				      (Ptr)&extdms_done, cmd);
				while (!extdms_done)
					/* busy wait until done */;
		
				/*
				 * Check to see if successful, if not
				 * try to initialize it as other types
				 */
				cmd = ((adbaddr << 4) & 0xf0) | 0x3;
				extdms_done = 0;
				cmd = (cmd & 0xf3) | 0x0c; /* talk command */
				ADBOp((Ptr)buffer, (Ptr)extdms_complete,
				      (Ptr)&extdms_done, cmd);
				while (!extdms_done)
					/* busy wait until done */;
					
				if (buffer[2] == ADBMS_MSA3) {
					/* Initialize as above */
					cmd = ((adbaddr << 4) & 0xF0) | 0xA;
					/* listen 2 */
					buffer[0] = 3;
					buffer[1] = 0x00;
					/* Irrelevant, buffer has 0x77 */
					buffer[2] = 0x07;
					/*
					 * enable 3 button mode = 0111b,
					 * speed = normal
					 */
					extdms_done = 0;
					ADBOp((Ptr)buffer, (Ptr)extdms_complete,
					      (Ptr)&extdms_done, cmd);
					while (!extdms_done)
						/* busy wait until done */;
				} else {
					/* No special support for this mouse */
				}
			}
		}
	}
}

void 
adb_init()
{
	ADBDataBlock adbdata;
	ADBSetInfoBlock adbinfo;
	int totaladbs;
	int adbindex, adbaddr;
	int error, cmd, count, devtype = 0;
	u_char buffer[9];
	extern int adb_initted;

#ifdef MRG_ADB
	/*
	 * Even if serial console only, some models require the
	 * ADB in order to get the date/time and do soft power.
	 */
	if ((mac68k_machine.serial_console & 0x03)) {
		printf("adb: using serial console\n");
		return;
	}

	if (!mrg_romready()) {
		printf("adb: no ROM ADB driver in this kernel for this machine\n");
		return;
	}

	printf("adb: bus subsystem\n");
#ifdef ADB_DEBUG
	if (adb_debug)
		printf("adb: call mrg_initadbintr\n");
#endif

	mrg_initadbintr();	/* Mac ROM Glue okay to do ROM intr */
#ifdef ADB_DEBUG
	if (adb_debug)
		printf("adb: returned from mrg_initadbintr\n");
#endif

	/* ADBReInit pre/post-processing */
	JADBProc = adb_jadbproc;

	/* Initialize ADB */
#ifdef ADB_DEBUG
	if (adb_debug)
		printf("adb: calling ADBAlternateInit.\n");
#endif

	ADBAlternateInit();
#else
	ADBReInit();
#endif /* MRG_ADB */

#ifdef ADB_DEBUG
	if (adb_debug)
		printf("adb: done with ADBReInit\n");
#endif

	totaladbs = CountADBs();
	extdms_init(totaladbs);

	/* for each ADB device */
	for (adbindex = 1; adbindex <= totaladbs; adbindex++) {
		/* Get the ADB information */
		adbaddr = GetIndADB(&adbdata, adbindex);

		/* Print out the glory */
		printf("adb: ");
		switch (adbdata.origADBAddr) {
		case ADBADDR_SECURE:
			printf("security dongle (%d)", (int)(adbdata.devType));
			break;
		case ADBADDR_MAP:
			switch (adbdata.devType) {
			case ADB_STDKBD:
				printf("standard keyboard");
				break;
			case ADB_ISOKBD:
				printf("standard keyboard (ISO layout)");
				break;
			case ADB_EXTKBD:
				extdms_done = 0;
				/* talk R1 */
				cmd = (((adbaddr << 4) & 0xf0) | 0x0d );
				ADBOp((Ptr)buffer, (Ptr)extdms_complete,
				    (Ptr)&extdms_done, cmd);

				/* Wait until done, but no more than 2 secs */
				count = 40000;
				while (!extdms_done && count-- > 0)
					delay(50);

				if (extdms_done &&
				    buffer[1] == 0x9a && buffer[2] == 0x20)
					printf("Mouseman (non-EMP) pseudo keyboard");
				else if (extdms_done &&
				    buffer[1] == 0x9a && buffer[2] == 0x21)
					printf("Trackman (non-EMP) pseudo keyboard");
				else
					printf("extended keyboard");
				break;
			case ADB_EXTISOKBD:
				printf("extended keyboard (ISO layout)");
				break;
			case ADB_KBDII:
				printf("keyboard II");
				break;
			case ADB_ISOKBDII:
				printf("keyboard II (ISO layout)");
				break;
			case ADB_PBKBD:
				printf("PowerBook keyboard");
				break;
			case ADB_PBISOKBD:
				printf("PowerBook keyboard (ISO layout)");
				break;
			case ADB_ADJKPD:
				printf("adjustable keypad");
				break;
			case ADB_ADJKBD:
				printf("adjustable keyboard");
				break;
			case ADB_ADJISOKBD:
				printf("adjustable keyboard (ISO layout)");
				break;
			case ADB_ADJJAPKBD:
				printf("adjustable keyboard (Japanese layout)");
				break;
			case ADB_PBEXTISOKBD:
				printf("PowerBook extended keyboard (ISO layout)");
				break;
			case ADB_PBEXTJAPKBD:
				printf("PowerBook extended keyboard (Japanese layout)");
				break;
			case ADB_PBEXTKBD:
				printf("PowerBook extended keyboard");
				break;
			case ADB_DESIGNKBD:
				printf("extended keyboard");
				break;
			default:
				printf("mapped device (%d)", (int)(adbdata.devType));
				break;
			}
			break;
		case ADBADDR_REL:
			extdms_done = 0;
			/* talk register 3 */
			ADBOp((Ptr)buffer, (Ptr)extdms_complete,
			    (Ptr)&extdms_done, (adbaddr << 4) | 0xf);

			/* Wait until done, but no more than 2 secs */
			count = 40000;
			while (!extdms_done && count-- > 0)
				delay(50);

			if (!extdms_done) {
				printf("ghost mouse?");
				break;
			}

			devtype = buffer[2];
			switch (devtype) {
			case ADBMS_100DPI:
				printf("100 dpi mouse");
				break;
			case ADBMS_200DPI:
				printf("200 dpi mouse");
				break;
			case ADBMS_MSA3:
				printf("Mouse Systems A3 mouse, default parameters");
				break;
			case ADBMS_USPEED:
				printf("MicroSpeed mouse, default parameters");
				break;
			case ADBMS_UCONTOUR:
				printf("Contour mouse, default parameters");
				break;
			case ADBMS_EXTENDED:
				extdms_done = 0;
				/* talk register 1 */
				ADBOp((Ptr)buffer, (Ptr)extdms_complete,
				    (Ptr)&extdms_done, (adbaddr << 4) | 0xd);
				while (!extdms_done)
					/* busy-wait until done */;
				if (buffer[1] == 0x9a && buffer[2] == 0x20)
					printf("Mouseman (non-EMP) mouse");
				else if (buffer[1] == 0x9a && buffer[2] == 0x21)
					printf("Trackman (non-EMP) trackball");
				else {
					printf("extended mouse <%c%c%c%c> "
					    "%d-button %d dpi ",
					    buffer[1], buffer[2],
					    buffer[3], buffer[4],
					    (int)buffer[8],
					    (int)*(short *)&buffer[5]);
					if (buffer[7] == 1)
						printf("mouse");
					else if (buffer[7] == 2)
						printf("trackball");
					else
						printf("unknown device");
				}
				break;
			default:
				printf("relative positioning device (mouse?) "
				    "(%d)", (int)(adbdata.devType));
				break;
			}
			break;
		case ADBADDR_ABS:
			switch (adbdata.devType) {
			case ADB_ARTPAD:
				printf("WACOM ArtPad II");
				break;
			default:
				printf("abs. pos. device (tablet?) (%d)",
				    (int)(adbdata.devType));
				break;
			}
			break;
		case ADBADDR_DATATX:
			printf("data transfer device (modem?) (%d)",
			    (int)(adbdata.devType));
			break;
		case ADBADDR_MISC:
			switch (adbdata.devType) {
			case ADB_POWERKEY:
				printf("Sophisticated Circuits PowerKey");
				break;
			default:
				printf("misc. device (remote control?) (%d)",
				    (int)(adbdata.devType));
				break;
			}
			break;
		default:
			printf("unknown type device, (def %d, handler %d)",
			    (int)(adbdata.origADBAddr), (int)(adbdata.devType));
			break;
		}
		printf(" at %d\n", adbaddr);

		/* Set completion routine to be NetBSD's */
		if ((adbdata.origADBAddr == ADBADDR_REL) && 
		    (buffer[0] > 0) && (buffer[2] == ADBMS_MSA3)) {
			/* Special device handler for the A3 mouse */
			adbinfo.siServiceRtPtr = (Ptr)adb_msa3_asmcomplete;
		} else if ((adbdata.origADBAddr == ADBADDR_MAP) &&
		    (adbdata.devType == ADB_EXTKBD) &&
		    (buffer[1] == 0x9a) &&
		    ((buffer[2] == 0x20) || (buffer[2] == 0x21))) {
			/* ignore non-EMP Mouseman/Trackman pseudo keyboard */
			adbinfo.siServiceRtPtr = (Ptr)0;
		} else if ((adbdata.origADBAddr == ADBADDR_REL) &&
		    (devtype == ADBMS_EXTENDED) &&
		    (buffer[1] == 0x9a) &&
		    ((buffer[2] == 0x20) || (buffer[2] == 0x21))) {
			/*
			 * Set up non-EMP Mouseman/Trackman to put button
			 * bits in 3rd byte instead of sending via pseudo
			 * keyboard device.
			 */
			extdms_done = 0;
			/* listen register 1 */
			buffer[0] = 2;
			buffer[1] = 0x00;
			buffer[2] = 0x81;
			ADBOp((Ptr)buffer, (Ptr)extdms_complete,
			    (Ptr)&extdms_done, (adbaddr << 4) | 0x9);
			while (!extdms_done)
				/* busy-wait until done */;
			extdms_done = 0;
			/* listen register 1 */
			buffer[0] = 2;
			buffer[1] = 0x01;
			buffer[2] = 0x81;
			ADBOp((Ptr)buffer, (Ptr)extdms_complete,
			    (Ptr)&extdms_done, (adbaddr << 4) | 0x9);
			while (!extdms_done)
				/* busy-wait until done */;
			extdms_done = 0;
			/* listen register 1 */
			buffer[0] = 2;
			buffer[1] = 0x02;
			buffer[2] = 0x81;
			ADBOp((Ptr)buffer, (Ptr)extdms_complete,
			    (Ptr)&extdms_done, (adbaddr << 4) | 0x9);
			while (!extdms_done)
				/* busy-wait until done */;
			extdms_done = 0;
			/* listen register 1 */
			buffer[0] = 2;
			buffer[1] = 0x03;
			buffer[2] = 0x38;
			ADBOp((Ptr)buffer, (Ptr)extdms_complete,
			    (Ptr)&extdms_done, (adbaddr << 4) | 0x9);
			while (!extdms_done)
				/* busy-wait until done */;
			/* non-EMP Mouseman has special handler */
			adbinfo.siServiceRtPtr = (Ptr)adb_mm_nonemp_asmcomplete;
		} else {
			/* Default completion routine */
			adbinfo.siServiceRtPtr = (Ptr)adb_asmcomplete;
		}
		adbinfo.siDataAreaAddr = NULL;
		error = SetADBInfo(&adbinfo, adbaddr);
#ifdef ADB_DEBUG
		if (adb_debug)
			printf("adb: returned %d from SetADBInfo\n", error);
#endif
	}

	adb_initted = 1;
}
