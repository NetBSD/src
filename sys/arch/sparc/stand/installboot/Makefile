#	$NetBSD: Makefile,v 1.17 2001/10/31 17:21:47 thorpej Exp $

.include <bsd.own.mk>

WARNS?=1
PROG=	installboot
SRCS=	installboot.c byteorder.c loadfile.c loadfile_aout.c \
	loadfile_elf32.c loadfile_elf64.c
MKMAN=	no
CFLAGS+=-g

LIBSA=${.CURDIR}/../../../../lib/libsa

.PATH.c: ${LIBSA}

CPPFLAGS+=-I${LIBSA} -I.
CPPFLAGS+=-I${.CURDIR}/..
CPPFLAGS+=-DBOOT_ELF64

LDSTATIC?=	-static

.if !make(obj)
.BEGIN:
	@([ -h machine ] || ln -s ${.CURDIR}/../../../${MACHINE}/include machine)
	@([ -h sparc ] || ln -s ${.CURDIR}/../../../sparc/include sparc)
.NOPATH: machine sparc
CLEANFILES+= machine sparc
.endif

.include <bsd.prog.mk>
