# $NetBSD: Makefile.inc,v 1.2 1997/06/05 12:59:19 agc Exp $
# Original from Freebsd, no rcs id.

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
