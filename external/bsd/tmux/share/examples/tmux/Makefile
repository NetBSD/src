# $NetBSD: Makefile,v 1.2 2016/08/31 17:27:37 christos Exp $

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
SRCDIR=		${NETBSDSRCDIR}/external/bsd/tmux/dist
.PATH:		${SRCDIR}

FILESDIR=	/usr/share/examples/tmux
FILESMODE=	444
FILES=		example_tmux.conf
.endif

.include <bsd.prog.mk>
