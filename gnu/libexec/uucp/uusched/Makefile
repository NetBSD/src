#	$NetBSD: Makefile,v 1.9 2000/01/23 22:13:54 mycroft Exp $
# Makefile for uusched

BINDIR=		$(bindir)

CLEANFILES=	uusched

realall:	${CLEANFILES}

uusched:	uusched.in
	sed -e "s|@BINDIR@|$(bindir)|g" -e "s|@SBINDIR@|$(sbindir)|g" \
		$(.CURDIR)/uusched.in > $(.TARGET)

FILES=uusched
FILESMODE=${BINMODE}

.include <bsd.prog.mk>
