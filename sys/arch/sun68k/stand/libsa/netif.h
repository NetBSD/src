/*	$NetBSD: netif.h,v 1.3 2005/12/11 12:19:29 christos Exp $	*/


#include "iodesc.h"

struct netif {
	void *nif_devdata;
};

ssize_t		netif_get(struct iodesc *, void *, size_t, time_t);
ssize_t		netif_put(struct iodesc *, void *, size_t);

int		netif_open(void *);
int		netif_close(int);

struct iodesc	*socktodesc(int);

