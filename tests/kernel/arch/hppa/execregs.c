/*	$NetBSD: execregs.c,v 1.2 2025/02/28 16:08:19 riastradh Exp $	*/

/*-
 * Copyright (c) 2025 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD: execregs.c,v 1.2 2025/02/28 16:08:19 riastradh Exp $");

#include "execregs.h"

#include <errno.h>
#include <spawn.h>
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>

extern char **environ;

static unsigned long
nonnull(unsigned long x)
{

	x |= x << 8;
	x |= x << 16;
	return x;
}

/*
 * setfpregs()
 *
 *	Set up all the floating-point registers with something nonzero
 *	in each one.  We initialize the floating-point status register
 *	to set various bits so it's not all zero, but nothing that
 *	would trigger traps.
 */
static void
setfpregs(void)
{
	static const uint64_t fpe[] = {
		(__BITS(63,59)		/* all exception flags (VZOUI) set */
		    | __BIT(58)		/* C bit (comparison) set */
		    | __BITS(54,43)	/* CQ (comparison queue) all set */
		    | __SHIFTIN(1, __BITS(42,41))	/* round toward zero */
		    | __SHIFTIN(0, __BIT(38))	/* no delayed trap */
		    | __SHIFTIN(1, __BIT(37))	/* Denormalized As Zero */
		    | __SHIFTIN(0, __BITS(36,32))	/* exceptions masked */
		    | 0x10101010),
		0x9191919111111111,
		0x9292929212121212,
		0x9393939313131313,
	};
	const uint64_t *fpep = fpe;

	static const double fr[28] = {
		0x1.04p0, 0x1.05p0, 0x1.06p0, 0x1.07p0,
		0x1.08p0, 0x1.09p0, 0x1.0ap0, 0x1.0bp0,
		0x1.0cp0, 0x1.0dp0, 0x1.0ep0, 0x1.0fp0,
		0x1.10p0, 0x1.11p0, 0x1.12p0, 0x1.13p0,
		0x1.14p0, 0x1.15p0, 0x1.16p0, 0x1.17p0,
		0x1.18p0, 0x1.19p0, 0x1.1ap0, 0x1.1bp0,
		0x1.1cp0, 0x1.1dp0, 0x1.1ep0, 0x1.1fp0,
	};
	const double *frp = fr;

	__asm volatile(
		"fldds,ma	8(%0), %%fr0\n\t"
		"fldds,ma	8(%0), %%fr1\n\t"
		"fldds,ma	8(%0), %%fr2\n\t"
		"fldds		0(%0), %%fr3"
	    : "+r"(fpep)
	    : "m"(fpe));

	__asm volatile(
		"fldds,ma	8(%0), %%fr4\n\t"
		"fldds,ma	8(%0), %%fr5\n\t"
		"fldds,ma	8(%0), %%fr6\n\t"
		"fldds,ma	8(%0), %%fr7\n\t"
		"fldds,ma	8(%0), %%fr8\n\t"
		"fldds,ma	8(%0), %%fr9\n\t"
		"fldds,ma	8(%0), %%fr10\n\t"
		"fldds,ma	8(%0), %%fr11\n\t"
		"fldds,ma	8(%0), %%fr12\n\t"
		"fldds,ma	8(%0), %%fr13\n\t"
		"fldds,ma	8(%0), %%fr14\n\t"
		"fldds,ma	8(%0), %%fr15\n\t"
		"fldds,ma	8(%0), %%fr16\n\t"
		"fldds,ma	8(%0), %%fr17\n\t"
		"fldds,ma	8(%0), %%fr18\n\t"
		"fldds,ma	8(%0), %%fr19\n\t"
		"fldds,ma	8(%0), %%fr20\n\t"
		"fldds,ma	8(%0), %%fr21\n\t"
		"fldds,ma	8(%0), %%fr22\n\t"
		"fldds,ma	8(%0), %%fr23\n\t"
		"fldds,ma	8(%0), %%fr24\n\t"
		"fldds,ma	8(%0), %%fr25\n\t"
		"fldds,ma	8(%0), %%fr26\n\t"
		"fldds,ma	8(%0), %%fr27\n\t"
		"fldds,ma	8(%0), %%fr28\n\t"
		"fldds,ma	8(%0), %%fr29\n\t"
		"fldds,ma	8(%0), %%fr30\n\t"
		"fldds		0(%0), %%fr31"
	    : "+r"(frp)
	    : "m"(fr));
}

/*
 * setpsw()
 *
 *	Set some bits in PSW, the processor status word.
 */
