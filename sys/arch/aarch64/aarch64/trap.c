/* $NetBSD: trap.c,v 1.1.4.3 2017/12/03 11:35:43 jdolecek Exp $ */

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

__KERNEL_RCSID(1, "$NetBSD: trap.c,v 1.1.4.3 2017/12/03 11:35:43 jdolecek Exp $");

#include <sys/param.h>
#include <sys/types.h>
#include <sys/cpu.h>
#include <sys/proc.h>
#include <sys/userret.h>
#include <sys/systm.h>

#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/siginfo.h>

#include <aarch64/locore.h>

static void
dump_trapframe(struct trapframe *tf, void (*pr)(const char *, ...))
{
	(*pr)("trapframe @ %p:\n", tf);
	(*pr)("esr=%016"PRIxREGISTER
	    ",  pc=%016"PRIxREGISTER
	    ",  lr=%016"PRIxREGISTER
	    ",  sp=%016"PRIxREGISTER"\n",
	    tf->tf_esr, tf->tf_pc, tf->tf_lr, tf->tf_sp);
	(*pr)(" x0=%016"PRIxREGISTER
	    ",  x1=%016"PRIxREGISTER
	    ",  x2=%016"PRIxREGISTER
	    ",  x3=%016"PRIxREGISTER"\n",
	    tf->tf_reg[0], tf->tf_reg[1], tf->tf_reg[2], tf->tf_reg[3]);
	(*pr)(" x4=%016"PRIxREGISTER
	    ",  x5=%016"PRIxREGISTER
	    ",  x6=%016"PRIxREGISTER
	    ",  x7=%016"PRIxREGISTER"\n",
	    tf->tf_reg[4], tf->tf_reg[5], tf->tf_reg[6], tf->tf_reg[7]);
	(*pr)(" x8=%016"PRIxREGISTER
	    ",  x9=%016"PRIxREGISTER
	    ", x10=%016"PRIxREGISTER
	    ", x11=%016"PRIxREGISTER"\n",
	    tf->tf_reg[8], tf->tf_reg[9], tf->tf_reg[10], tf->tf_reg[11]);
	(*pr)("x12=%016"PRIxREGISTER
	    ", x13=%016"PRIxREGISTER
	    ", x14=%016"PRIxREGISTER
	    ", x15=%016"PRIxREGISTER"\n",
	    tf->tf_reg[12], tf->tf_reg[13], tf->tf_reg[14], tf->tf_reg[15]);
	(*pr)("x16=%016"PRIxREGISTER
	    ", x17=%016"PRIxREGISTER
	    ", x18=%016"PRIxREGISTER
	    ", x19=%016"PRIxREGISTER"\n",
	    tf->tf_reg[16], tf->tf_reg[17], tf->tf_reg[18], tf->tf_reg[19]);
	(*pr)("x20=%016"PRIxREGISTER
	    ", x21=%016"PRIxREGISTER
	    ", x22=%016"PRIxREGISTER
	    ", x23=%016"PRIxREGISTER"\n",
	    tf->tf_reg[20], tf->tf_reg[21], tf->tf_reg[22], tf->tf_reg[23]);
	(*pr)("x24=%016"PRIxREGISTER
	    ", x25=%016"PRIxREGISTER
	    ", x26=%016"PRIxREGISTER
	    ", x27=%016"PRIxREGISTER"\n",
	    tf->tf_reg[24], tf->tf_reg[25], tf->tf_reg[26], tf->tf_reg[27]);
	(*pr)("x28=%016"PRIxREGISTER
	    ", x29=%016"PRIxREGISTER
	    ", x30=%016"PRIxREGISTER"\n",
	    tf->tf_reg[28], tf->tf_reg[29], tf->tf_reg[30]);
}

void
userret(struct lwp *l, struct trapframe *tf)
{
	mi_userret(l);
}

