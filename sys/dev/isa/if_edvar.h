/*	$NetBSD: if_edvar.h,v 1.1.2.1 1997/07/30 07:05:35 marc Exp $	*/

/*
 * Device driver for National Semiconductor DS8390/WD83C690 based ethernet
 * adapters.
 *
 * Copyright (c) 1994, 1995 Charles M. Hannum.  All rights reserved.
 *
 * Copyright (C) 1993, David Greenman.  This software may be used, modified,
 * copied, distributed, and sold, in both source and binary form provided that
 * the above copyright and these terms are retained.  Under no circumstances is
 * the author responsible for the proper functioning of this software, nor does
 * the author assume any responsibility for damages incurred with its use.
 *
 * Currently supports the Western Digital/SMC 8003 and 8013 series, the SMC
 * Elite Ultra (8216), the 3Com 3c503, the NE1000 and NE2000, and a variety of
 * similar clones.
 */

/*
 * ed_softc: per line info and status
 */
struct ed_softc {
	struct	device sc_dev;
	void *sc_ih;

	struct	ethercom sc_ethercom;	/* ethernet common */

	char	*type_str;	/* pointer to type string */
	u_char	vendor;		/* interface vendor */
	u_char	type;		/* interface type code */

	bus_space_tag_t sc_iot; /* bus identifier */
	bus_space_tag_t sc_memt;
	bus_space_handle_t sc_ioh;   /* io handle */
	/* this is ISA-specific.  it's only used by the SMC card.
	   I don't know if this card has a pcmcia version.  leave
	   it here for now */
	bus_space_handle_t sc_delaybah; /* io handle for `delay port' */
	bus_space_handle_t sc_memh; /* bus memory handle */

	int nic_pcmcia_window;
	int asic_pcmcia_window;

	bus_size_t	asic_base;	/* offset of ASIC I/O port */
	bus_size_t	nic_base;	/* offset of NIC (DS8390) I/O port */

/*
 * The following 'proto' variable is part of a work-around for 8013EBT asics
 * being write-only.  It's sort of a prototype/shadow of the real thing.
 */
	u_char	wd_laar_proto;
/*
 * This `proto' variable is so we can turn MENB on and off without reading
 * the value back from the card all the time.
 */
	u_char	wd_msr_proto;
	u_char	cr_proto;	/* values always set in CR */
	u_char	isa16bit;	/* width of access to card 0=8 or 1=16 */
	u_char	is790;		/* set by probe if NIC is a 790 */

	int	mem_start;	/* offset of NIC memory */
	int	mem_end;	/* offset of NIC memory end */
	int	mem_size;	/* total NIC memory size */
	int	mem_ring;	/* offset of RX ring-buffer (in NIC mem) */

	u_char	mem_shared;	/* NIC memory is shared with host */
	u_char	txb_cnt;	/* number of transmit buffers */
	u_char	txb_inuse;	/* number of transmit buffers active */

	u_char 	txb_new;	/* pointer to where new buffer will be added */
	u_char	txb_next_tx;	/* pointer to next buffer ready to xmit */
	u_short	txb_len[8];	/* buffered xmit buffer lengths */
	u_char	tx_page_start;	/* first page of TX buffer area */
	u_char	rec_page_start;	/* first page of RX ring-buffer */
	u_char	rec_page_stop;	/* last page of RX ring-buffer */
	u_char	next_packet;	/* pointer to next unread RX packet */

	u_int8_t sc_enaddr[6];
};

void edattach __P((struct device *self));
int edintr __P((void *));
int ed_find_Novell __P((struct ed_softc *, struct cfdata *,
    bus_space_tag_t, bus_space_handle_t));

#define	NIC_PUT(t, bah, nic, reg, val)	\
	bus_space_write_1((t), (bah), ((nic) + (reg)), (val))
#define	NIC_GET(t, bah, nic, reg)	\
	bus_space_read_1((t), (bah), ((nic) + (reg)))