static void
setpsw(void)
{
	uint32_t x = 0xe0000000, y = 0xffffffff, sum;

	/*
	 * Trigger some arithmetic that causes the carry/borrow
	 * (PSW[C/B]) bits to be set.
	 *
	 * XXX Also set PSW[V].
	 */
	__asm volatile("sh3add %[sum], %[x], %[y]"
	    : /* outputs */ [sum] "=r"(sum)
	    : /* inputs */ [x] "r"(x), [y] "r"(y));
}

int
execregschild(char *path)
{
	register long t1 __asm("r22") = nonnull(22);
	register long t2 __asm("r21") = nonnull(21);
	/* r30/sp: stack pointer */
	register long t3 __asm("r20") = nonnull(20);
	/* cr17/iisq_head: privileged */
	/* cr17/iisq_tail: privileged */
	/* cr18/iioq_head: privileged */
	/* cr18/iioq_tail: privileged */
	/* cr15/eiem: privileged */
	/* cr22/ipsw: privileged */
	/* sr3: privileged(?) */
	/* cr8/pidr1: privileged */
	/* cr20/isr: privileged */
	/* cr21/ior: privileged */
	/* cr19/iir: privileged */
	/* flags: N/A(?) */
	long sar = nonnull(0x8a);	/* cr11 */
	/* r1: ADDIL (add immediate left) result, nonnull anyway */
	/* r2/rp: return pointer, nonnull anyway */
	/* r3: frame pointer, nonnull anyway */
	register long r4 __asm("r4") = nonnull(4);
	register long r5 __asm("r5") = nonnull(5);
	register long r6 __asm("r6") = nonnull(6);
	register long r7 __asm("r7") = nonnull(7);
	register long r8 __asm("r8") = nonnull(8);
	register long r9 __asm("r9") = nonnull(9);
	register long r10 __asm("r10") = nonnull(10);
	register long r11 __asm("r11") = nonnull(11);
	register long r12 __asm("r12") = nonnull(12);
	register long r13 __asm("r13") = nonnull(13);
	register long r14 __asm("r14") = nonnull(14);
	register long r15 __asm("r15") = nonnull(15);
	register long r16 __asm("r16") = nonnull(16);
	register long r17 __asm("r17") = nonnull(17);
	register long r18 __asm("r18") = nonnull(18);
	register long t4 __asm("r19") = nonnull(19);
	register long arg3 __asm("r23") = nonnull(23);
	/* r24/arg2: envp, nonnull anyway */
	/* r25/arg1: argv, nonnull anyway */
	/* r26/arg0: path, nonnull anyway */
	/* r27/dp: data pointer, nonnull anyway */
	register long ret0 __asm("r28") = nonnull(28);
	register long ret1 __asm("r29") = nonnull(29);
	register long r31 __asm("r31") = nonnull(31);
	/* sr0-sr7: space registers initialized by kernel */
	/* cr9/pidr2: privileged */
	/* cr12/pidr3: privileged */
	/* cr13/pidr4: privileged */
	/* cr0/rctr: privileged */
	/* cr10/ccr: privileged */
	/* cr23/eirr: privileged */
	/* cr24: privileged */
	/* cr25/vtop: privileged */
	/* cr27/tr3: _lwp_private, thread-local storage -- nonnull anyway */
	/* cr28: privileged */
	/* cr30/fpregs: privileged */
	/* cr31: privileged */

	char *argv[] = {path, NULL};
	char **envp = environ;

	setfpregs();
	setpsw();

	/*
	 * Not perfect -- compiler might use some registers for
	 * stack/argument transfers, but all the arguments are nonnull
	 * so this is probably a good test anyway.
	 */
	__asm volatile("mtctl %[sar], %%sar"	/* cr11 */
	    : /* outputs */
	    : [sar] "r"(sar)
	    : "memory");
	__asm volatile("" :
	    "+r"(t1),
	    "+r"(t2),
	    "+r"(t3),
	    "+r"(r4),
	    "+r"(r5),
	    "+r"(r6),
	    "+r"(r7),
	    "+r"(r8),
	    "+r"(r9),
	    "+r"(r10),
	    "+r"(r11),
	    "+r"(r12)
	    :: "memory");
	/* pacify gcc error: more than 30 operands in 'asm' */
	__asm volatile("" :
	    "+r"(r13),
	    "+r"(r14),
	    "+r"(r15),
	    "+r"(r16),
	    "+r"(r17),
	    "+r"(r18),
	    "+r"(t4),
	    "+r"(arg3),
	    "+r"(ret0),
	    "+r"(ret1),
	    "+r"(r31)
	    :: "memory");

	return execve(path, argv, envp);
}

