#	$NetBSD: Makefile,v 1.10 2002/08/27 08:53:17 lukem Exp $

PROG=	bootelf
SRCS=	boot.c byteorder.c loadfile.c loadfile_elf32.c
NOMAN=	# defined

LIBSA=	${.CURDIR}/../../../../lib/libsa

.PATH: ${LIBSA}

CPPFLAGS+= -I${LIBSA} -I${.OBJDIR}
LDSTATIC= -static

CLEANFILES+=	machine

realdepend realall: machine

machine::
	-rm -f $@
	ln -s ${.CURDIR}/../../../../arch/${MACHINE}/include $@

${PROG}: ${OBJS} machine

.include <bsd.prog.mk>
