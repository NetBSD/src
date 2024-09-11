#	$NetBSD: Makefile,v 1.39 2024/09/11 20:15:36 andvar Exp $

S=	${.CURDIR}/../../../..

BASE= dosboot
PROG= ${BASE}.com
NEWVERSWHAT=	"DOS Boot"
STARTFILE=	start_dos.o doscommain.o
RELOC=		0x100

SRCS=	main.c devopen.c exec.c
SRCS+=	dosfile.c dos_file.S
SRCS+=	exec_multiboot1.c exec_multiboot2.c

#CPPFLAGS+= -DSLOW	# for libz; no longer available
CPPFLAGS+= -DCOMPAT_386BSD_MBRPART
CPPFLAGS+= -DLIBSA_ENABLE_LS_OP
CPPFLAGS+= -DNO_MULTIBOOT2 # keep the binary small
#uncomment if there are problems with memory detection
#CPPFLAGS+= -DCONSERVATIVE_MEMDETECT

#CFLAGS= -O2 -fomit-frame-pointer -fno-defer-pop
CFLAGS+= -Wall -Wmissing-prototypes -Wstrict-prototypes -Wno-main

.PATH: ${.CURDIR}/../libsa

# XXX these should depend on the size of the image
CPPFLAGS+= -DSTACK_START=0x10000
SAMISCCPPFLAGS+= -DHEAP_START=0x20000 -DHEAP_LIMIT=0x50000
SAMISCMAKEFLAGS= SA_USE_CREAD=yes SA_INCLUDE_NET=no SA_ENABLE_LS_OP=yes

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
