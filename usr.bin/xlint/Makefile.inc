#	$NetBSD: Makefile.inc,v 1.9.44.1 2009/05/13 19:20:13 jym Exp $

WARNS?=		3	# XXX: fails -Wsign-compare

.include <bsd.own.mk>

.PATH:		${.CURDIR}/../common

# On the m68000, MACHINE_CPU is "m68k", but we have different
# target parameters (namely, long double is a different size
# on the m68000), so we special case the directory name here.
.if ${MACHINE_ARCH} == "m68000"
ARCHSUBDIR=	m68000
.else
ARCHSUBDIR=	${MACHINE_CPU}
.endif

CPPFLAGS+=	-I${.CURDIR}/../arch/${ARCHSUBDIR}
CPPFLAGS+=	-I${.CURDIR}/../common

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
