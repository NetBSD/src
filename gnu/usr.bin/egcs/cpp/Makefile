#	from: @{#}Makefile	6.4 (Berkeley) 2/21/91
#	$NetBSD: Makefile,v 1.12 2000/08/06 16:37:57 mrg Exp $

.include <bsd.own.mk>

.if (${MACHINE_ARCH} != "sparc64")
HAVE_GCC28!=	${CXX} --version | egrep "^(2\.8|egcs)" ; echo

PROG=		cpp
BINDIR=		/usr/libexec
SRCS=		cccp.c cexp.y obstack.c prefix.c version.c 
CPPFLAGS+=	-DGCC_INCLUDE_DIR=\"\" \
		-DGPLUSPLUS_INCLUDE_DIR=\"/usr/include/g++\" -DLINTCOMMENT
.if empty(HAVE_GCC28)
CWARNFLAGS+=	-Wno-error
.endif

.NOPATH: cexp.c
.BEGIN:
	@rm -f ${DIST}/cexp.c

.endif

LINKS=		/usr/bin/cpp /usr/bin/gcpp
FILES=		usr.bin.cpp.sh
FILESNAME=	cpp
FILESDIR=	/usr/bin
FILESMODE=	${BINMODE}

.include <bsd.prog.mk>

MAN=		cccp.1
MLINKS=		cccp.1 cpp.1 cccp.1 gcpp.1
