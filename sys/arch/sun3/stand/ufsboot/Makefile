#	$NetBSD: Makefile,v 1.6 1998/07/01 22:56:45 gwr Exp $

SA_PROG= ufsboot

# Using local rule for this
SRCxx= ${.CURDIR}/../libsa/xxboot.c

# Regular local source
SRC1= conf.c

# Build these locally too
SRC2= ufs.c

SRCS= ${SRCxx} ${SRC1} ${SRC2}
OBJS= boot.o ${SRC1:.c=.o} ${SRC2:.c=.o}

all:	${SA_PROG}.bin

.include <bsd.prog.mk>
.PATH: ${S}/lib/libsa
DEFS+=  -DCOMPAT_UFS

# This file is common to netboot and ufsboot,
# but gets a define so it knows which it is.
boot.o : ${SRCxx}
	${COMPILE.c} -DXX='"ufs"' -o $@ ${SRCxx}

