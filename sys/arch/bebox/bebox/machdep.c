/*	$NetBSD: machdep.c,v 1.107.34.1 2018/07/28 04:37:29 pgoyette Exp $	*/

/*
 * Copyright (C) 1995, 1996 Wolfgang Solfrank.
 * Copyright (C) 1995, 1996 TooLs GmbH.
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
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.107.34.1 2018/07/28 04:37:29 pgoyette Exp $");

#include "opt_compat_netbsd.h"
#include "opt_ddb.h"

#define _POWERPC_BUS_DMA_PRIVATE

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/mount.h>
#include <sys/reboot.h>
#include <sys/systm.h>
#include <sys/vnode.h>

#include <uvm/uvm_extern.h>

#include <machine/bebox.h>
#include <machine/autoconf.h>
#include <machine/bootinfo.h>
#include <machine/powerpc.h>

#include <powerpc/pic/picvar.h> 
#include <powerpc/pio.h>
#include <powerpc/prep_bus.h>
#include <powerpc/psl.h>

#include <dev/cons.h>

#include "vga.h"
#if (NVGA > 0)
#include <dev/ic/mc6845reg.h>
#include <dev/ic/pcdisplayvar.h>
#include <dev/ic/vgareg.h>
#include <dev/ic/vgavar.h>
#endif

#include "pckbc.h"
#if (NPCKBC > 0)
#include <dev/isa/isareg.h>
#include <dev/ic/i8042reg.h>
#include <dev/ic/pckbcvar.h>
#include <dev/pckbport/pckbportvar.h>
#endif

#include "com.h"
#if (NCOM > 0)
#include <sys/termios.h>
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>
#endif

/*
 * Global variables used here and there
 */
char bootinfo[BOOTINFO_MAXSIZE];
#define	MEMREGIONS	2
struct mem_region physmemr[MEMREGIONS], availmemr[MEMREGIONS];
char bootpath[256];
struct pic_ops *isa_pic;
extern int primary_pic;
void initppc(u_long, u_long, u_int, void *);
static void disable_device(const char *);
void setup_bebox_intr(void);

extern void *startsym, *endsym;

void
initppc(u_long startkernel, u_long endkernel, u_int args, void *btinfo)
{
	/*
	 * copy bootinfo
	 */
	memcpy(bootinfo, btinfo, sizeof (bootinfo));

	/*
	 * BeBox memory region set
	 */
	{
		struct btinfo_memory *meminfo;

		meminfo =
		    (struct btinfo_memory *)lookup_bootinfo(BTINFO_MEMORY);
		if (!meminfo)
			panic("not found memory information in bootinfo");
		physmemr[0].start = 0;
		physmemr[0].size = meminfo->memsize & ~PGOFSET;
		availmemr[0].start = (endkernel + PGOFSET) & ~PGOFSET;
		availmemr[0].size = meminfo->memsize - availmemr[0].start;
	}

	/*
	 * Get CPU clock
	 */
	{
		struct btinfo_clock *clockinfo;
		extern u_long ticks_per_sec, ns_per_tick;

		clockinfo =
		    (struct btinfo_clock *)lookup_bootinfo(BTINFO_CLOCK);
		if (!clockinfo)
			panic("not found clock information in bootinfo");
		ticks_per_sec = clockinfo->ticks_per_sec;
		ns_per_tick = 1000000000 / ticks_per_sec;
	}

	prep_initppc(startkernel, endkernel, args);
}

/*
 * Machine dependent startup code.
 */
void
cpu_startup(void)
{

	/*
	 * Do common VM initialization
	 */
	oea_startup(NULL);

	pic_init();
	isa_pic = setup_i8259();
	setup_bebox_intr();
	primary_pic = 1;

	/*
	 * set up i8259 as a cascade on BeInterruptController irq 26.
	 */
	intr_establish(16 + 26, IST_LEVEL, IPL_HIGH, pic_handle_intr, isa_pic);

	oea_install_extint(pic_ext_intr);

	/*
	 * Now that we have VM, malloc's are OK in bus_space.
	 */
	bus_space_mallocok();
}

/*
 * lookup_bootinfo:
 * Look up information in bootinfo of boot loader.
 */
