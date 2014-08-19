#	$NetBSD: Makefile,v 1.33.18.1 2014/08/20 00:03:14 tls Exp $

S!= cd ${.CURDIR}/../../../..; pwd

# DBG= -DDEBUG -DNETIF_DEBUG -DNFS_DEBUG -DRPC_DEBUG \
# 	-DNET_DEBUG -DRARP_DEBUG -DETHER_DEBUG

DEFS= ${DBG} #-fno-defer-pop

CPPFLAGS+= -DSUPPORT_BOOTP -DSUPPORT_DHCP
CPPFLAGS+= -DSUPPORT_RARP -DSUPPORT_BOOTPARAM
CPPFLAGS+= -DSUPPORT_NFS -DNFS_NOSYMLINK

.include "../Makefile.booters"

# only needed during build
libinstall::

.include <bsd.prog.mk>

### find out what to use for libsa
SA_AS= library
SAMISCMAKEFLAGS= SA_USE_LOADFILE=yes SA_EXTRADIR=${.CURDIR}
.include "${S}/lib/libsa/Makefile.inc"
LIBSA= ${SALIB}

### find out what to use for libkern
KERN_AS= library
.include "${S}/lib/libkern/Makefile.inc"
LIBKERN= ${KERNLIB}

LIBS= ${LIBSA} ${LIBKERN}

CLEANFILES+= SRT0.o

all realall: ${LIBS} SRT0.o

cleandir distclean: .WAIT cleanlibdir

cleanlibdir:
	-rm -rf lib
