#	$NetBSD: Makefile.inc,v 1.12 2001/11/12 23:16:24 tv Exp $

.include <bsd.own.mk>

# Override these to avoid use of host compiler, etc.
INSTALL=	${TOOLDIR}/bin/binstall
MKDEP=		CC=${HOST_CC:Q} ${TOOLDIR}/bin/mkdep
