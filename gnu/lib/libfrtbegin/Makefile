#	$NetBSD: Makefile,v 1.3 2003/09/04 04:34:18 mrg Exp $

LIB=	frtbegin

NOLINT=		# defined
NOPROFILE=	# defined
NOPIC=		# defined

.include <bsd.own.mk>

.include "${.CURDIR}/../libg2c3/arch/${MACHINE_ARCH}/defs.mk"

DIST=		${NETBSDSRCDIR}/gnu/dist/gcc

SRCS=  		main.c
#CPPFLAGS+=      -DHAVE_CONFIG_H -I${.CURDIR} -I${DIST}/include
GCPPFLAGS=	${G_ALL_CFLAGS:O:u}
CPPFLAGS+=	-I${.CURDIR}/arch/${MACHINE_ARCH} \
		${GCPPFLAGS:M-D*} ${GCPPFLAGS:M-I*:N-I.*} \
		-DAllow_TYQUAD

.PATH: ${DIST}/libf2c/libF77

.include <bsd.lib.mk>
