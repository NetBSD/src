#	$NetBSD: Makefile,v 1.8 2019/10/13 07:28:21 mrg Exp $
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

COPTS.sliplogin.c+=	${GCC_NO_FORMAT_TRUNCATION}

.include <bsd.prog.mk>
