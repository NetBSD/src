LIB=netpgpbn
SRCS= bignum.c digest.c misc.c rand.c
SRCS+= stubs.c
MAN=libnetpgpbn.3
WARNS=4
CPPFLAGS+=-I${EXTDIST}/src/libcipher

INCS=bn.h digest.h
INCSDIR=/usr/include/netpgp

EXTDIST=	${.CURDIR}/../../dist
.PATH:		${EXTDIST}/src/libbn

.include <bsd.lib.mk>
