LIB=netpgprsa
SRCS=rsa.c rsastubs.c
MAN=libnetpgprsa.3
WARNS=4
CPPFLAGS+=-I${EXTDIST}/src/libbn

EXTDIST=	${.CURDIR}/../../dist
.PATH:		${EXTDIST}/src/librsa

INCS=rsa.h
INCSDIR=/usr/include/netpgp

.include <bsd.lib.mk>
