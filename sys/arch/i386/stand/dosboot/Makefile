#	$NetBSD: Makefile,v 1.33.26.1 2023/12/14 17:56:08 martin Exp $

S=	${.CURDIR}/../../../..

BASE= dosboot
PROG= ${BASE}.com
NEWVERSWHAT=	"DOS Boot"
STARTFILE=	${DOSSTART}
RELOC=		0x100

SRCS= main.c devopen.c exec.c exec_multiboot1.c exec_multiboot2.c

CPPFLAGS+= -DSLOW	# for libz
CPPFLAGS+= -DCOMPAT_386BSD_MBRPART
CPPFLAGS+= -DXMS
CPPFLAGS+= -DLIBSA_ENABLE_LS_OP
CPPFLAGS+= -DNO_MULTIBOOT2 # keep the binary small
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
SAMISCMAKEFLAGS= SA_USE_CREAD=yes SA_INCLUDE_NET=no SA_ENABLE_LS_OP=yes
I386MISCMAKEFLAGS= I386_INCLUDE_DOS=yes

# DOS command line arguments are located at 0x0000.
COPTS.doscommain.c+= -fno-delete-null-pointer-checks

# Do not page-align data segment.
LDFLAGS+= -Wl,-N

VERSIONFILE= ${.CURDIR}/version

.include <bsd.init.mk>

release: check_RELEASEDIR
	${HOST_INSTALL_FILE} -m ${BINMODE} dosboot.com \
	    ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/misc

.include "../Makefile.booters"
