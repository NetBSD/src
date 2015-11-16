#	$NetBSD: Makefile,v 1.6 2015/11/16 23:27:07 pooka Exp $
#

RUMPTOP=${TOPRUMP}

.PATH:	${RUMPTOP}/../dev/pci ${RUMPTOP}/../dev/mii

LIB=	rumpdev_miiphy
COMMENT=MII and PHY drivers (for networking)

IOCONF=	MIIPHY.ioconf
RUMP_COMPONENT=ioconf

SRCS+= mii.c mii_ethersubr.c mii_physubr.c ukphy.c ukphy_subr.c
SRCS+= acphy.c amhphy.c atphy.c bmtphy.c brgphy.c ciphy.c dmphy.c etphy.c \
    exphy.c gentbi.c glxtphy.c gphyter.c icsphy.c igphy.c ihphy.c ikphy.c \
    inphy.c iophy.c lxtphy.c makphy.c nsphy.c nsphyter.c pnaphy.c \
    qsphy.c rdcphy.c rgephy.c rlphy.c sqphy.c tlphy.c tqphy.c urlphy.c

CPPFLAGS+= -I${RUMPTOP}/librump/rumpkern
   
.include "${RUMPTOP}/Makefile.rump"
.include <bsd.lib.mk>
.include <bsd.klinks.mk>
