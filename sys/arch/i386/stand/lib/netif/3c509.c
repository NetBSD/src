/*	$NetBSD: 3c509.c,v 1.1.1.1 1997/03/14 02:40:33 perry Exp $	*/

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

#include "etherdrv.h"
#include "3c509.h"

char etherdev[20];

char bnc=0, utp=0; /* for 3C509 */
unsigned short eth_base;

extern void epreset __P((void));
extern int ep_get_e __P((int));

static int send_ID_sequence __P((int));
static int get_eeprom_data __P((int, int));

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
	int data, j, id_port = EP_ID_PORT;
	u_short k;
/*	int ep_current_tag = EP_LAST_TAG + 1; */
	short *p;

	/*********************************************************
			Search for 3Com 509 card
	***********************************************************/
/*
	ep_current_tag--;
*/

	/* Look for the ISA boards. Init and leave them actived */
	/* search for the first card, ignore all others */
	outb(id_port, 0xc0);	/* Global reset */
	delay(1000);
/*
	for (i = 0; i < EP_MAX_BOARDS; i++) {
*/
		outb(id_port, 0);
		outb(id_port, 0);
		send_ID_sequence(id_port);

		data = get_eeprom_data(id_port, EEPROM_MFG_ID);
		if (data != MFG_ID)
			return 0;

		/* resolve contention using the Ethernet address */
		for (j = 0; j < 3; j++)
			data = get_eeprom_data(id_port, j);

		eth_base =
		    (get_eeprom_data(id_port, EEPROM_ADDR_CFG) & 0x1f) * 0x10 + 0x200;
		outb(id_port, EP_LAST_TAG);	/* tags board */
		outb(id_port, ACTIVATE_ADAPTER_TO_CONFIG);
/*
		ep_current_tag--;
		break;
	}

	if(i==EP_MAX_BOARDS)return 0;
*/

	/*
	* The iobase was found and MFG_ID was 0x6d50. PROD_ID should be
	* 0x9[0-f]50
	*/
	GO_WINDOW(0);
	k = ep_get_e(EEPROM_PROD_ID);
	if ((k & 0xf0ff) != (PROD_ID & 0xf0ff))
		return 0;

	printf("3C5x9 board on ISA at 0x%x - ", eth_base);

	/* test for presence of connectors */
	i = inw(IS_BASE + EP_W0_CONFIG_CTRL);
	j = inw(IS_BASE + EP_W0_ADDRESS_CFG) >> 14;

	switch(j) {
		case 0:
			if(i & IS_UTP) {
				printf("10baseT\r\n");
				utp=1;
				}
			else {
				printf("10baseT not present\r\n");
				return 0;
				}

			break;
/*
		case 1:
			if(i & IS_AUI)
				printf("10base5\r\n");
			else {
				printf("10base5 not present\r\n");
				return 0;
				}

			break;
*/
		case 3:
			if(i & IS_BNC) {
				printf("10base2\r\n");
				bnc=1;
				}
			else {
				printf("10base2 not present\r\n");
				return 0;
				}

			break;
		default:
			printf("unknown connector\r\n");
		  return 0;
		}
	/*
	* Read the station address from the eeprom
	*/
	p = (u_short *) eth_myaddr;
	for (i = 0; i < 3; i++) {
	  u_short help;
	  GO_WINDOW(0);
	  help=ep_get_e(i);
	  p[i] = ((help & 0xff) << 8) | ((help & 0xff00) >> 8);
	  GO_WINDOW(2);
	  outw(BASE + EP_W2_ADDR_0 + (i * 2), help);
	}
	for(i = 0; i < 6; i++) myadr[i] = eth_myaddr[i];
	epreset();

	sprintf(etherdev, "ep@isa,0x%x", eth_base);
	return(1);
}

static int
send_ID_sequence(port)
int port;
{
	int cx, al;

	for (al = 0xff, cx = 0; cx < 255; cx++) {
		outb(port, al);
		al <<= 1;
		if (al & 0x100)
			al ^= 0xcf;
	}
	return (1);
}

/*
 * We get eeprom data from the id_port given an offset into the eeprom.
 * Basically; after the ID_sequence is sent to all of the cards; they enter
 * the ID_CMD state where they will accept command requests. 0x80-0xbf loads
 * the eeprom data.  We then read the port 16 times and with every read; the
 * cards check for contention (ie: if one card writes a 0 bit and another
 * writes a 1 bit then the host sees a 0. At the end of the cycle; each card
 * compares the data on the bus; if there is a difference then that card goes
 * into ID_WAIT state again). In the meantime; one bit of data is returned in
 * the AX register which is conveniently returned to us by inb().  Hence; we
 * read 16 times getting one bit of data with each read.
 */
static int
get_eeprom_data(id_port, offset)
int id_port;
int offset;
{
	int i, data = 0;
	outb(id_port, 0x80 + offset);
	delay(1000);
	for (i = 0; i < 16; i++)
		data = (data << 1) | (inw(id_port) & 1);
	return (data);
}
