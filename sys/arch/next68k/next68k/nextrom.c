/*	$NetBSD: nextrom.c,v 1.1.1.1 1998/06/09 07:53:06 dbj Exp $	*/
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

#include <next68k/next68k/nextrom.h>


int mon_getc(void);
int mon_putc(int c);

char *mg;

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
  if (s) while(*s) {
    MONRELOC(putcptr,MG_putc)(*s++);
  }
  MONRELOC(putcptr,MG_putc)('\r');
  MONRELOC(putcptr,MG_putc)('\n');
};

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
