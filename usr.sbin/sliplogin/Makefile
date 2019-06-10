#	$NetBSD: Makefile,v 1.6.16.1 2019/06/10 22:10:37 christos Exp $
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
