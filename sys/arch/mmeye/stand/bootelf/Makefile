#	$NetBSD: Makefile,v 1.8 2002/05/02 18:54:32 wiz Exp $

PROG= bootelf
SRCS= boot.c byteorder.c loadfile.c loadfile_elf32.c
NOMAN=
# defined
LIBSA=${.CURDIR}/../../../../lib/libsa

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
