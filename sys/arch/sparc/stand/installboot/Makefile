#	$NetBSD: Makefile,v 1.21 2001/12/04 18:54:34 thorpej Exp $

.include <bsd.own.mk>

WARNS?=1
PROG=	installboot
SRCS=	installboot.c byteorder.c loadfile.c loadfile_aout.c loadfile_elf32.c

CPPFLAGS+=-DBOOT_AOUT -DBOOT_ELF32

MKMAN=	no
CFLAGS+=-g

LIBSA=${.CURDIR}/../../../../lib/libsa

.PATH.c: ${LIBSA}

CPPFLAGS+=-I${LIBSA} -I.
CPPFLAGS+=-I${.CURDIR}/..

LDSTATIC?=	-static

.if !make(obj) && !make(clean) && !make(cleandir)
.BEGIN:
	@([ -h machine ] || ln -s ${.CURDIR}/../../../${MACHINE}/include machine)
	@([ -h sparc ] || ln -s ${.CURDIR}/../../../sparc/include sparc)
.NOPATH: machine sparc
.endif
CLEANFILES+= machine sparc

.include <bsd.prog.mk>
