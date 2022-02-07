#	$NetBSD: Makefile.inc,v 1.18 2022/02/07 21:57:47 rillig Exp $

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

CLEANFILES+=	*.gcno *.gcda *.gcov

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
