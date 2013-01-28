#	$NetBSD: Makefile.inc,v 1.13 2013/01/28 14:25:18 matt Exp $

WARNS?=		3	# XXX: fails -Wsign-compare

.include <bsd.own.mk>

.PATH:		${.CURDIR}/../common

# Special cases first, then check for a MACHINE_ARCH specific
# subdir before looking for MACHINE_CPU.
.if exists(${.CURDIR}/../arch/${MACHINE_ARCH})
ARCHSUBDIR=	${MACHINE_ARCH}
.elif exists(${.CURDIR}/../arch/${MACHINE_ARCH:C/e[lb]$//})
ARCHSUBDIR=	${MACHINE_ARCH:C/e[lb]$//}
.else
ARCHSUBDIR=	${MACHINE_CPU}
.endif

CPPFLAGS+=	-I${.CURDIR}/../arch/${ARCHSUBDIR}
CPPFLAGS+=	-I${.CURDIR}/../common

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
