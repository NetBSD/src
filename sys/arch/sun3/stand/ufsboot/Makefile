#	$NetBSD: Makefile,v 1.6.14.1 2000/11/20 20:27:56 bouyer Exp $

SA_PROG= ufsboot

# Regular local source
SRCS= conf.c

# Build these locally
SRCS+= ufs.c

all:	${SA_PROG}.bin

.include <bsd.prog.mk>
.PATH: ${S}/lib/libsa
DEFS+=  -DCOMPAT_UFS
