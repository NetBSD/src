/*	$NetBSD: if_aevar.h,v 1.8 1997/04/29 04:40:26 scottr Exp $	*/

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
 */

int	ae_size_card_memory __P((bus_space_tag_t, bus_space_handle_t, int));
int	ae_test_mem __P((struct dp8390_softc *));
int	ae_write_mbuf __P((struct dp8390_softc *, struct mbuf *, int));
