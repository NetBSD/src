#	$NetBSD: Makefile,v 1.20.2.2 2002/02/17 20:03:10 thorpej Exp $

S=	${.CURDIR}/../../../../

BASE= dosboot
PROG= ${BASE}.com
NOMAN=	# defined
NEWVERSWHAT=	"DOS Boot"
STARTFILE=	${DOSSTART}
RELOC=		0x100

SRCS= main.c devopen.c exec.c

CPPFLAGS+= -DCOMPAT_386BSD_MBRPART
CPPFLAGS+= -DXMS
#uncomment if there are problems with memory detection
#CPPFLAGS+= -DCONSERVATIVE_MEMDETECT

#CFLAGS= -O2 -fomit-frame-pointer -fno-defer-pop
CFLAGS+= -Wall -Wmissing-prototypes -Wstrict-prototypes -Wno-main

# XXX should go into library
SRCS+= getopt.c
.PATH: ${.CURDIR}/../libsa

SAMISCCPPFLAGS+= -DHEAP_START=0x10000 -DHEAP_LIMIT=0x40000
SAMISCMAKEFLAGS= SA_USE_CREAD=yes SA_INCLUDE_NET=no
I386MISCMAKEFLAGS= I386_INCLUDE_DOS=yes

VERSIONFILE= ${.CURDIR}/version

.include <bsd.own.mk>

.ifndef RELEASEDIR
release:
	@echo setenv RELEASEDIR first
	@false
.else
release:
	cp dosboot.com ${RELEASEDIR}/installation/misc
.endif

.include "../Makefile.booters"
