/* $NetBSD: machdep.c,v 1.28.2.1 2009/05/13 17:16:01 jym Exp $ */

/*-
 * Copyright (c) 1998 Ben Harris
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
 * 3. The name of the author may not be used to endorse or promote products
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * machdep.c -- standard machine-dependent functions
 */

#include <sys/param.h>

__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.28.2.1 2009/05/13 17:16:01 jym Exp $");

#include <sys/buf.h>
#include <sys/kernel.h>
#include <sys/mbuf.h>
#include <sys/mount.h>
#include <sys/reboot.h>
#include <sys/sysctl.h>
#include <sys/systm.h>
#include <sys/cpu.h>
#include <sys/device.h>

#include <dev/i2c/i2cvar.h>
#include <dev/i2c/pcf8583var.h>

#include <arch/acorn26/ioc/iociicvar.h>

#include <uvm/uvm_extern.h>

#include <machine/machdep.h>
#include <machine/memcreg.h>

int physmem;
char cpu_model[] = "Archimedes";

struct vm_map *phys_map = NULL;
struct vm_map *mb_map = NULL; /* and ever more shall be so */

int waittime = -1;

void
cpu_reboot(int howto, char *b)
{

	/* If "always halt" was specified as a boot flag, obey. */
	if ((boothowto & RB_HALT) != 0)
		howto |= RB_HALT;

	boothowto = howto;

	/* If system is cold, just halt. */
	if (cold) {
		boothowto |= RB_HALT;
		goto haltsys;
	}

	if ((boothowto & RB_NOSYNC) == 0 && waittime < 0) {
		waittime = 0;
		vfs_shutdown();
		/*
		 * If we've been adjusting the clock, the todr
		 * will be out of synch; adjust it now.
		 */
		resettodr();
	}

	/* Disable interrupts. */
	splhigh();

#if 0
	/* XXX Need to implement this */
	if (boothowto & RB_DUMP)
		dumpsys();
#endif

	/* run any shutdown hooks */
	doshutdownhooks();

	pmf_system_shutdown(boothowto);

haltsys:
	if (howto & RB_HALT) {
		printf("system halted\n");
		for (;;);
	} else {
		printf("rebooting...");
		/*
		 * On a real reset, the ROM is mapped into address
		 * zero.  On RISC OS 3.11 at least, this can be faked
		 * by copying the first instruction in the ROM to
		 * address zero -- it immediately branches into the
		 * ROM at its usual address, and hence would remove
		 * the ROM mapped at zero anyway.
		 *
		 * In order to convince RISC OS to do a hard reset, we
		 * fake a *FX 200,2.  Note that I don't know if this
		 * will work on RISC OS <3.10.  This is slightly
		 * suboptimal as it causes RISC OS to zero the whole
		 * of RAM on startup (farewell, message buffer).  If
		 * anyone can tell me how to fake a control-reset in
		 * software, I'd be most grateful.
		 */
		*(volatile u_int8_t *)0x9c2 = 2; /* Zero page magic */
		*(volatile u_int32_t *)0
			= *(volatile u_int32_t *)MEMC_ROM_LOW_BASE;
		/* reboot in SVC mode, IRQs and FIQs disabled */
		__asm volatile("movs pc, %0" : :
		    "r" (R15_MODE_SVC | R15_FIQ_DISABLE | R15_IRQ_DISABLE));
	}
	panic("cpu_reboot failed");
}

/*
 * cpu_startup: allocate memory for variable-sized tables,
 * initialize CPU, and do autoconfiguration.
 */
void
cpu_startup(void)
{
	vaddr_t minaddr, maxaddr;
	char pbuf[9];

	/* Stuff to do here: */
	/* initmsgbuf() is called from start() */

	printf("%s%s", copyright, version);
	format_bytes(pbuf, sizeof(pbuf), ctob(physmem));
	printf("total memory = %s\n", pbuf);

	/* Various boilerplate memory allocations. */
	minaddr = 0;

	/*
	 * Allocate a submap for physio
	 */
	phys_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				   512 * 1024, 0, false, NULL);

	/*
	 * No need to allocate an mbuf cluster submap.  Mbuf clusters
	 * are allocated via the pool allocator, and we use direct-mapped
	 * pool pages.
	 */

	format_bytes(pbuf, sizeof(pbuf), ptoa(uvmexp.free));
	printf("avail memory = %s\n", pbuf);

	curpcb = &lwp0.l_addr->u_pcb;

#if 0
	/* Test exception handlers */
	__asm(".word 0x06000010"); /* undefined instruction */
	__asm("swi 0"); /* SWI */
	(*(void (*)(void))(0x00008000))(); /* prefetch abort */
	*(volatile int *)(0x00008000) = 0; /* data abort */
	*(volatile int *)(0x10000000) = 0; /* address exception */
#endif
}

/* Read a byte from CMOS RAM. */
int
cmos_read(int location)
{
	uint8_t val;

	KASSERT(iociic_bootstrap_cookie() != NULL);
	if (pcfrtc_bootstrap_read(iociic_bootstrap_cookie(), 0x50,
	    location, &val, 1) != 0)
		return (-1);
	return (val);
}

/* Write a byte to CMOS RAM. */
int
cmos_write(int location, int value)
{
	uint8_t val = value;

	KASSERT(iociic_bootstrap_cookie() != NULL);
	return (pcfrtc_bootstrap_write(iociic_bootstrap_cookie(), 0x50,
	    location, &val, 1));
}