pid_t
spawnregschild(char *path, int fd)
{
	register long t1 __asm("r22") = nonnull(22);
	register long t2 __asm("r21") = nonnull(21);
	/* r30/sp: stack pointer */
	register long t3 __asm("r20") = nonnull(20);
	/* cr17/iisq_head: privileged */
	/* cr17/iisq_tail: privileged */
	/* cr18/iioq_head: privileged */
	/* cr18/iioq_tail: privileged */
	/* cr15/eiem: privileged */
	/* cr22/ipsw: privileged */
	/* sr3: privileged(?) */
	/* cr8/pidr1: privileged */
	/* cr20/isr: privileged */
	/* cr21/ior: privileged */
	/* cr19/iir: privileged */
	/* flags: N/A(?) */
	long sar = nonnull(0x8a);	/* cr11 */
	/* r1: ADDIL (add immediate left) result, nonnull anyway */
	/* r2/rp: return pointer, nonnull anyway */
	/* r3: frame pointer, nonnull anyway */
	register long r4 __asm("r4") = nonnull(4);
	register long r5 __asm("r5") = nonnull(5);
	register long r6 __asm("r6") = nonnull(6);
	register long r7 __asm("r7") = nonnull(7);
	register long r8 __asm("r8") = nonnull(8);
	register long r9 __asm("r9") = nonnull(9);
	register long r10 __asm("r10") = nonnull(10);
	register long r11 __asm("r11") = nonnull(11);
	register long r12 __asm("r12") = nonnull(12);
	register long r13 __asm("r13") = nonnull(13);
	register long r14 __asm("r14") = nonnull(14);
	register long r15 __asm("r15") = nonnull(15);
	register long r16 __asm("r16") = nonnull(16);
	register long r17 __asm("r17") = nonnull(17);
	register long r18 __asm("r18") = nonnull(18);
	register long t4 __asm("r19") = nonnull(19);
	/* r23/arg3: attrp, nonnull anyway */
	/* r24/arg2: fileactsp, nonnull anyway */
	/* r25/arg1: path, nonnull anyway */
	/* r26/arg0: pidp, nonnull anyway */
	/* r27/dp: data pointer, nonnull anyway */
	register long ret0 __asm("r28") = nonnull(28);
	register long ret1 __asm("r29") = nonnull(29);
	register long r31 __asm("r31") = nonnull(31);
	/* sr0-sr7: space registers initialized by kernel */
	/* cr9/pidr2: privileged */
	/* cr12/pidr3: privileged */
	/* cr13/pidr4: privileged */
	/* cr0/rctr: privileged */
	/* cr10/ccr: privileged */
	/* cr23/eirr: privileged */
	/* cr24: privileged */
	/* cr25/vtop: privileged */
	/* cr27/tr3: _lwp_private, thread-local storage -- nonnull anyway */
	/* cr28: privileged */
	/* cr30/fpregs: privileged */
	/* cr31: privileged */

	char *argv[] = {path, NULL};
	char **envp = environ;
	posix_spawn_file_actions_t fileacts;
	posix_spawnattr_t attr;
	pid_t pid;
	int error;

	error = posix_spawn_file_actions_init(&fileacts);
	if (error)
		goto out;
	error = posix_spawn_file_actions_adddup2(&fileacts, fd, STDOUT_FILENO);
	if (error)
		goto out;
	error = posix_spawnattr_init(&attr);
	if (error)
		goto out;

	setfpregs();
	setpsw();

	/*
	 * Not perfect -- compiler might use some registers for
	 * stack/argument transfers, but all the arguments are nonnull
	 * so this is probably a good test anyway.
	 */
	__asm volatile("mtctl %[sar], %%sar"	/* cr11 */
	    : /* outputs */
	    : [sar] "r"(sar)
	    : "memory");
	__asm volatile("" :
	    "+r"(t1),
	    "+r"(t2),
	    "+r"(t3),
	    "+r"(r4),
	    "+r"(r5),
	    "+r"(r6),
	    "+r"(r7),
	    "+r"(r8),
	    "+r"(r9),
	    "+r"(r10),
	    "+r"(r11),
	    "+r"(r12)
	    :: "memory");
	/* pacify gcc error: more than 30 operands in 'asm' */
	__asm volatile("" :
	    "+r"(r13),
	    "+r"(r14),
	    "+r"(r15),
	    "+r"(r16),
	    "+r"(r17),
	    "+r"(r18),
	    "+r"(t4),
	    "+r"(ret0),
	    "+r"(ret1),
	    "+r"(r31)
	    :: "memory");

	error = posix_spawn(&pid, path, &fileacts, &attr, argv, envp);
	if (error)
		goto out;

out:	posix_spawnattr_destroy(&attr);
	posix_spawn_file_actions_destroy(&fileacts);
	if (error) {
		errno = error;
		return -1;
	}
	return 0;
}
