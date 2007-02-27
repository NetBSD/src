/*	$NetBSD: machdep.c,v 1.10.2.1 2007/02/27 16:52:32 yamt Exp $	*/
/*
 * Copyright (c) 2003 Anders Magnusson (ragge@ludd.luth.se).
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
 *    derived from this software without specific prior written permission
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

#include <sys/param.h>
#include <sys/signal.h>
#include <sys/proc.h>
#include <sys/ptrace.h>
#include <sys/systm.h>
#include <sys/mount.h>
#include <sys/syscallargs.h>
#include <sys/reboot.h>
#include <sys/sysctl.h>
#include <sys/msgbuf.h>
#include <sys/mbuf.h>
#include <sys/buf.h>
#include <sys/conf.h>

#include <machine/intr.h>

#include <dev/cons.h>

int physmem;
struct cpu_info cpu_info_store;
struct vm_map *exec_map, *mb_map;
char machine[] = MACHINE;		/* from <machine/param.h> */
char machine_arch[] = MACHINE_ARCH;	/* from <machine/param.h> */
char cpu_model[100];
caddr_t msgbufaddr;

void
cpu_startup()
{
	extern int avail_end;
	vaddr_t minaddr, maxaddr;
	char pbuf[9];

	spl0();	/* Enable interrupts */

	/*
	 * Initialize error message buffer.
	 */
	initmsgbuf(msgbufaddr, round_page(MSGBUFSIZE));

	/*
	 * Good {morning,afternoon,evening,night}.
	 */
	printf("%s%s", copyright, version);
	format_bytes(pbuf, sizeof(pbuf), avail_end);
	pbuf[strlen(pbuf)-1] = 0; /* Remove 'B' */
	printf("total memory = %sW\n", pbuf);

	minaddr = 0;
	/*
	 * Allocate a submap for exec arguments.  This map effectively limits
	 * the number of processes exec'ing at any time.
	 * At most one process with the full length is allowed.
	 */
	exec_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				 NCARGS/4, VM_MAP_PAGEABLE, false, NULL);

	/*
	 * allocate mbuf cluster submap.
	 */
	mb_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
	    nmbclusters * mclbytes, VM_MAP_INTRSAFE, false, NULL);

	format_bytes(pbuf, sizeof(pbuf), ptoa(uvmexp.free)/4);
	pbuf[strlen(pbuf)-1] = 0; /* Remove 'B' */
	printf("avail memory = %sW\n", pbuf);

#ifdef DDB
	if (boothowto & RB_KDB)
		Debugger();
#endif

	kl10_conf();
}

void
consinit()
{
	void dtecninit(struct consdev *);
	dtecninit(NULL);
}

int
process_read_regs(struct lwp *p, struct reg *regs)
{
	panic("process_read_regs");
	return 0;
}

void
cpu_dumpconf()
{
	panic("cpu_dumpconf");
}

void
setstatclockrate(int hzrate)
{
}

int
sys___sigreturn14(struct lwp *p, void *v, register_t *retval)
{
	panic("sys___sigreturn14");
	return 0;
}

void
sendsig(int sig, const sigset_t *mask, u_long code)
{
	panic("sendsig");
}

void
cpu_lwp_free(struct lwp *l, int proc)
{
	panic("cpu_lwp_free");
}

void
cpu_exit(struct lwp *l)
{
	panic("cpu_exit");
}

void
cpu_reboot(int howto, char *b)
{
	printf("cpu_reboot\n");
	asm("jrst 4,0765432");
	panic("foo");
}

int
process_set_pc(struct lwp *p, caddr_t addr)
{
	panic("process_set_pc");
}

int
process_sstep(struct lwp *p, int sstep)
{
	panic("process_sstep");
}

int
process_write_regs(struct lwp *p, const struct reg *regs)
{
	panic("process_write_regs");
}

void
cpu_getmcontext(struct lwp *l, mcontext_t *mcp, unsigned int *flags)
{
	panic("cpu_getmcontext");
}

int
cpu_setmcontext(struct lwp *l, const mcontext_t *mcp, unsigned int flags)
{
	panic("cpu_setmcontext");
	return 0;
}

int
cpu_switch (struct lwp *p, struct lwp *op)
{
	panic("cpu_switch");
}

void
cpu_switchto (struct lwp *p, struct lwp *op)
{
	panic("cpu_switchto");
}

ipl_cookie_t
makeiplcookie(ipl_t ipl)
{
	ipl_cookie_t icookie;

	icookie._pi = PI_LVLOFF | IPL2PI(ipl);
	return icookie;
}
