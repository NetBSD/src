#	$NetBSD: Makefile,v 1.1 1998/08/18 14:02:35 tv Exp $

LIB=		objc
SRCS=		hash.c sarray.c class.c sendmsg.c init.c archive.c \
		encoding.c selector.c objects.c misc.c NXConstStr.m \
		Object.m Protocol.m nil_method.c thr.c linking.m \
		thr-single.c
DPSRCS+=	runtime-info.h

CPPFLAGS+=	-I. -I${ARCH}/${MACHINE_ARCH} -I${ARCH} \
		-I${DIST}/config -I${DIST}

NOLINT=		nolint (Objective-C)

DIST=		${.CURDIR}/../../dist/gcc
ARCH=		${.CURDIR}/../../usr.bin/egcs/arch

CLEANFILES+=	rtscratch rtscratch.s
runtime-info.h:
	@touch rtscratch
	`${CC} --print-prog-name=cc1obj` -print-objc-runtime-info rtscratch >$@

sendmsg.o: runtime-info.h

.PATH:		${DIST}/objc

.include <bsd.lib.mk>
