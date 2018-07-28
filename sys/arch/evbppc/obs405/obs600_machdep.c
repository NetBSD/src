/*	$NetBSD: obs600_machdep.c,v 1.9.38.1 2018/07/28 04:37:33 pgoyette Exp $	*/
/*	Original: md_machdep.c,v 1.3 2005/01/24 18:47:37 shige Exp $	*/

/*
 * Copyright 2001, 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Eduardo Horvath and Simon Burge for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

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
__KERNEL_RCSID(0, "$NetBSD: obs600_machdep.c,v 1.9.38.1 2018/07/28 04:37:33 pgoyette Exp $");

#include "opt_compat_netbsd.h"
#include "opt_ddb.h"
#include "opt_modular.h"

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/errno.h>
#include <sys/kernel.h>
#include <sys/ksyms.h>
#include <sys/mount.h>
#include <sys/reboot.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/module.h>
#include <sys/bus.h>
#include <sys/cpu.h>

#include <uvm/uvm_extern.h>

#include <machine/obs600.h>

#include <powerpc/ibm4xx/amcc405ex.h>
#include <powerpc/ibm4xx/cpu.h>
#include <powerpc/ibm4xx/dcr4xx.h>
#include <powerpc/ibm4xx/dev/comopbvar.h>
#include <powerpc/ibm4xx/dev/gpiicreg.h>
#include <powerpc/ibm4xx/dev/opbvar.h>

#include <powerpc/spr.h>
#include <powerpc/ibm4xx/spr.h>

#include <dev/ic/comreg.h>

#include "ksyms.h"

#include "com.h"
#if (NCOM > 0)
#include <sys/termios.h>

#ifndef CONADDR
#define CONADDR		AMCC405EX_UART0_BASE
#endif
#ifndef CONSPEED
#define CONSPEED	B115200
#endif
#ifndef CONMODE
			/* 8N1 */
#define CONMODE		((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8)
#endif
#endif	/* NCOM */

/*
 * XXXX:
 * It is very troublesome though we can calculate from various registers. X-<
 */
#define OBS600_CPU_FREQ	(600 * 1000 * 1000)
#define OBS600_MEM_SIZE	(1 * 1024 * 1024 * 1024)

#define	TLB_PG_SIZE 	(16 * 1024 * 1024)

/*
 * Global variables used here and there
 */
char bootpath[256];

extern paddr_t msgbuf_paddr;

void initppc(vaddr_t, vaddr_t, int, char *[], char *);
static int read_eeprom(int, char *);


void
initppc(vaddr_t startkernel, vaddr_t endkernel, int argc, char *argv[],
	char *argstr)
{
	vaddr_t va;
	u_int memsize;

	memsize = OBS600_MEM_SIZE;

	/* Linear map kernel memory */
	for (va = 0; va < endkernel; va += TLB_PG_SIZE)
		ppc4xx_tlb_reserve(va, va, TLB_PG_SIZE, TLB_EX);

	/*
	 * Map console and I2C after RAM. (see pmap_tlbmiss())
	 * All peripherals mapped on a page.
	 */
	ppc4xx_tlb_reserve(AMCC405EX_OPB_BASE, roundup(memsize, TLB_PG_SIZE),
	    TLB_PG_SIZE, TLB_I | TLB_G);

	/* Initialize AMCC 405EX CPU */
	ibm40x_memsize_init(memsize, startkernel);
	ibm4xx_init(startkernel, endkernel, pic_ext_intr);

#ifdef DDB
	if (boothowto & RB_KDB)
		Debugger();
#endif

	/*
	 * Look for the ibm4xx modules in the right place.
	 */
	module_machine = module_machine_ibm4xx;
}

void
consinit(void)
{

#if (NCOM > 0)
	com_opb_cnattach(OBS600_COM_FREQ, CONADDR, CONSPEED, CONMODE);
#endif /* NCOM */
}

/*
 * Machine dependent startup code.
 */
