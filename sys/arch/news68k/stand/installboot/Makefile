#	$NetBSD: Makefile,v 1.2 2001/01/23 11:30:54 tsutsui Exp $

PROG= installboot
SRCS= installboot.c loadfile.c
MKMAN= no

S= ${.CURDIR}/../../../..
LIBSA=${S}/lib/libsa

.PATH.c: ${LIBSA}

CPPFLAGS+= -I${.OBJDIR} -I${LIBSA}
LDSTATIC?= -static

# for machine/loadfile_machdep.h
CLEANFILES+= machine m68k

.BEGIN: machine m68k
.NOPATH: machine m68k
realdepend realall: machine m68k

machine::
	-rm -f $@
	ln -s ${S}/arch/${MACHINE}/include $@

m68k::
	-rm -f $@
	ln -s ${S}/arch/m68k/include $@

.include <bsd.prog.mk>
