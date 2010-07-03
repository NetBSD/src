#	$NetBSD: Makefile,v 1.1.2.2 2010/07/03 01:20:02 rmind Exp $
#

.PATH:  ${.CURDIR}/../../../../kern

LIB=	rumpkern_tty

SRCS=	tty.c tty_bsdpty.c tty_conf.c tty_ptm.c tty_pty.c tty_tty.c tty_subr.c

SRCS+=	component.c

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpkern -I${RUMPTOP}/librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
