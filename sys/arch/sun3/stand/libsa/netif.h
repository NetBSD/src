
#include "iodesc.h"

struct netif {
	void *devdata;
};

int		netif_get __P((struct iodesc *, void *, int, time_t));
int		netif_put __P((struct iodesc *, void *, int));

int		netif_open __P((void *));
int		netif_close __P((int));

struct iodesc	*socktodesc __P((int));

