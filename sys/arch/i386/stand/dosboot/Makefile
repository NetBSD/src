#	$NetBSD: Makefile,v 1.24.8.2 2009/11/01 13:58:36 jym Exp $

S=	${.CURDIR}/../../../..

BASE= dosboot
PROG= ${BASE}.com
NOMAN=	# defined
NEWVERSWHAT=	"DOS Boot"
STARTFILE=	${DOSSTART}
RELOC=		0x100

SRCS= main.c devopen.c exec.c

CPPFLAGS+= -DSLOW	# for libz
CPPFLAGS+= -DCOMPAT_386BSD_MBRPART
CPPFLAGS+= -DXMS
#uncomment if there are problems with memory detection
#CPPFLAGS+= -DCONSERVATIVE_MEMDETECT

#CFLAGS= -O2 -fomit-frame-pointer -fno-defer-pop
CFLAGS+= -Wall -Wmissing-prototypes -Wstrict-prototypes -Wno-main

# XXX should go into library
SRCS+= getopt.c
.PATH: ${.CURDIR}/../libsa

# XXX these should depend on the size of the image
CPPFLAGS+= -DSTACK_START=0x10000
SAMISCCPPFLAGS+= -DHEAP_START=0x20000 -DHEAP_LIMIT=0x50000
SAMISCMAKEFLAGS= SA_USE_CREAD=yes SA_INCLUDE_NET=no
I386MISCMAKEFLAGS= I386_INCLUDE_DOS=yes

VERSIONFILE= ${.CURDIR}/version

PIE_CFLAGS=
PIE_LDFLAGS=
PIE_AFLAGS=

.include <bsd.own.mk>

release: check_RELEASEDIR
	${HOST_INSTALL_FILE} -m ${BINMODE} dosboot.com \
	    ${RELEASEDIR}/${MACHINE}/installation/misc

.include "../Makefile.booters"
