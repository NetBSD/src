#	$NetBSD: Makefile,v 1.11 2003/10/21 10:01:20 lukem Exp $
# Makefile for uusched

BINDIR=		$(bindir)

CLEANFILES=	uusched

realall:	${CLEANFILES}

uusched:	uusched.in
	${_MKTARGET_CREATE}
	sed -e "s|@BINDIR@|$(bindir)|g" -e "s|@SBINDIR@|$(sbindir)|g" \
		$(.CURDIR)/uusched.in > $(.TARGET)

FILES=uusched
FILESMODE=${BINMODE}

.include <bsd.prog.mk>
