#	$NetBSD: Makefile,v 1.4 2002/02/11 21:36:35 tv Exp $

NOLINT=		# defined
NOPROFILE=	# defined
NOPIC=		# defined
NOLINKLIB=	# defined

.include <bsd.own.mk>

.if defined(USE_NEW_TOOLCHAIN) && exists(${.CURDIR}/arch/${MACHINE_ARCH}/defs.mk)

LIB=		iberty

.include "${.CURDIR}/arch/${MACHINE_ARCH}/defs.mk"

TOP=		${.CURDIR}/../..
DIST=		${TOP}/dist/toolchain

SRCS=		${G_REQUIRED_OFILES:.o=.c} ${G_EXTRA_OFILES:.o=.c} \
		${G_LIBOBJS:.o=.c} ${G_ALLOCA:.o=.c}

CPPFLAGS+=	-DHAVE_CONFIG_H -I${.CURDIR}/arch/${MACHINE_ARCH} \
		-I${DIST}/include

.PATH: ${DIST}/libiberty

.include <bsd.lib.mk>

#${OBJS}: ${.CURDIR}/arch/${MACHINE_ARCH}/defs.mk

.else
# Do nothing. (PROG undefined)
.include <bsd.prog.mk>
.endif
