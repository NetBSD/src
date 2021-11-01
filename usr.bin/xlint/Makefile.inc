#	$NetBSD: Makefile.inc,v 1.17 2021/11/01 19:48:51 rillig Exp $

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
CWARNFLAGS.gcc+=-Wdeclaration-after-statement	# see tools/README
CWARNFLAGS.gcc+=-std=c90			# see tools/README

CLEANFILES+=	*.gcno *.gcda *.gcov

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
