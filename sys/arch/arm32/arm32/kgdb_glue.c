/*	$NetBSD: kgdb_glue.c,v 1.5 1996/10/15 02:11:31 mark Exp $	*/

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

#include <kgdb/kgdb.h>

#include <vm/vm.h>

#include <machine/cpu.h>
#include <machine/katelib.h>
#include <machine/psl.h>
#include <machine/pte.h>
#include <machine/pmap.h>
#include <machine/kgdb.h>

int kgdbregs[NREG];

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
kgdbinit()
{
}

int
kgdb_poll()
{
	return 0;
}

void
kgdb_trap()
{
	__asm(".word 0xe6000010");
}

int
kgdb_trap_glue(regs)
	int *regs;
{
	int inst;
	int cnt;

	inst = fetchinst(regs[PC] - 4);
	switch (inst) {
	default:
		/* non KGDB undefined instruction */
		return 0;
	case 0xe6000011:	/* KGDB installed breakpoint */
		regs[PC] -= 4;
		break;
	case 0xe6000010:	/* breakpoint in kgdb_connect */
		break;
	}
	while (1) {
		kgdbcopy(regs, kgdbregs, sizeof kgdbregs);
		switch (kgdbcmds()) {
		case 1:
			kgdbcopy(kgdbregs, regs, sizeof kgdbregs);
			if ((cnt = singlestep(regs)) < 0)
				panic("singlestep");
			regs[PC] += cnt;
			continue;
		default:
			break;
		}
		break;
	}
	kgdbcopy(kgdbregs, regs, sizeof kgdbregs);
	if (PSR_IN_USR_MODE(regs[PSR]) ||
	    !PSR_IN_32_MODE(regs[PSR]))
		panic("KGDB: invalid mode %x", regs[PSR]);
	return 1;
}

void
kgdbcopy(vs, vd, n)
	void *vs, *vd;
	int n;
{
	char *s = vs, *d = vd;
	
	while (--n >= 0)
		*d++ = *s++;
}

void
kgdbzero(vd, n)
	void *vd;
	int n;
{
	char *d = vd;

	while (--n >= 0)
		*d++ = 0;
}

int
kgdbcmp(vs, vd, n)
	void *vs, *vd;
	int n;
{
	char *s = vs, *d = vd;
	
	while (--n >= 0)
		if (*d++ != *s++)
			return *--d - *--s;
	return 0;
}


int kgdbfbyte(src)
unsigned char *src;
{
  /* modified db_interface.c source */
  pt_entry_t *ptep;
  vm_offset_t va;
  int ch;
         
  va = (unsigned long)src & (~PGOFSET);
  ptep = vtopte(va);
  
  if ((*ptep & L2_MASK) == L2_INVAL) {
    return -1;
  }        
         
  ch = *src;
  
  return ch;
 } /* kgdbfbyte */


int kgdbsbyte(dst, ch)
unsigned char *dst;
int ch;
{
  /* modified db_interface.c source */
  pt_entry_t *ptep, pte, pteo;
  vm_offset_t va;
         
  va = (unsigned long)dst & (~PGOFSET);
  ptep = vtopte(va);
  
  if ((*ptep & L2_MASK) == L2_INVAL) {
    return -1;
  }        
  
  pteo = ReadWord(ptep);
  pte = pteo | PT_AP(AP_KRW);
  WriteWord(ptep, pte);
  tlbflush();
         
  *dst = (unsigned char)ch;
  
  WriteWord(ptep, pteo);
  tlbflush();
        
  return 0;
 } /* kgdbsbyte */
