#	$NetBSD: Makefile,v 1.2 1999/12/12 15:21:27 fredb Exp $

DESTDIR=	${RELEASEDIR}
FILES=		INSTALL.html INSTALL.more INSTALL.ps INSTALL.txt

.if !defined(RELEASEDIR)
install:
	@echo "You must set RELEASEDIR before doing that!"
.endif

release: install

.include <bsd.files.mk>
.include <bsd.prog.mk>
