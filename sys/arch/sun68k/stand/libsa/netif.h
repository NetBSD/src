/*	$NetBSD: netif.h,v 1.1.24.1 2005/01/24 08:35:03 skrll Exp $	*/


#include "iodesc.h"

struct netif {
	void *nif_devdata;
};

ssize_t		netif_get(struct iodesc *, void *, size_t, time_t);
ssize_t		netif_put(struct iodesc *, void *, size_t);

int		netif_open(void *);
int		netif_close(int);

struct iodesc	*socktodesc(int);

