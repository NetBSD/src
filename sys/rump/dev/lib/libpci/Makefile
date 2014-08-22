#	$NetBSD: Makefile,v 1.4 2014/08/22 14:28:58 pooka Exp $
#

RUMPTOP= ${TOPRUMP}

.PATH:	${RUMPTOP}/../dev/pci

LIB=	rumpdev_pci
IOCONF=	PCI.ioconf

SRCS+=	pci.c pci_map.c pci_quirks.c pci_subr.c pci_stub.c pci_usrreq.c
SRCS+=	pcibusprint.c

SRCS+=	rumpdev_pci.c

# ok, these don't _really_ belong here, but it's the only
# place they're currently useful, so let it slide
SRCS+=	rumpdev_bus_space.c
SRCS+=	rumpdev_bus_dma.c

SRCS+=	pci_at_mainbus.c

CPPFLAGS+= -I${.CURDIR}/opt -I${RUMPTOP}/librump/rumpkern
CPPFLAGS+= -I${RUMPTOP}/librump/rumpvfs

.if ${RUMP_PCI_IOSPACE:Uno} == "yes"
CPPFLAGS+=-DRUMP_PCI_IOSPACE
.endif

.if defined(RUMP_PCI_USER)
RUMPCOMP_USER_SRCS=	${RUMP_PCI_USER}
RUMPCOMP_INCS_DIR:=	${.PARSEDIR}
RUMPCOMP_USER_CPPFLAGS=-I${RUMPCOMP_INCS_DIR}
.endif

.include "${RUMPTOP}/Makefile.rump"
.include <bsd.lib.mk>
.include <bsd.klinks.mk>
