#	$NetBSD: Makefile,v 1.8 1999/02/13 23:53:57 lukem Exp $
# Makefile for uusched

BINDIR=		$(bindir)

CLEANFILES=	uusched

all:	${CLEANFILES}

uusched:	uusched.in
	sed -e "s|@BINDIR@|$(bindir)|g" -e "s|@SBINDIR@|$(sbindir)|g" \
		$(.CURDIR)/uusched.in > $(.TARGET)

FILES=uusched
FILESMODE=${BINMODE}

.include <bsd.prog.mk>
