# $NetBSD: Makefile.inc,v 1.3 1997/10/17 12:03:44 lukem Exp $
# Original from Freebsd, no rcs id.

WARNS?=	0

# Figure out what to use for libinstall
INSTALLLIBDIR=		${.CURDIR}/../lib

.if exists(${INSTALLLIBDIR}/obj.${MACHINE})
INSTALLOBJDIR=		${INSTALLLIBDIR}/obj.${MACHINE}
.else
.if exists(${INSTALLLIBDIR}/obj)
INSTALLOBJDIR=		${INSTALLLIBDIR}/obj
.else
INSTALLOBJDIR=		${INSTALLLIBDIR}
.endif
.endif

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
