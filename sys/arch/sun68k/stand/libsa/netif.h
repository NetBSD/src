/*	$NetBSD: netif.h,v 1.3.86.1 2009/01/19 13:16:57 skrll Exp $	*/


#include "iodesc.h"

struct netif {
	void *nif_devdata;
};

int		netif_open(void *);
int		netif_close(int);
