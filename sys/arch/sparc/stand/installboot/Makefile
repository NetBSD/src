#	$NetBSD: Makefile,v 1.20 2001/12/04 11:31:10 jmc Exp $

.include <bsd.own.mk>

WARNS?=1
PROG=	installboot
SRCS=	installboot.c byteorder.c loadfile.c loadfile_aout.c 

.if ${MACHINE_ARCH} == "sparc"
SRCS+= loadfile_elf32.c
.endif

.if ${MACHINE_ARCH} == "sparc64"
SRCS+= loadfile_elf64.c
.endif

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
