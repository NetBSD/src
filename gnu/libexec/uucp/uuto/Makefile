#	$NetBSD: Makefile,v 1.7 1997/03/24 22:06:08 christos Exp $
# Makefile for uuto

BINDIR=		$(bindir)

NOPROG=
CLEANFILES=	uuto

all:	${CLEANFILES}

uuto:	uuto.in
	sed -e "s|@BINDIR@|$(bindir)|g" -e "s|@SBINDIR@|$(sbindir)|g" \
		$(.CURDIR)/uuto.in > $(.TARGET)

FILES=uuto
FILESMODE=${BINMODE}

.include <bsd.prog.mk>
