#	$NetBSD: Makefile,v 1.7 1997/03/24 22:06:07 christos Exp $
# Makefile for uusched

BINDIR=		$(bindir)

NOPROG=
CLEANFILES=	uusched

all:	${CLEANFILES}

uusched:	uusched.in
	sed -e "s|@BINDIR@|$(bindir)|g" -e "s|@SBINDIR@|$(sbindir)|g" \
		$(.CURDIR)/uusched.in > $(.TARGET)

FILES=uusched
FILESMODE=${BINMODE}

.include <bsd.prog.mk>
