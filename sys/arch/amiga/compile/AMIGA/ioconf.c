#include "sys/param.h"
#include "sys/buf.h"

#include "arch/amiga/dev/device.h"


#define C (caddr_t)
#define D (struct driver *)

extern struct driver scsidriver;
extern struct driver serdriver;
extern struct driver sddriver;
extern struct driver sddriver;
extern struct driver sddriver;
extern struct driver sddriver;
extern struct driver sddriver;
extern struct driver sddriver;
extern struct driver sddriver;
extern struct driver stdriver;
extern struct driver stdriver;
extern struct driver grfdriver;

struct amiga_ctlr amiga_cinit[] = {
/*	driver,		unit,	alive,	addr,	flags */
	{ &scsidriver,	0,	0,	C 0x10001,	0x0 },
	0
};

struct amiga_device amiga_dinit[] = {
/*driver,	cdriver,	unit,	ctlr,	slave,	addr,	dk,	flags*/
{ &serdriver,	D 0x0,		0,	-1,	-1,	C 0x10003,	0,	0x0 },
{ &sddriver,	&scsidriver,	0,	0,	0,	C 0x0,	1,	0x0 },
{ &sddriver,	&scsidriver,	1,	0,	1,	C 0x0,	1,	0x0 },
{ &sddriver,	&scsidriver,	2,	0,	2,	C 0x0,	1,	0x0 },
{ &sddriver,	&scsidriver,	3,	0,	3,	C 0x0,	1,	0x0 },
{ &sddriver,	&scsidriver,	4,	0,	4,	C 0x0,	1,	0x0 },
{ &sddriver,	&scsidriver,	5,	0,	5,	C 0x0,	1,	0x0 },
{ &sddriver,	&scsidriver,	6,	0,	6,	C 0x0,	1,	0x0 },
{ &stdriver,	&scsidriver,	0,	0,	4,	C 0x0,	0,	0x0 },
{ &stdriver,	&scsidriver,	1,	0,	5,	C 0x0,	0,	0x0 },
{ &grfdriver,	D 0x0,		0,	-1,	-1,	C 0x10007,	0,	0x0 },
0
};
