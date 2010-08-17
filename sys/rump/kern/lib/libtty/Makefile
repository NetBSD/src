#	$NetBSD: Makefile,v 1.1.6.2 2010/08/17 06:47:59 uebayasi Exp $
#

.PATH:  ${.CURDIR}/../../../../kern

LIB=	rumpkern_tty

SRCS=	tty.c tty_bsdpty.c tty_conf.c tty_ptm.c tty_pty.c tty_tty.c tty_subr.c

SRCS+=	component.c

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpkern -I${RUMPTOP}/librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
