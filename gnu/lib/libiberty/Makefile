#	$NetBSD: Makefile,v 1.8 2003/12/04 09:29:32 mrg Exp $

NOLINT=		# defined
NOPROFILE=	# defined
NOPIC=		# defined
NOLINKLIB=	# defined

.include <bsd.own.mk>

LIB=		iberty

.include "${.CURDIR}/defs.mk"

DIST=		${NETBSDSRCDIR}/gnu/dist/binutils

SRCS=		${G_REQUIRED_OFILES:.o=.c} ${G_EXTRA_OFILES:.o=.c} \
		${G_LIBOBJS:.o=.c} ${G_ALLOCA:.o=.c}

CPPFLAGS+=	-DHAVE_CONFIG_H -I${.CURDIR} -I${DIST}/include

.PATH: ${DIST}/libiberty

.include <bsd.lib.mk>
