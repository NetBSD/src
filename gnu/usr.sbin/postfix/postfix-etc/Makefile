#	$NetBSD: Makefile,v 1.1 2000/04/30 21:22:08 perry Exp $

# This Makefile is called only by src/etc, not by usr.sbin/postfix/Makefile

DIST=	${.CURDIR}/../../../dist/postfix/conf
.PATH:	${DIST}

BINDIR=	${PFIX_ETCDIR}

# These two .cf files are the only ones absolutely needed. It is not
# clear if we should be installing things like the transport and
# virtual file, etc. I contend we shouldn't, since they contain no
# real information at all, just the man page sources, and are not
# needed by default.

FILES=	main.cf master.cf README

.include <bsd.prog.mk>
