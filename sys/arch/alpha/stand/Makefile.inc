# $NetBSD: Makefile.inc,v 1.20 2004/12/28 07:50:00 jmc Exp $

NOMAN=			# defined

.include <bsd.own.mk>

BINDIR?=		/usr/mdec
WARNS?=			1

CPPFLAGS+=		-I${.CURDIR}/../..
LDSTATIC=		-static

# binutils 2.15 forces a .eh_frame section into all objects. Nuke it for space
.c.o:
	${COMPILE.c} ${.IMPSRC}
	${OBJCOPY} -R .eh_frame ${.PREFIX}.o 
.S.o:
	${COMPILE.S} ${.IMPSRC}
	${OBJCOPY} -R .eh_frame ${.PREFIX}.o 
.s.o:
	${COMPILE.s} ${.IMPSRC}
	${OBJCOPY} -R .eh_frame ${.PREFIX}.o 
