/*	$NetBSD: netif.h,v 1.1 2001/06/14 12:57:14 fredette Exp $	*/


#include "iodesc.h"

struct netif {
	void *nif_devdata;
};

ssize_t		netif_get __P((struct iodesc *, void *, size_t, time_t));
ssize_t		netif_put __P((struct iodesc *, void *, size_t));

int		netif_open __P((void *));
int		netif_close __P((int));

struct iodesc	*socktodesc __P((int));