void
trap(struct trapframe *tf, int reason)
{
	struct lwp * const l = curlwp;
	size_t code = tf->tf_esr & 0xffff;
	bool usertrap_p = tf->tf_esr & 01;
	bool ok = true;
	ksiginfo_t ksi;

	code = code;
	dump_trapframe(tf, printf);

	if (usertrap_p) {
		if (!ok)
			(*l->l_proc->p_emul->e_trapsignal)(l, &ksi);
		userret(l, tf);
	}
	else if (!ok) {
		dump_trapframe(tf, printf);
		panic("%s: fatal kernel trap", __func__);
	}
}

void
interrupt(struct trapframe *tf)
{
}

// XXXAARCH64 might be populated in frame.h in future

#define FB_X19	0
#define FB_X20	1
#define FB_X21	2
#define FB_X22	3
#define FB_X23	4
#define FB_X24	5
#define FB_X25	6
#define FB_X26	7
#define FB_X27	8
#define FB_X28	9
#define FB_X29	10
#define FB_LR	11
#define FB_SP	12
#define FB_V0	13
#define FB_MAX	14

struct faultbuf {
	register_t fb_reg[FB_MAX];
};

int	cpu_set_onfault(struct faultbuf *, register_t) __returns_twice;
void	cpu_jump_onfault(struct trapframe *, const struct faultbuf *);
void	cpu_unset_onfault(void);
struct faultbuf *cpu_disable_onfault(void);
void	cpu_enable_onfault(struct faultbuf *);

void
cpu_jump_onfault(struct trapframe *tf, const struct faultbuf *fb)
{

	tf->tf_reg[19] = fb->fb_reg[FB_X19];
	tf->tf_reg[20] = fb->fb_reg[FB_X20];
	tf->tf_reg[21] = fb->fb_reg[FB_X21];
	tf->tf_reg[22] = fb->fb_reg[FB_X22];
	tf->tf_reg[23] = fb->fb_reg[FB_X23];
	tf->tf_reg[24] = fb->fb_reg[FB_X24];
	tf->tf_reg[25] = fb->fb_reg[FB_X25];
	tf->tf_reg[26] = fb->fb_reg[FB_X26];
	tf->tf_reg[27] = fb->fb_reg[FB_X27];
	tf->tf_reg[28] = fb->fb_reg[FB_X28];
	tf->tf_reg[29] = fb->fb_reg[FB_X29];
	tf->tf_reg[0] = fb->fb_reg[FB_V0];
	tf->tf_sp = fb->fb_reg[FB_SP];
	tf->tf_lr = fb->fb_reg[FB_LR];
}

void
cpu_unset_onfault(void)
{

	curlwp->l_md.md_onfault = NULL;
}

struct faultbuf *
cpu_disable_onfault(void)
{
	struct faultbuf * const fb = curlwp->l_md.md_onfault;

	curlwp->l_md.md_onfault = NULL;
	return fb;
}

void
cpu_enable_onfault(struct faultbuf *fb)
{

	curlwp->l_md.md_onfault = NULL;
}

/*
 * kcopy(9)
 * int kcopy(const void *src, void *dst, size_t len);
 *
 * copy(9)
 * int copyin(const void *uaddr, void *kaddr, size_t len);
 * int copyout(const void *kaddr, void *uaddr, size_t len);
 * int copystr(const void *kfaddr, void *kdaddr, size_t len, size_t *done);
 * int copyinstr(const void *uaddr, void *kaddr, size_t len, size_t *done);
 * int copyoutstr(const void *kaddr, void *uaddr, size_t len, size_t *done);
 */

int
kcopy(const void *kfaddr, void *kdaddr, size_t len)
{
	struct faultbuf fb;
	int error;

	if ((error = cpu_set_onfault(&fb, EFAULT)) == 0) {
		memcpy(kdaddr, kfaddr, len);
		cpu_unset_onfault();
	}
	return error;
}

int
copyin(const void *uaddr, void *kaddr, size_t len)
{
	struct faultbuf fb;
	int error;

	if ((error = cpu_set_onfault(&fb, EFAULT)) == 0) {
		memcpy(kaddr, uaddr, len);
		cpu_unset_onfault();
	}
	return error;
}

