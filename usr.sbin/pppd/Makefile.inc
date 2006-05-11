#	$NetBSD: Makefile.inc,v 1.3 2006/05/11 07:17:41 mrg Exp $

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif

.include <bsd.own.mk>

.if ${HAVE_GCC} == 4
COPTS+=	-fno-strict-aliasing
.endif
