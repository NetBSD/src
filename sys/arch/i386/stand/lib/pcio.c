/*	$NetBSD: pcio.c,v 1.2 1997/03/15 22:15:49 perry Exp $	*/

/*
 * Copyright (c) 1996
 *	Matthias Drochner.  All rights reserved.
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
 *	This product includes software developed for the NetBSD Project
 *	by Matthias Drochner.
 * 4. The name of the author may not be used to endorse or promote products
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
 *
 */

/* console I/O
 needs lowlevel routines from conio.S and comio.S
*/

#include <lib/libsa/stand.h>

#include "libi386.h"

extern void conputc __P((int));
extern int congetc __P((void));
extern int coniskey __P((void));

#ifdef SUPPORT_SERIAL
extern int computc __P((int, int));
extern int comgetc __P((int));
extern int comstatus __P((int));

static int iodev;
#endif

static char *iodevname[] = {
    "pc",
#ifdef SUPPORT_SERIAL
    "com0",
    "com1"
#endif
};

char *initio(dev)
int dev;
{
#ifdef SUPPORT_SERIAL
  switch(dev){
    case CONSDEV_AUTO:
      /* serial console must have hardware handshake!
       check: 1. character output without error
              2. status bits for modem ready set
       (status seems only useful after character output)
       */
      cominit(0);
      if(!(computc(' ', 0) & 0x80) && (comstatus(0) & 0x00b0))
	dev = CONSDEV_COM0;
      else {
	cominit(1);
	if(!(computc(' ', 1) & 0x80) && (comstatus(1) & 0x00b0))
	  dev = CONSDEV_COM1;
	else
	  dev = CONSDEV_PC;
      }
      break;
    case CONSDEV_COM0: cominit(0); break;
    case CONSDEV_COM1: cominit(1); break;
    default: dev = CONSDEV_PC; break;
  }
  return(iodevname[iodev = dev]);
#else
  return(iodevname[0]);
#endif
}

static inline void internal_putchar(c)
int c;
{
#ifdef SUPPORT_SERIAL
  switch(iodev){
    case CONSDEV_PC:
#endif
      conputc(c);
#ifdef SUPPORT_SERIAL
      break;
    case CONSDEV_COM0: computc(c, 0); break;
    case CONSDEV_COM1: computc(c, 1); break;
  }
#endif
}

void putchar(c)
int c;
{
  if(c=='\n')
    internal_putchar('\r');
  internal_putchar(c);
}

int getchar(){
#ifdef SUPPORT_SERIAL
  switch(iodev){
    case CONSDEV_PC:
#endif
      return(congetc());
#ifdef SUPPORT_SERIAL
    case CONSDEV_COM0: return(comgetc(0));
    case CONSDEV_COM1: return(comgetc(1));
  }
#endif
}

int iskey(){
#ifdef SUPPORT_SERIAL
  switch(iodev){
    case CONSDEV_PC:
#endif
      return(coniskey());
#ifdef SUPPORT_SERIAL
    case CONSDEV_COM0: return(!!(comstatus(0) & 0x0100));
    case CONSDEV_COM1: return(!!(comstatus(1) & 0x0100));
  }
#endif
}