int
copyout(const void *kaddr, void *uaddr, size_t len)
{
	struct faultbuf fb;
	int error;

	if ((error = cpu_set_onfault(&fb, EFAULT)) == 0) {
		memcpy(uaddr, kaddr, len);
		cpu_unset_onfault();
	}
	return error;
}

int
copystr(const void *kfaddr, void *kdaddr, size_t len, size_t *done)
{
	struct faultbuf fb;
	int error;

	if ((error = cpu_set_onfault(&fb, EFAULT)) == 0) {
		len = strlcpy(kdaddr, kfaddr, len);
		cpu_unset_onfault();
		if (done != NULL) {
			*done = len;
		}
	}
	return error;
}

int
copyinstr(const void *uaddr, void *kaddr, size_t len, size_t *done)
{
	struct faultbuf fb;
	int error;

	if ((error = cpu_set_onfault(&fb, EFAULT)) == 0) {
		len = strlcpy(kaddr, uaddr, len);
		cpu_unset_onfault();
		if (done != NULL) {
			*done = len;
		}
	}
	return error;
}

int
copyoutstr(const void *kaddr, void *uaddr, size_t len, size_t *done)
{
	struct faultbuf fb;
	int error;

	if ((error = cpu_set_onfault(&fb, EFAULT)) == 0) {
		len = strlcpy(uaddr, kaddr, len);
		cpu_unset_onfault();
		if (done != NULL) {
			*done = len;
		}
	}
	return error;
}

/*
 * fetch(9)
 * int fubyte(const void *base);
 * int fusword(const void *base);
 * int fuswintr(const void *base);
 * long fuword(const void *base);
 *
 * store(9)
 * int subyte(void *base, int c);
 * int susword(void *base, short c);
 * int suswintr(void *base, short c);
 * int suword(void *base, long c);
 */

union xubuf {
	uint8_t b[4];
	uint16_t w[2];
	uint32_t l[1];
};

static bool
fetch_user_data(union xubuf *xu, const void *base, size_t len)
{
	struct faultbuf fb;

	if (cpu_set_onfault(&fb, 1) == 0) {
		memcpy(xu->b, base, len);
		cpu_unset_onfault();
		return true;
	}
	return false;
}

int
fubyte(const void *base)
{
	union xubuf xu;

	if (fetch_user_data(&xu, base, sizeof(xu.b[0])))
		return xu.b[0];
	return -1;
}

int
fusword(const void *base)
{
	union xubuf xu;

	if (fetch_user_data(&xu, base, sizeof(xu.w[0])))
		return xu.w[0];
	return -1;
}

int
fuswintr(const void *base)
{

	return -1;
}

long
fuword(const void *base)
{
	union xubuf xu;

	if (fetch_user_data(&xu, base, sizeof(xu.l[0])))
		return xu.l[0];
	return -1;
}

static bool
store_user_data(void *base, const union xubuf *xu, size_t len)
{
	struct faultbuf fb;

	if (cpu_set_onfault(&fb, 1) == 0) {
		memcpy(base, xu->b, len);
		cpu_unset_onfault();
		return true;
	}
	return false;
}

int
subyte(void *base, int c)
{
	union xubuf xu;

	xu.l[0] = 0; xu.b[0] = c; // { .b[0] = c, .b[1 ... 3] = 0 }
	return store_user_data(base, &xu, sizeof(xu.b[0])) ? 0 : -1;
}

int
susword(void *base, short c)
{
	union xubuf xu;

	xu.l[0] = 0; xu.w[0] = c; // { .w[0] = c, .w[1] = 0 }
	return store_user_data(base, &xu, sizeof(xu.w[0])) ? 0 : -1;
}

int
suswintr(void *base, short c)
{

	return -1;
}

int
suword(void *base, long c)
{
	union xubuf xu;

	xu.l[0] = c; // { .l[0] = c }
	return store_user_data(base, &xu, sizeof(xu.l[0])) ? 0 : -1;
}
