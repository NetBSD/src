#	$NetBSD: Makefile,v 1.8 1999/02/13 23:53:57 lukem Exp $
# Makefile for uuto

BINDIR=		$(bindir)

CLEANFILES=	uuto

all:	${CLEANFILES}

uuto:	uuto.in
	sed -e "s|@BINDIR@|$(bindir)|g" -e "s|@SBINDIR@|$(sbindir)|g" \
		$(.CURDIR)/uuto.in > $(.TARGET)

FILES=uuto
FILESMODE=${BINMODE}

.include <bsd.prog.mk>
