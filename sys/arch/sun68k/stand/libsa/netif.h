/*	$NetBSD: netif.h,v 1.4 2009/01/12 11:32:45 tsutsui Exp $	*/


#include "iodesc.h"

struct netif {
	void *nif_devdata;
};

int		netif_open(void *);
int		netif_close(int);
