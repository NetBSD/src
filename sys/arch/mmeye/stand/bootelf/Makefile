#	$NetBSD: Makefile,v 1.10.34.1 2006/02/01 14:51:29 yamt Exp $

PROG=	bootelf
SRCS=	alloc.c boot.c byteorder.c loadfile.c loadfile_elf32.c
NOMAN=	# defined

LIBSA=	${.CURDIR}/../../../../lib/libsa

.PATH: ${LIBSA}

CFLAGS+= -D_STANDALONE -ffreestanding
CPPFLAGS+= -I${LIBSA} -I${.OBJDIR}
LDSTATIC= -static

CLEANFILES+=	machine

realdepend realall: machine

machine::
	-rm -f $@
	ln -s ${.CURDIR}/../../../../arch/${MACHINE}/include $@

${PROG}: ${OBJS} machine

.include <bsd.prog.mk>
