#	$NetBSD: Makefile,v 1.5 2002/02/27 17:03:49 tv Exp $

NOLINT=		# defined
NOPROFILE=	# defined
NOPIC=		# defined
NOLINKLIB=	# defined

.include <bsd.own.mk>

.if defined(USE_NEW_TOOLCHAIN)

LIB=		iberty

.include "${.CURDIR}/defs.mk"

TOP=		${.CURDIR}/../..
DIST=		${TOP}/dist/toolchain

SRCS=		${G_REQUIRED_OFILES:.o=.c} ${G_EXTRA_OFILES:.o=.c} \
		${G_LIBOBJS:.o=.c} ${G_ALLOCA:.o=.c}

CPPFLAGS+=	-DHAVE_CONFIG_H -I${.CURDIR} -I${DIST}/include

.PATH: ${DIST}/libiberty

.include <bsd.lib.mk>

.else
# Do nothing. (PROG undefined)
.include <bsd.prog.mk>
.endif
