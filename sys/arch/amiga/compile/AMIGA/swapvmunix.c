#include "sys/param.h"
#include "sys/conf.h"

dev_t	rootdev = makedev(4, 48);
dev_t	dumpdev = makedev(4, 49);

struct	swdevt swdevt[] = {
	{ makedev(4, 49),	0,	0 },	/* sd6b */
	{ 0, 0, 0 }
};
