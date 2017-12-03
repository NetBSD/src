#	$NetBSD: Makefile,v 1.1.24.2 2017/12/03 11:39:15 jdolecek Exp $
#

.PATH:  ${.CURDIR}/../../../../kern					\
	${.CURDIR}/../../../../compat/common

LIB=	rumpkern_tty
COMMENT=TTY/PTY support

IOCONF=	TTY.ioconf
SRCS=	tty.c tty_bsdpty.c tty_conf.c tty_ptm.c tty_pty.c tty_tty.c tty_subr.c

.include <bsd.init.mk>

.if !empty(RUMP_NBCOMPAT:M60)
SRCS+=	tty_60.c
.endif

SRCS+=	tty_component.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
