/*	$NetBSD: 3c590.c,v 1.2 1997/03/15 22:18:55 perry Exp $	*/

/* stripped down from freebsd:sys/i386/netboot/3c509.c */


/**************************************************************************
NETBOOT -  BOOTP/TFTP Bootstrap Program

Author: Martin Renters.
  Date: Mar 22 1995

 This code is based heavily on David Greenman's if_ed.c driver and
  Andres Vega Garcia's if_ep.c driver.

 Copyright (C) 1993-1994, David Greenman, Martin Renters.
 Copyright (C) 1993-1995, Andres Vega Garcia.
 Copyright (C) 1995, Serge Babkin.
  This software may be used, modified, copied, distributed, and sold, in
  both source and binary form provided that the above copyright and these
  terms are retained. Under no circumstances are the authors responsible for
  the proper functioning of this software, nor do the authors assume any
  responsibility for damages incurred with its use.

3c509 support added by Serge Babkin (babkin@hq.icb.chel.su)

3c509.c,v 1.2 1995/05/30 07:58:52 rgrimes Exp

***************************************************************************/

#include <sys/types.h>
#include <machine/pio.h>

#include <lib/libsa/stand.h>

#include <libi386.h>
#include <pcivar.h>

#include "etherdrv.h"
#include "3c509.h"

char etherdev[20];

int ether_medium;
unsigned short eth_base;

extern void epreset __P((void));
extern int ep_get_e __P((int));

u_char eth_myaddr[6];

/**************************************************************************
ETH_PROBE - Look for an adapter
***************************************************************************/
int EtherInit(myadr)
char *myadr;
{
	/* common variables */
	int i;
	/* variables for 3C509 */
	short *p;

	/*********************************************************
			Search for 3Com 590 card
	***********************************************************/

	pcihdl_t hdl;
	int iobase;

	if(pcicheck() == -1) {
	    printf("cannot access PCI\n");
	    return(0);
	}

	if(pcifinddev(0x10b7, 0x5900, &hdl)) {
	    printf("cannot find 3c590\n");
	    return(0);
	}

	if(pcicfgread(&hdl, 0x10, &iobase) || !(iobase & 1)) {
	    printf("cannot map IO space\n");
	    return(0);
	}
	eth_base = iobase & 0xfffffffc;

	ether_medium = ETHERMEDIUM_BNC; /* XXX */

	GO_WINDOW(0);

	/*
	* Read the station address from the eeprom
	*/
	p = (u_short *) eth_myaddr;
	for (i = 0; i < 3; i++) {
	  u_short help;
	  GO_WINDOW(0);
	  help = ep_get_e(i);
	  p[i] = ((help & 0xff) << 8) | ((help & 0xff00) >> 8);
	  GO_WINDOW(2);
	  outw(BASE + EP_W2_ADDR_0 + (i * 2), help);
	}
	for(i = 0; i < 6; i++) myadr[i] = eth_myaddr[i];
	epreset();

	sprintf(etherdev, "ep@pci,0x%x", eth_base);
	return(1);
}
