#	$NetBSD: Makefile,v 1.10 2003/10/20 00:40:53 lukem Exp $
# Makefile for uuto

BINDIR=		$(bindir)

CLEANFILES=	uuto

realall:	${CLEANFILES}

uuto:	uuto.in
	${_MKMSGCREATE}
	${_MKCMD}\
	sed -e "s|@BINDIR@|$(bindir)|g" -e "s|@SBINDIR@|$(sbindir)|g" \
		$(.CURDIR)/uuto.in > $(.TARGET)

FILES=uuto
FILESMODE=${BINMODE}

.include <bsd.prog.mk>
