LIB=netpgpcipher
SRCS= rijndael-alg-fst.c rijndael-api-fst.c rijndael.c
SRCS+= cast.c
SRCS+= modes.c
SRCS+= camellia.c
MAN=libnetpgpcipher.3
WARNS=4

EXTDIST=	${.CURDIR}/../../dist
.PATH:		${EXTDIST}/src/libcipher

INCS=aes.h rijndael.h cast.h camellia.h
INCSDIR=/usr/include/netpgp

.include <bsd.lib.mk>
