#	$NetBSD: Makefile.inc,v 1.7 1997/12/10 14:14:24 drochner Exp $

ECHO	?=	echo

BINDIR	?=	/usr/local/bin
FONTDIR	=	/usr/share/misc/pcvtfonts
LIBDIR	=	/usr/local/lib
LIBMODE	=	644

# for pcvt_ioctl.h

INCDIR= ${DESTDIR}/usr/include/machine

${PROG}: ${INCDIR}/pcvt_ioctl.h

${INCDIR}/pcvt_ioctl.h:
	@echo "\"make includes\" in parent directory first!"; false

MAN3EXT	=	3
MAN4EXT	=	4
MAN5EXT	=	5
MAN8EXT	=	8
MANDIR	=	/usr/local/man

MAN = ${MAN1} ${MAN3} ${MAN4} ${MAN5} ${MAN8}

.if	empty(MAN) && defined(PROG)
MAN+=	${PROG}.1
.endif
