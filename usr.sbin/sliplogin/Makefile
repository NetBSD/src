#	$NetBSD: Makefile,v 1.6.14.1 2018/11/26 01:52:55 pgoyette Exp $
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

.include <bsd.prog.mk>
