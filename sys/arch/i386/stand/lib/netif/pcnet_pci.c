/*	$NetBSD: pcnet_pci.c,v 1.2 1997/03/15 22:20:55 perry Exp $	*/

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


#include <sys/types.h>
#include <machine/pio.h>
#include <lib/libsa/stand.h>

#include <pcivar.h>

#include "etherdrv.h"
#include "lance.h"

char etherdev[20];

int lance_rap, lance_rdp;

static pcihdl_t hdl;

u_char eth_myaddr[6];

extern void am7990_init __P((void));
extern void am7990_stop __P((void));

int EtherInit(myadr)
char *myadr;
{
  int iobase, pcicsr, i;

  if(pcicheck() == -1) {
    printf("cannot access PCI\n");
    return(0);
  }

  if(pcifinddev(0x1022, 0x2000, &hdl)) {
    printf("cannot find PCNET\n");
    return(0);
  }

  if(pcicfgread(&hdl, 0x10, &iobase) || !(iobase & 1)) {
    printf("cannot map IO space\n");
    return(0);
  }
  iobase &= 0xfffffffc;

  lance_rap = iobase + 0x12;
  lance_rdp = iobase + 0x10;

  /* make sure it's stopped */
  am7990_stop();

  /* enable bus mastering in PCI command register */
  if(pcicfgread(&hdl, 0x04, &pcicsr)
     || pcicfgwrite(&hdl, 0x04, pcicsr | 4)) {
    printf("cannot enable DMA\n");
    return(0);
  }

  for(i=0; i<6; i++)
    myadr[i] = eth_myaddr[i] = inb(iobase + i);

  am7990_init();

  sprintf(etherdev, "le@pci,0x%x", iobase);
  return(1);
}
