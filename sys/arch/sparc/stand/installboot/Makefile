#	$NetBSD: Makefile,v 1.13.8.3 2002/02/28 04:12:09 nathanw Exp $

NOMAN=	# defined

.include <bsd.own.mk>

WARNS?=1
PROG=	installboot
SRCS=	installboot.c byteorder.c loadfile.c loadfile_aout.c loadfile_elf32.c

CPPFLAGS+=-DBOOT_AOUT -DBOOT_ELF32

CFLAGS+=-g

LIBSA=${.CURDIR}/../../../../lib/libsa

.PATH.c: ${LIBSA}

CPPFLAGS+=-I${LIBSA} -I.
CPPFLAGS+=-I${.CURDIR}/..

LDSTATIC?=	-static

.if !defined(HOSTPROGNAME)
.if !make(obj) && !make(clean) && !make(cleandir)
.BEGIN:
	@([ -h machine ] || ln -s ${.CURDIR}/../../../${MACHINE}/include machine)
	@([ -h sparc ] || ln -s ${.CURDIR}/../../../sparc/include sparc)
.NOPATH: machine sparc
.endif
CLEANFILES+= machine sparc
.endif

.ifndef HOSTPROG
.include <bsd.prog.mk>
.endif
