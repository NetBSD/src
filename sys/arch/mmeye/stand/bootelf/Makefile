#	$NetBSD: Makefile,v 1.12 2006/04/03 06:15:00 uebayasi Exp $

PROG=	bootelf
SRCS=	alloc.c boot.c byteorder.c loadfile.c loadfile_elf32.c
NOMAN=	# defined

LIBSA=	${.CURDIR}/../../../../lib/libsa

.PATH: ${LIBSA}

CPPFLAGS+=	-I${.CURDIR}/../../../.. -I${.OBJDIR}
LDSTATIC=	-static

CLEANFILES+=	machine

realdepend realall: machine

machine::
	-rm -f $@
	ln -s ${.CURDIR}/../../../../arch/${MACHINE}/include $@

${PROG}: ${OBJS} machine

.include <bsd.prog.mk>
