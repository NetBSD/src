#	$NetBSD: Makefile,v 1.7 2000/07/16 21:56:15 jdolecek Exp $

SA_PROG= ufsboot

# Regular local source
SRCS= conf.c

# Build these locally
SRCS+= ufs.c

all:	${SA_PROG}.bin

.include <bsd.prog.mk>
.PATH: ${S}/lib/libsa
DEFS+=  -DCOMPAT_UFS
