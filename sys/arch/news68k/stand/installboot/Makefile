#	$NetBSD: Makefile,v 1.4 2001/10/31 01:51:45 thorpej Exp $

PROG= installboot
SRCS= installboot.c loadfile.c loadfile_aout.c loadfile_elf32.c
MKMAN= no
WARNS?= 1

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
