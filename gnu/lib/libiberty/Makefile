#	$NetBSD: Makefile,v 1.3 2001/12/12 23:09:13 lukem Exp $

NOLINT=		# defined
NOPROFILE=	# defined
NOPIC=		# defined
NOLINKLIB=	# defined

.include <bsd.own.mk>

.if defined(USE_NEW_TOOLCHAIN)

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
