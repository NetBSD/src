.include "../Makefile.inc"

.PATH:	${S}/dev/i2c

KMOD=	sgp40mox
IOCONF=	sgp40mox.ioconf
SRCS=	sgp40.c
SRCS+=	sensirion_voc_algorithm.c

WARNS=	3

.include <bsd.kmodule.mk>
