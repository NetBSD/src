#	$NetBSD: Makefile,v 1.10 2003/10/20 00:40:53 lukem Exp $
# Makefile for uusched

BINDIR=		$(bindir)

CLEANFILES=	uusched

realall:	${CLEANFILES}

uusched:	uusched.in
	${_MKMSGCREATE}
	${_MKCMD}\
	sed -e "s|@BINDIR@|$(bindir)|g" -e "s|@SBINDIR@|$(sbindir)|g" \
		$(.CURDIR)/uusched.in > $(.TARGET)

FILES=uusched
FILESMODE=${BINMODE}

.include <bsd.prog.mk>
