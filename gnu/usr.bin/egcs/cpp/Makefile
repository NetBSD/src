#	from: @{#}Makefile	6.4 (Berkeley) 2/21/91
#	$NetBSD: Makefile,v 1.10 1999/01/18 21:14:50 christos Exp $

HAVE_GCC28!=	${CXX} --version | egrep "^(2\.8|egcs)" ; echo

PROG=		cpp
BINDIR=		/usr/libexec
SRCS=		cccp.c cexp.y obstack.c prefix.c version.c 
CPPFLAGS+=	-DGCC_INCLUDE_DIR=\"\" \
		-DGPLUSPLUS_INCLUDE_DIR=\"/usr/include/g++\" -DLINTCOMMENT
.if empty(HAVE_GCC28)
CWARNFLAGS+=	-Wno-error
.endif

LINKS=		/usr/bin/cpp /usr/bin/gcpp
FILES=		usr.bin.cpp.sh
FILESNAME=	cpp
FILESDIR=	/usr/bin
FILESMODE=	${BINMODE}

MAN=		cccp.1
MLINKS=		cccp.1 cpp.1 cccp.1 gcpp.1

.NOPATH: cexp.c
.BEGIN:
	@rm -f ${DIST}/cexp.c

.include <bsd.prog.mk>
