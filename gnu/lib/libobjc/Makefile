#	$NetBSD: Makefile,v 1.10 2001/01/08 08:29:59 itojun Exp $

HAVE_GCC28!= ${CXX} --version | egrep "^(2\.8|egcs)" ; echo

.if (${HAVE_GCC28} != "")

LIB=		objc
SRCS=		hash.c sarray.c class.c sendmsg.c init.c archive.c \
		encoding.c selector.c objects.c misc.c NXConstStr.m \
		Object.m Protocol.m nil_method.c thr.c linking.m \
		thr-single.c
DPSRCS+=	runtime-info.h

INCS=		hash.h objc-list.h sarray.h objc.h objc-api.h NXConstStr.h \
		Object.h Protocol.h encoding.h typedstream.h thr.h
INCSDIR=	/usr/include/objc

.if exists(${ARCH}/${MACHINE_ARCH})
ARCHSUBDIR= ${MACHINE_ARCH}
.elif exists(${ARCH}/${MACHINE_CPU})
ARCHSUBDIR= ${MACHINE_CPU}
.else
.BEGIN:
	@echo no egcs/arch/${MACHINE_ARCH} nor egcs/arch/${MACHINE_CPU} exists
	@false
.endif

CPPFLAGS+=	-I. -I${ARCH}/${ARCHSUBDIR} -I${ARCH} \
		-I${DIST}/config -I${DIST}

MKLINT=		no

DIST=		${.CURDIR}/../../dist/gcc
ARCH=		${.CURDIR}/../../usr.bin/egcs/arch

CLEANFILES+=	rtscratch rtscratch.s
runtime-info.h:
	@touch rtscratch
	`${OBJC} --print-prog-name=cc1obj` -print-objc-runtime-info rtscratch >$@

sendmsg.o sendmsg.so sendmsg.po: runtime-info.h

.PATH:		${DIST}/objc

.include <bsd.lib.mk>

.else

# XXX Temporary: during an egcs bootstrap, don't try to build with gcc 2.7
.include <bsd.own.mk>
${TARGETS}:

.endif
