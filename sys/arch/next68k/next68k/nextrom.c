/*	$NetBSD: nextrom.c,v 1.2 1998/07/01 22:23:40 dbj Exp $	*/
/*
 * Copyright (c) 1998 Darrin B. Jewell
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
 *      This product includes software developed by Darrin B. Jewell
 * 4. The name of the author may not be used to endorse or promote products
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

#include <sys/types.h>
#include <machine/cpu.h>

#include <next68k/next68k/seglist.h>

#include <next68k/next68k/nextrom.h>


int mon_getc(void);
int mon_putc(int c);


#if 0
struct mon_global *mg;
#else
char *mg;
#endif

#if 0


#define	MON(type, off) (*(type *)((u_int) (mg) + off))

#define RELOC(v, t)	*((t *)((u_int)&(v) + NEXT_RAMBASE))

#define	MONRELOC(type, off) (*(type *)((u_int) RELOC(mg,char *) + off))

typedef int (*getcptr)(void);
typedef int (*putcptr)(int);

void
dbj_message(char *s)
{
  MONRELOC(putcptr,MG_putc)('D');
  MONRELOC(putcptr,MG_putc)('B');
  MONRELOC(putcptr,MG_putc)('J');
  MONRELOC(putcptr,MG_putc)(':');
  while (s && *s) {
    MONRELOC(putcptr,MG_putc)(*s++);
  }
  MONRELOC(putcptr,MG_putc)('\r');
  MONRELOC(putcptr,MG_putc)('\n');
};

void
dbj_message(char *s)
{
	char *vector = (u_int)&mg + NEXT_RAMBASE;
	putcptr func = (putcptr)(*(int *)(vector + MG_putc));

	(*func)('D');
	(*func)('B');
	(*func)('J');
	(*func)(':');
  while (s && *s) {
		(*func)(*s++);
	}
	(*func)('\r');
	(*func)('\n');
}

void
dbj_check()
{
  MONRELOC(putcptr,MG_putc)('*');
  MONRELOC(putcptr,MG_putc)('\r');
  MONRELOC(putcptr,MG_putc)('\n');
}

struct dbj_trapframe {
    int dregs[8];
    int aregs[8];
    short sr;
    int pc;
    short frame;
    char info[0];
};

void
dbj_hex(v)
     unsigned long v;
{
  int i;
  MONRELOC(putcptr,MG_putc)('0');
  MONRELOC(putcptr,MG_putc)('x');
  for(i=0;i<8;i++) {
    if ((v&0xf0000000) == 0x00000000) MONRELOC(putcptr,MG_putc)('0');
    if ((v&0xf0000000) == 0x10000000) MONRELOC(putcptr,MG_putc)('1');
    if ((v&0xf0000000) == 0x20000000) MONRELOC(putcptr,MG_putc)('2');
    if ((v&0xf0000000) == 0x30000000) MONRELOC(putcptr,MG_putc)('3');
    if ((v&0xf0000000) == 0x40000000) MONRELOC(putcptr,MG_putc)('4');
    if ((v&0xf0000000) == 0x50000000) MONRELOC(putcptr,MG_putc)('5');
    if ((v&0xf0000000) == 0x60000000) MONRELOC(putcptr,MG_putc)('6');
    if ((v&0xf0000000) == 0x70000000) MONRELOC(putcptr,MG_putc)('7');
    if ((v&0xf0000000) == 0x80000000) MONRELOC(putcptr,MG_putc)('8');
    if ((v&0xf0000000) == 0x90000000) MONRELOC(putcptr,MG_putc)('9');
    if ((v&0xf0000000) == 0xa0000000) MONRELOC(putcptr,MG_putc)('a');
    if ((v&0xf0000000) == 0xb0000000) MONRELOC(putcptr,MG_putc)('b');
    if ((v&0xf0000000) == 0xc0000000) MONRELOC(putcptr,MG_putc)('c');
    if ((v&0xf0000000) == 0xd0000000) MONRELOC(putcptr,MG_putc)('d');
    if ((v&0xf0000000) == 0xe0000000) MONRELOC(putcptr,MG_putc)('e');
    if ((v&0xf0000000) == 0xf0000000) MONRELOC(putcptr,MG_putc)('f');
    v=v<<4;
  }
  MONRELOC(putcptr,MG_putc)('\r');
  MONRELOC(putcptr,MG_putc)('\n');
}

void
dbj_print(char *s)
{
  mon_putc('D');
  mon_putc('B');
  mon_putc('J');
  mon_putc(':');
  if (s) while(*s) {
    mon_putc(*s++);
  }
  mon_putc('\r');
  mon_putc('\n');
}

int
mon_getc()
{
  return(MON(getcptr,MG_getc)());
}


int
mon_putc(c)
     int c;
{
  return(MON(putcptr,MG_putc)(c));
}

#endif

void    next68k_bootargs __P((unsigned char *args[]));

/*
 * Very early initialization, before we do much.
 * Memory isn't even mapped here.
 */
