#	$NetBSD: Makefile,v 1.9.2.1 2000/11/20 20:25:50 bouyer Exp $

.include <bsd.own.mk>

WARNS?=1
PROG=	installboot
SRCS=	installboot.c loadfile.c
MKMAN=	no
CFLAGS+=-g

LIBSA=${.CURDIR}/../../../../lib/libsa

.PATH.c: ${LIBSA}

CPPFLAGS+=-I${LIBSA} -I.

LDSTATIC?=	-static

.if !make(obj)
.BEGIN:
	@([ -h machine ] || ln -s ${.CURDIR}/../../../${MACHINE}/include machine)
	@([ -h sparc ] || ln -s ${.CURDIR}/../../../sparc/include sparc)
.NOPATH: machine sparc
CLEANFILES+= machine sparc
.endif

.include <bsd.prog.mk>
