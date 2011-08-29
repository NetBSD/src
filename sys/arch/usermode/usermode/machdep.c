/* $NetBSD: machdep.c,v 1.18 2011/08/29 12:46:58 reinoud Exp $ */

/*-
 * Copyright (c) 2007 Jared D. McNeill <jmcneill@invisible.ca>
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.18 2011/08/29 12:46:58 reinoud Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/exec.h>
#include <sys/buf.h>
#include <sys/boot_flag.h>
#include <sys/ucontext.h>
#include <machine/pcb.h>

#include <uvm/uvm_extern.h>
#include <uvm/uvm_page.h>

#include <dev/mm.h>

#include <machine/thunk.h>

#include "opt_memsize.h"
#include "opt_sdl.h"

char machine[] = "usermode";
char machine_arch[] = "usermode";

int		usermode_x = IPL_NONE;
/* XXX */
int		physmem = MEMSIZE * 1024 / PAGE_SIZE;

static char **saved_argv;
char *usermode_root_image_path = NULL;

void	main(int argc, char *argv[]);
void	usermode_reboot(void);

void
main(int argc, char *argv[])
{
#if defined(SDL)
	extern int genfb_thunkbus_cnattach(void);
#endif
	extern void ttycons_consinit(void);
	extern void pmap_bootstrap(void);
	extern void kernmain(void);
	int i, j, r, tmpopt = 0;

	saved_argv = argv;

#if defined(SDL)
	if (genfb_thunkbus_cnattach() == 0)
#endif
		ttycons_consinit();

	for (i = 1; i < argc; i++) {
		if (argv[i][0] != '-') {
			usermode_root_image_path = argv[i];
			continue;
		}
		for (j = 1; argv[i][j] != '\0'; j++) {
			r = 0;
			BOOT_FLAG(argv[i][j], r);
			if (r == 0) {
				printf("-%c: unknown flag\n", argv[i][j]);
				printf("usage: %s [-acdqsvxz]\n", argv[0]);
				printf("       (ex. \"%s -s\")\n", argv[0]);
				return;
			}
			tmpopt |= r;
		}
	}
	boothowto = tmpopt;

	uvm_setpagesize();
	uvmexp.ncolors = 2;

	pmap_bootstrap();

	splraise(IPL_HIGH);

	kernmain();
}

void
usermode_reboot(void)
{
	struct thunk_itimerval itimer;

	/* make sure the timer is turned off */
	memset(&itimer, 0, sizeof(itimer));
	thunk_setitimer(ITIMER_REAL, &itimer, NULL);

	if (thunk_execv(saved_argv[0], saved_argv) == -1)
		thunk_abort();
	/* NOTREACHED */
}

void
setstatclockrate(int arg)
{
}

void
consinit(void)
{
	printf("NetBSD/usermode startup\n");
}

void
setregs(struct lwp *l, struct exec_package *pack, vaddr_t stack)
{
	struct pcb *pcb = lwp_getpcb(l);
	ucontext_t *ucp = &pcb->pcb_userland_ucp;

printf("setregs called: lwp %p, exec package %p, stack %p\n", l, pack, (void *) stack);
printf("current stat of pcb %p\n", pcb);
printf("\tpcb->pcb_ucp.uc_stack.ss_sp   = %p\n", pcb->pcb_ucp.uc_stack.ss_sp);
printf("\tpcb->pcb_ucp.uc_stack.ss_size = %d\n", (int) pcb->pcb_ucp.uc_stack.ss_size);
printf("\tpcb->pcb_userland_ucp.uc_stack.ss_sp   = %p\n", pcb->pcb_userland_ucp.uc_stack.ss_sp);
printf("\tpcb->pcb_userland_ucp.uc_stack.ss_size = %d\n", (int) pcb->pcb_userland_ucp.uc_stack.ss_size);

	ucp->uc_stack.ss_sp = (void *) stack;
	ucp->uc_stack.ss_size = pack->ep_ssize;
	thunk_makecontext_trapframe2go(ucp, (void *) pack->ep_entry, &pcb->pcb_tf);

printf("updated pcb %p\n", pcb);
printf("\tpcb->pcb_ucp.uc_stack.ss_sp   = %p\n", pcb->pcb_ucp.uc_stack.ss_sp);
printf("\tpcb->pcb_ucp.uc_stack.ss_size = %d\n", (int) pcb->pcb_ucp.uc_stack.ss_size);
printf("\tpcb->pcb_userland_ucp.uc_stack.ss_sp   = %p\n", pcb->pcb_userland_ucp.uc_stack.ss_sp);
printf("\tpcb->pcb_userland_ucp.uc_stack.ss_size = %d\n", (int) pcb->pcb_userland_ucp.uc_stack.ss_size);
printf("\tpack->ep_entry                = %p\n", (void *) pack->ep_entry);
printf("\t    argument                  = %p\n", &pcb->pcb_tf);
}

void
sendsig_siginfo(const ksiginfo_t *ksi, const sigset_t *mask)
{
}

int
mm_md_physacc(paddr_t pa, vm_prot_t prog)
{
	return 0;
}
