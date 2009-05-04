/*	$NetBSD: netif.h,v 1.3.78.1 2009/05/04 08:12:02 yamt Exp $	*/


#include "iodesc.h"

struct netif {
	void *nif_devdata;
};

int		netif_open(void *);
int		netif_close(int);
