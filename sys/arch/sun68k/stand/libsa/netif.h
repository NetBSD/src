/*	$NetBSD: netif.h,v 1.3.74.1 2009/01/17 13:28:34 mjf Exp $	*/


#include "iodesc.h"

struct netif {
	void *nif_devdata;
};

int		netif_open(void *);
int		netif_close(int);
