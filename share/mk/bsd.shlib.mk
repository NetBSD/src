#	$NetBSD: bsd.shlib.mk,v 1.7 2012/08/23 15:45:03 joerg Exp $

.if !defined(_BSD_SHLIB_MK_)
_BSD_SHLIB_MK_=1

.if ${MKDYNAMICROOT} == "no" || \
    (${BINDIR:Ux} != "/bin" && ${BINDIR:Ux} != "/sbin" && \
     ${BINDIR:Ux} != "/libexec" && ${USE_SHLIBDIR:Uno} == "no")
SHLIBDIR?=	/usr/lib
.else
SHLIBDIR?=	/lib
.endif

_LIBSODIR?=	${SHLIBDIR}

.if ${MKDYNAMICROOT} == "no" || \
    (${BINDIR:Ux} != "/bin" && ${BINDIR:Ux} != "/sbin" && \
     ${BINDIR:Ux} != "/libexec")
SHLINKDIR?=	/usr/libexec
.else
SHLINKDIR?=	/libexec
.endif

.endif	# !defined(_BSD_SHLIB_MK_)
