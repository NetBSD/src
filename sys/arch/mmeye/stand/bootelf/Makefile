#	$NetBSD: Makefile,v 1.2.16.2 2002/06/23 17:38:16 jdolecek Exp $

PROG=	bootelf
SRCS=	boot.c byteorder.c loadfile.c loadfile_elf32.c
NOMAN=	# defined

LIBSA=	${.CURDIR}/../../../../lib/libsa

.PATH: ${LIBSA}

CPPFLAGS+= -I${LIBSA} -I${.OBJDIR}
LDSTATIC?= -static

CLEANFILES+=	machine

realdepend realall: machine

machine::
	-rm -f $@
	ln -s ${.CURDIR}/../../../../arch/${MACHINE}/include $@

${PROG}: ${OBJS} machine

.include <bsd.prog.mk>
