/*	$NetBSD: ipkdb_glue.c,v 1.1.2.2 2002/02/28 04:11:56 nathanw Exp $	*/

/*
 * Copyright (C) 1994 Wolfgang Solfrank.
 * Copyright (C) 1996 Frank Lancaster.
 * Copyright (C) 1994-1996 TooLs GmbH.
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/reboot.h>

#include <ipkdb/ipkdb.h>

#include <uvm/uvm_extern.h>

#include <machine/cpu.h>
#include <arm/arm32/katelib.h>
#include <machine/pmap.h>
#include <machine/ipkdb.h>
#include <machine/trap.h>

int ipkdbregs[NREG];

dump(p, l)
	u_char *p;
{
	int i, j, n;
	
	while (l > 0) {
		printf("%08x: ", p);
		n = l > 16 ? 16 : l;
		for (i = 4; --i >= 0;) {
			for (j = 4; --j >= 0;)
				printf(--n >= 0 ? "%02x " : "   ", *p++);
			printf(" ");
		}
		p -= 16;
		n = l > 16 ? 16 : l;
		n = (n + 3) & ~3;
		for (i = 4; --i >= 0;)
			printf((n -= 4) >= 0 ? "%08x " : "", *((long *)p)++);
		printf("\n");
		l -= 16;
	}
}

void
ipkdbinit()
{
}

int
ipkdb_poll()
{
	return 0;
}

void
ipkdb_trap()
{
	__asm(".word 0xe6000010");
}

int
ipkdb_trap_glue(regs)
	int *regs;
{
	int inst;
	int cnt;

	inst = fetchinst(regs[PC] - INSN_SIZE);
	switch (inst) {
	default:
		/* non IPKDB undefined instruction */
		return 0;
	case GDB_BREAKPOINT:	/* IPKDB installed breakpoint */
		regs[PC] -= INSN_SIZE;
		break;
	case IPKDB_BREAKPOINT:	/* breakpoint in ipkdb_connect */
		break;
	}
	while (1) {
		ipkdbcopy(regs, ipkdbregs, sizeof ipkdbregs);
		switch (ipkdbcmds()) {
		case 1:
			ipkdbcopy(ipkdbregs, regs, sizeof ipkdbregs);
			if ((cnt = singlestep(regs)) < 0)
				panic("singlestep");
			regs[PC] += cnt;
			continue;
		default:
			break;
		}
		break;
	}
	ipkdbcopy(ipkdbregs, regs, sizeof ipkdbregs);
	if (PSR_IN_USR_MODE(regs[PSR]) ||
	    !PSR_IN_32_MODE(regs[PSR]))
		panic("IPKDB: invalid mode %x", regs[PSR]);
	return 1;
}

void
ipkdbcopy(vs, vd, n)
	void *vs, *vd;
	int n;
{
	char *s = vs, *d = vd;
	
	while (--n >= 0)
		*d++ = *s++;
}

void
ipkdbzero(vd, n)
	void *vd;
	int n;
{
	char *d = vd;

	while (--n >= 0)
		*d++ = 0;
}

int
ipkdbcmp(vs, vd, n)
	void *vs, *vd;
	int n;
{
	char *s = vs, *d = vd;
	
	while (--n >= 0)
		if (*d++ != *s++)
			return *--d - *--s;
	return 0;
}


int
ipkdbfbyte(src)
	unsigned char *src;
{
	pt_entry_t *ptep;
	vaddr_t va;
	int ch;

	va = (vaddr_t)src & (~PGOFSET);
	ptep = vtopte(va);

	if ((*ptep & L2_MASK) == L2_INVAL)
		return -1;

	ch = *src;

	return ch;
}


int
ipkdbsbyte(dst, ch)
	unsigned char *dst;
	int ch;
{
	pt_entry_t *ptep, pte, pteo;
	vaddr_t va;

	va = (vaddr_t)dst & (~PGOFSET);
	ptep = vtopte(va);

	if ((*ptep & L2_MASK) == L2_INVAL)
		return -1;

	pteo = ReadWord(ptep);
	pte = pteo | PT_AP(AP_KRW);
	WriteWord(ptep, pte);
	tlbflush();		/* XXX should be purge */
         
	*dst = (unsigned char)ch;

	/* make sure the caches and memory are in sync */
	cpu_cache_syncI_rng((u_int)dst, 4);

	WriteWord(ptep, pteo);
	tlbflush();		/* XXX should be purge */

	return 0;
}
