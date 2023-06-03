#	$NetBSD: Makefile,v 1.9 2023/06/03 09:09:23 lukem Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 7/19/93

USE_FORT?= yes	# setuid
PROG=	sliplogin
MAN=	sliplogin.8
BINOWN=	root
BINMODE=4555

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
FILESDIR=	/usr/share/examples/slip
FILES=	slip.login
.endif

COPTS.sliplogin.c+=	${CC_WNO_FORMAT_TRUNCATION}

.include <bsd.prog.mk>
