#	$NetBSD: Makefile,v 1.9 2000/01/23 22:13:55 mycroft Exp $
# Makefile for uuto

BINDIR=		$(bindir)

CLEANFILES=	uuto

realall:	${CLEANFILES}

uuto:	uuto.in
	sed -e "s|@BINDIR@|$(bindir)|g" -e "s|@SBINDIR@|$(sbindir)|g" \
		$(.CURDIR)/uuto.in > $(.TARGET)

FILES=uuto
FILESMODE=${BINMODE}

.include <bsd.prog.mk>
