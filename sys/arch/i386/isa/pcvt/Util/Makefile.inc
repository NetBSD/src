#	$NetBSD: Makefile.inc,v 1.6.6.1 1998/11/04 17:38:59 cgd Exp $

ECHO	?=	echo

BINDIR	?=	/usr/bin
FONTDIR	=	/usr/share/pcvt/fonts
LIBDIR	=	/usr/lib
LIBMODE	=	644

INCDIR= ${DESTDIR}/usr/include/machine

${PROG}: ${INCDIR}/pcvt_ioctl.h

MAN3EXT	=	3
MAN4EXT	=	4
MAN5EXT	=	5
MAN8EXT	=	8
MANDIR	=	/usr/share/man

MAN = ${MAN1} ${MAN3} ${MAN4} ${MAN5} ${MAN8}

.if	empty(MAN) && defined(PROG)
MAN+=	${PROG}.1
.endif