void
cpu_startup(void)
{
	prop_number_t pn;
	prop_data_t pd;
	u_char *macaddr, *macaddr1;
	static u_char buf[16];	/* MAC address x2 buffer */

	/*
	 * cpu common startup
	 */
	ibm4xx_cpu_startup("OpenBlockS600 AMCC PowerPC 405EX Board");

	/*
	 * Set up the board properties database.
	 */
	board_info_init();

	read_eeprom(sizeof(buf), buf);
	macaddr = &buf[0];
	macaddr1 = &buf[8];

	/*
	 * Now that we have VM, malloc()s are OK in bus_space.
	 */
	bus_space_mallocok();

	pn = prop_number_create_integer(OBS600_CPU_FREQ);
	KASSERT(pn != NULL);
	if (prop_dictionary_set(board_properties, "processor-frequency", pn) ==
	    false)
		panic("setting processor-frequency");
	prop_object_release(pn);

	pn = prop_number_create_integer(OBS600_MEM_SIZE);
	KASSERT(pn != NULL);
	if (prop_dictionary_set(board_properties, "mem-size", pn) == false)
		panic("setting mem-size");
	prop_object_release(pn);

#define ETHER_ADDR_LEN	6

	pd = prop_data_create_data_nocopy(macaddr, ETHER_ADDR_LEN);
	KASSERT(pd != NULL);
	if (prop_dictionary_set(board_properties, "emac0-mac-addr", pd) ==
	    false)
		panic("setting emac0-mac-addr");
	prop_object_release(pd);
	pd = prop_data_create_data_nocopy(macaddr1, ETHER_ADDR_LEN);
	KASSERT(pd != NULL);
	if (prop_dictionary_set(board_properties, "emac1-mac-addr", pd) ==
	    false)
		panic("setting emac1-mac-addr");
	prop_object_release(pd);

	/* emac0 connects to phy 2 and emac1 to phy 3 via RGMII. */
	pn = prop_number_create_integer(2);
	KASSERT(pn != NULL);
	if (prop_dictionary_set(board_properties, "emac0-mii-phy", pn) == false)
		panic("setting emac0-mii-phy");
	prop_object_release(pn);
	pn = prop_number_create_integer(3);
	KASSERT(pn != NULL);
	if (prop_dictionary_set(board_properties, "emac1-mii-phy", pn) == false)
		panic("setting emac1-mii-phy");
	prop_object_release(pn);

	/*
	 * no fake mapiodev
	 */
	fake_mapiodev = 0;

	splraise(-1);
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

	if (!cold && (howto & RB_DUMP))
		ibm4xx_dumpsys();

	doshutdownhooks();

	pmf_system_shutdown(boothowto);

	if ((howto & RB_POWERDOWN) == RB_POWERDOWN) {
	  /* Power off here if we know how...*/
	}

	if (howto & RB_HALT) {
		printf("halted\n\n");

#if 0
		goto reboot;	/* XXX for now... */
#endif

#ifdef DDB
		printf("dropping to debugger\n");
		while(1)
			Debugger();
#endif
	}

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

	/* flush cache for msgbuf */
	__syncicache((void *)msgbuf_paddr, round_page(MSGBUFSIZE));

#if 0
 reboot:
#endif
	ppc4xx_reset();

	printf("ppc4xx_reset() failed!\n");
#ifdef DDB
	while(1)
		Debugger();
#else
	while (1)
		/* nothing */;
#endif
}

/*
 * Read EEPROM via I2C.  We don't use bus_space(9) here.  This is MD-part and,
 * don't support bus_space_unmap() to a space on reserved space? X-<
 *
 * XXXX: Also this function assume already initialized for I2C...
 */
static int
read_eeprom(int len, char *buf)
{
	volatile uint8_t *iic0;
#define IIC0_READ(r)		(*(iic0 + (r)))
#define IIC0_WRITE(r, v)	(*(iic0 + (r)) = (v))
	uint8_t sts;
	int cnt, i = 0;

#define I2C_EEPROM_ADDR	0x52

	if ((iic0 = ppc4xx_tlb_mapiodev(AMCC405EX_IIC0_BASE, IIC_NREG)) == NULL)
		return ENOMEM; /* ??? */

	/* Clear Stop Complete Bit */
	IIC0_WRITE(IIC_STS, IIC_STS_SCMP);
	/* Check init */
	do {
		/* Get status */
		sts = IIC0_READ(IIC_STS);
	} while ((sts & IIC_STS_PT));

	IIC0_WRITE(IIC_MDCNTL,
	    IIC0_READ(IIC_MDCNTL) | IIC_MDCNTL_FMDB | IIC_MDCNTL_FSDB);

	/* 7-bit adressing */
	IIC0_WRITE(IIC_HMADR, 0);
	IIC0_WRITE(IIC_LMADR, I2C_EEPROM_ADDR << 1);

	IIC0_WRITE(IIC_MDBUF, 0);
	IIC0_WRITE(IIC_CNTL, IIC_CNTL_PT);
	do {
		/* Get status */
		sts = IIC0_READ(IIC_STS);
	} while ((sts & IIC_STS_PT) && !(sts & IIC_STS_ERR));

	cnt = 0;
	while (cnt < len) {
		/* always read 4byte */
		IIC0_WRITE(IIC_CNTL,
		    IIC_CNTL_PT				|
		    IIC_CNTL_RW				|
		    ((cnt == 0) ? IIC_CNTL_RPST : 0)	|
		    IIC_CNTL_TCT);
		do {
			/* Get status */
			sts = IIC0_READ(IIC_STS);
		} while ((sts & IIC_STS_PT) && !(sts & IIC_STS_ERR));

		if ((sts & IIC_STS_PT) || (sts & IIC_STS_ERR))
			break;
		if (sts & IIC_STS_MDBS) {
			delay(1);
			/* read 4byte */
			for (i = 0; i < 4 && cnt < len; i++, cnt++)
				buf[cnt] = IIC0_READ(IIC_MDBUF);
		}
	}
	for ( ; i < 4; i++)
		(void) IIC0_READ(IIC_MDBUF);

	return (cnt == len) ? 0 : EINVAL;
}
