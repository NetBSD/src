#	$NetBSD: Makefile,v 1.19 2002/02/27 17:58:52 tv Exp $

REQUIRETOOLS=	yes
NOLINT=		# defined

.include <bsd.own.mk>

LIB=		objc

.if defined(USE_NEW_TOOLCHAIN)

SHLIB_MAJOR=	1
SHLIB_MINOR=	0

# Machine-independent definitions (include file names).
.include "${.CURDIR}/defs.mk"

TOP=		${.CURDIR}/../..
DIST=		${TOP}/dist/toolchain
GCCARCH=	${TOP}/usr.bin/gcc/arch/${MACHINE_ARCH}

SRCS=		${G_OBJS:N[A-Z]*:Nlinking.o:.o=.c} ${G_OBJS:M[A-Z]*:.o=.m} linking.m

GCPPFLAGS=	${G_ALL_CFLAGS} ${G_INCLUDES}
CPPFLAGS+=	-I. -I${GCCARCH} ${GCPPFLAGS:M-D*} ${GCPPFLAGS:M-I*:N-I.*}

INCS=		${G_OBJC_H}
INCSDIR=	/usr/include/objc

DPSRCS+=	runtime-info.h
CLEANFILES+=	rtscratch rtscratch.s
runtime-info.h:
	@touch rtscratch
	`${OBJC} --print-prog-name=cc1obj` -print-objc-runtime-info rtscratch >$@

.PATH: ${DIST}/libobjc ${DIST}/libobjc/objc

.include <bsd.lib.mk>

${OBJS} ${POBJS} ${SOBJS}: runtime-info.h

.else
#
# XXX - BEGIN - Old toolchain build goop - delete when everything migrated - XXX
#
HAVE_GCC28!= ${CXX} --version | egrep "^(2\.8|egcs)" ; echo

.if (${HAVE_GCC28} != "")

SHLIB_MAJOR=	0
SHLIB_MINOR=	0

SRCS=		hash.c sarray.c class.c sendmsg.c init.c archive.c \
		encoding.c selector.c objects.c misc.c NXConstStr.m \
		Object.m Protocol.m nil_method.c thr.c linking.m \
		thr-single.c
DPSRCS+=	runtime-info.h

INCS=		hash.h objc-list.h sarray.h objc.h objc-api.h NXConstStr.h \
		Object.h Protocol.h encoding.h typedstream.h thr.h
INCSDIR=	/usr/include/objc
DIST=		${.CURDIR}/../../dist/gcc
ARCH=		${.CURDIR}/../../usr.bin/egcs/arch

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

CLEANFILES+=	rtscratch rtscratch.s
runtime-info.h:
	@touch rtscratch
	`${OBJC} --print-prog-name=cc1obj` -print-objc-runtime-info rtscratch >$@

sendmsg.o sendmsg.so sendmsg.po: runtime-info.h

.PATH:		${DIST}/objc

.include <bsd.lib.mk>

.else

# XXX Temporary: during an egcs bootstrap, don't try to build with gcc 2.7
${TARGETS}:

.endif
#
# XXX - END - Old toolchain build goop - delete when everything migrated - XXX
#
.endif
