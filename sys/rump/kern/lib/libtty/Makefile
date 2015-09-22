#	$NetBSD: Makefile,v 1.3.6.2 2015/09/22 12:06:15 skrll Exp $
#

.PATH:  ${.CURDIR}/../../../../kern					\
	${.CURDIR}/../../../../compat/common

LIB=	rumpkern_tty
IOCONF=	TTY.ioconf
SRCS=	tty.c tty_bsdpty.c tty_conf.c tty_ptm.c tty_pty.c tty_tty.c tty_subr.c

.include <bsd.init.mk>

.if !empty(RUMP_NBCOMPAT:M60)
SRCS+=	tty_60.c
.endif

SRCS+=	tty_component.c

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpkern -I${RUMPTOP}/librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