#include <next68k/next68k/nextrom.h>
extern char *mg;
#define	MON(type, off) (*(type *)((u_int) (mg) + off))
#define RELOC(v, t)	*((t *)((u_int)&(v) + NEXT_RAMBASE))
#define	MONRELOC(type, off) (*(type *)((u_int) RELOC(mg,char *) + off))

extern void dbj_message(char * s);
#define DBJ_DEBUG(xs)  \
  ((*((void (*)(char *))(((void *)&dbj_message)+NEXT_RAMBASE))) \
			((xs)+NEXT_RAMBASE))

u_char rom_enetaddr[6];

void
next68k_bootargs(args)
     unsigned char *args[];
{
  RELOC(mg,char *) = args[1];

#if 0
  DBJ_DEBUG("Check serial port A for console");

  /* If we continue, we will die because the compiler
   * generates global symbols
   */
#endif

  /* Construct the segment list */
  {
    int i;
    int j = 0;
    for (i=0;i<N_SIMM;i++) {
      if ((MONRELOC(char,MG_simm+i) & SIMM_SIZE) == SIMM_SIZE_EMPTY) {
      } else {
        RELOC(phys_seg_list[j].ps_start, vm_offset_t) 
          = NEXT_RAMBASE+(i*NEXT_BANKSIZE);
      }
      if ((MONRELOC(char,MG_simm+i) & SIMM_SIZE) == SIMM_SIZE_16MB) {
        RELOC(phys_seg_list[j].ps_end, vm_offset_t) = 
          RELOC(phys_seg_list[j].ps_start, vm_offset_t) +
          (0x1000000);
        j++;
      } 
      if ((MONRELOC(char,MG_simm+i) & SIMM_SIZE) == SIMM_SIZE_4MB) {
        RELOC(phys_seg_list[j].ps_end, vm_offset_t) = 
          RELOC(phys_seg_list[j].ps_start, vm_offset_t) +
          (0x400000);
        j++;
      }
      if ((MONRELOC(char,MG_simm+i) & SIMM_SIZE) == SIMM_SIZE_1MB) {
        RELOC(phys_seg_list[j].ps_end, vm_offset_t) = 
          RELOC(phys_seg_list[j].ps_start, vm_offset_t) +
          (0x100000);
        j++;
      }
    }
    /* pmap is unhappy if it is not null terminated */
    for(;j<MAX_PHYS_SEGS;j++) {
      RELOC(phys_seg_list[j].ps_start, vm_offset_t) = 0;
      RELOC(phys_seg_list[j].ps_end, vm_offset_t) = 0;
    }
  }

  /* Read the ethernet address from rom, this should be done later
   * in device driver somehow.
   */
  {
    int i;
    for(i=0;i<6;i++) {
      RELOC(rom_enetaddr[i], u_char) = MONRELOC(u_char *, MG_clientetheraddr)[i];
    }
  }
}
