/* 
 * Copyright (c) 1993 Philip A. Nelson.
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
 *	This product includes software developed by Philip A. Nelson.
 * 4. The name of Philip A. Nelson may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY PHILIP NELSON ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL PHILIP NELSON BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *	con_putc.c
 *
 *	$Id: con_putc.c,v 1.1.1.1 1993/09/09 23:53:47 phil Exp $
 */
/*
 * Small "console" driver for initial use in 532bsd.
 *
 * Phil Nelson
 *
 * Needs work.  i.e. make the console use the tty driver!
 * 
 */

/* #define DEBUG */
#define WR_ADR(adr,val)	(*((volatile unsigned char *)(adr))=(val))
#define RD_ADR(adr)	(*((volatile unsigned char *)(adr)))

#define RS_CONSOLE 		   0	/* minor number of console */

#define RS_CON_MAP_STAT	  0xFFC80001	/* raw addresses for console */
#define RS_CON_MAP_DATA	  0xFFC80003
#define RS_CON_ISR_ADR	  0xFFC80005

#define RS_CON_STAT	  0x28000001	/* raw addresses for console */
#define RS_CON_DATA	  0x28000003

#define SR_RX_RDY	0x01
#define SR_TX_RDY	0x04

/* A simple "more" for debugging output. */
#ifdef DEBUG
int ___lines = 0;
#else
int ___lines = -1;
#endif

int _mapped = 0;

/* A polled getc! */
char
cngetc()
{
   char c;
   int x=splhigh();
   while (0 == (RD_ADR (RS_CON_MAP_STAT) & SR_RX_RDY));
   c = RD_ADR(RS_CON_MAP_DATA);
   splx(x);
   return c;
}

/* A polled putc for the console terminal. */
cnputc (char c)
{
/*  int x=splhigh(); */

  if (c == '\n') cnputc('\r');
  if (_mapped) {
    while (0 == (RD_ADR (RS_CON_MAP_STAT) & SR_TX_RDY));
    WR_ADR (RS_CON_MAP_DATA, c);
    while (0 == (RD_ADR (RS_CON_MAP_STAT) & SR_TX_RDY));
  } else {
    while (0 == (RD_ADR (RS_CON_STAT) & SR_TX_RDY));
    WR_ADR (RS_CON_DATA, c);
    while (0 == (RD_ADR (RS_CON_STAT) & SR_TX_RDY));
  }
  if (c == '\n' && ___lines >= 0)
    {
     if (++___lines == 22) {
	___lines = 0;
	cnputc('m');cnputc('o');cnputc('r');cnputc('e');
	cnputc(':');cnputc(' ');
	cngetc();
	cnputc('\n');
     }
  }
  RD_ADR(RS_CON_ISR_ADR);  
/*  splx(x); */
}

int
consinit()
{
}

