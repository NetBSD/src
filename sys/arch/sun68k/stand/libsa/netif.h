/*	$NetBSD: netif.h,v 1.2 2005/01/22 15:36:11 chs Exp $	*/


#include "iodesc.h"

struct netif {
	void *nif_devdata;
};

ssize_t		netif_get(struct iodesc *, void *, size_t, time_t);
ssize_t		netif_put(struct iodesc *, void *, size_t);

int		netif_open(void *);
int		netif_close(int);

struct iodesc	*socktodesc(int);

