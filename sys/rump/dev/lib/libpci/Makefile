#	$NetBSD: Makefile,v 1.3.8.3 2017/12/03 11:39:08 jdolecek Exp $
#

RUMPTOP= ${TOPRUMP}

.include <bsd.own.mk>

.PATH:	${RUMPTOP}/../dev/pci

LIB=	rumpdev_pci
COMMENT=PCI bus support

IOCONF=	PCI.ioconf

SRCS+=	pci.c pci_map.c pci_quirks.c pci_subr.c pci_stub.c pci_usrreq.c
SRCS+=	pcibusprint.c

SRCS+=	rumpdev_pci.c

# ok, these don't _really_ belong here, but it's the only
# place they're currently useful, so let it slide
SRCS+=	rumpdev_bus_space.c
SRCS+=	rumpdev_bus_dma.c

SRCS+=	pci_at_mainbus.c

.if ${RUMP_PCI_IOSPACE:Uno} == "yes"
.error RUMP_PCI_IOSPACE defined in Makefile.  Use userfeatures.h instead.
.endif

RUMPCOMP_USER_CPPFLAGS:=-I${.PARSEDIR}

# current state-of-the-art interface (remains to be seen if it's "good enuf")
.ifdef RUMPCOMP_MAKEFILEINC_rumpdev_pci
.include "${RUMPCOMP_MAKEFILEINC_rumpdev_pci}"
.endif

# old-style ("compat") interfaces.  will go away some day or year.
# why?  the "protocol" requires passing variables via the env, and
# the names include dots which POSIX does not, strictly speaking,
# allow in env names.  There is at least one shell (dash >= 0.58)
# which refuses to pass variables with dots.
.if defined(RUMPCOMP_USER_SRCS.rumpdev_pci) \
    || defined(RUMPCOMP_MAKEFILEINC.rumpdev_pci)
.warning Use new style RUMPCOMP_MAKEFILEINC_rumpdev_pci
.endif
.ifdef RUMPCOMP_USER_PATH.rumpdev_pci
.PATH:			${RUMPCOMP_USER_PATH.rumpdev_pci}
RUMPCOMP_USER_SRCS=	${RUMPCOMP_USER_SRCS.rumpdev_pci}
RUMPCOMP_USER_CPPFLAGS+=${RUMPCOMP_USER_CPPFLAGS.rumpdev_pci}
RUMPCOMP_USER_CFLAGS=	${RUMPCOMP_USER_CFLAGS.rumpdev_pci}
CPPFLAGS+=		${RUMPCOMP_CPPFLAGS.rumpdev_pci}
.endif
.ifdef RUMPCOMP_MAKEFILEINC.rumpdev_pci
.include "${RUMPCOMP_MAKEFILEINC.rumpdev_pci}"
.endif

# XXX: messy
.undef RUMPKERN_ONLY

.include "${RUMPTOP}/Makefile.rump"
.include <bsd.lib.mk>
.include <bsd.klinks.mk>
