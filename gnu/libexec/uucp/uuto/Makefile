#	$NetBSD: Makefile,v 1.11 2003/10/21 10:01:20 lukem Exp $
# Makefile for uuto

BINDIR=		$(bindir)

CLEANFILES=	uuto

realall:	${CLEANFILES}

uuto:	uuto.in
	${_MKTARGET_CREATE}
	sed -e "s|@BINDIR@|$(bindir)|g" -e "s|@SBINDIR@|$(sbindir)|g" \
		$(.CURDIR)/uuto.in > $(.TARGET)

FILES=uuto
FILESMODE=${BINMODE}

.include <bsd.prog.mk>
