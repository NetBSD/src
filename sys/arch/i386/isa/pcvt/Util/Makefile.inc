#	$NetBSD: Makefile.inc,v 1.8 1998/10/26 21:06:22 hubertf Exp $

ECHO	?=	echo

BINDIR	?=	/usr/bin
FONTDIR	=	/usr/share/misc/pcvtfonts
LIBDIR	=	/usr/lib
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
MANDIR	=	/usr/share/man

MAN = ${MAN1} ${MAN3} ${MAN4} ${MAN5} ${MAN8}

.if	empty(MAN) && defined(PROG)
MAN+=	${PROG}.1
.endif