void *
lookup_bootinfo(int type)
{
	struct btinfo_common *bt;
	struct btinfo_common *help = (struct btinfo_common *)bootinfo;

	do {
		bt = help;
		if (bt->type == type)
			return (help);
		help = (struct btinfo_common *)((char*)help + bt->next);
	} while (bt->next &&
	    (size_t)help < (size_t)bootinfo + sizeof (bootinfo));

	return (NULL);
}

static void
disable_device(const char *name)
{
	extern struct cfdata cfdata[];
	int i;

	for (i = 0; cfdata[i].cf_name != NULL; i++)
		if (strcmp(cfdata[i].cf_name, name) == 0) {
			if (cfdata[i].cf_fstate == FSTATE_NOTFOUND)
				cfdata[i].cf_fstate = FSTATE_DNOTFOUND;
			else if (cfdata[i].cf_fstate == FSTATE_STAR)
				cfdata[i].cf_fstate = FSTATE_DSTAR;
		}
}

/*
 * consinit
 * Initialize system console.
 */
void
consinit(void)
{
	struct btinfo_console *consinfo;
	static int initted;

	if (initted)
		return;
	initted = 1;

	consinfo = (struct btinfo_console *)lookup_bootinfo(BTINFO_CONSOLE);
	if (!consinfo)
		panic("not found console information in bootinfo");

	/*
	 * We need to disable genfb or vga, because foo_match() return
	 * the same value.
	 */
	if (!strcmp(consinfo->devname, "be")) {
		/*
		 * We use Framebuffer for initialized by BootROM of BeBox.
		 * In this case, our console will be attached more late. 
		 */
#if (NPCKBC > 0)
		pckbc_cnattach(&genppc_isa_io_space_tag, IO_KBD, KBCMDP,
		    PCKBC_KBD_SLOT, 0);
#endif
		disable_device("vga");
		return;
	}

	disable_device("genfb");

#if (NVGA > 0)
	if (!strcmp(consinfo->devname, "vga")) {
		vga_cnattach(&prep_io_space_tag, &prep_mem_space_tag, -1, 1);
#if (NPCKBC > 0)
		pckbc_cnattach(&genppc_isa_io_space_tag, IO_KBD, KBCMDP,
		    PCKBC_KBD_SLOT, 0);
#endif
		return;
	}
#endif

#if (NCOM > 0)
	if (!strcmp(consinfo->devname, "com")) {
	   	bus_space_tag_t tag = &genppc_isa_io_space_tag;

		if(comcnattach(tag, consinfo->addr, consinfo->speed,
		    COM_FREQ, COM_TYPE_NORMAL,
		    ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8)))
			panic("can't init serial console");
		return;
	}
#endif
	panic("invalid console device %s", consinfo->devname);
}

/*
 * Halt or reboot the machine after syncing/dumping according to howto.
 */
void
cpu_reboot(int howto, char *what)
{
	static int syncing;
	static char str[256];
	char *ap = str, *ap1 = ap;

	boothowto = howto;
	if (!cold && !(howto & RB_NOSYNC) && !syncing) {
		syncing = 1;
		vfs_shutdown();		/* sync */
		resettodr();		/* set wall clock */
	}
	splhigh();
	if (howto & RB_HALT) {
		doshutdownhooks();
		pmf_system_shutdown(boothowto);
		printf("halted\n\n");

	}
	if (!cold && (howto & RB_DUMP))
		oea_dumpsys();
	doshutdownhooks();
	pmf_system_shutdown(boothowto);
	printf("rebooting\n\n");
	if (what && *what) {
		if (strlen(what) > sizeof str - 5)
			printf("boot string too large, ignored\n");
		else {
			strcpy(str, what);
			ap1 = ap = str + strlen(str);
			*ap++ = ' ';
		}
	}
	*ap++ = '-';
	if (howto & RB_SINGLE)
		*ap++ = 's';
	if (howto & RB_KDB)
		*ap++ = 'd';
	*ap++ = 0;
	if (ap[-2] == '-')
		*ap1 = 0;

	/* Left and Right LED on.  Max 15 for both LED. */
#define LEFT_LED(x)	(((x) & 0xf) << 4)
#define RIGHT_LED(x)	(((x) & 0xf))

	outb(PREP_BUS_SPACE_IO + 0x0c00, LEFT_LED(15) | RIGHT_LED(15));

	while (1);
}
