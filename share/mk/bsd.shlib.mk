#	$NetBSD: bsd.shlib.mk,v 1.3 2004/01/02 16:12:36 lukem Exp $

.if !defined(_BSD_SHLIB_MK_)
_BSD_SHLIB_MK_=1

.if ${MKDYNAMICROOT} == "no"
SHLIBINSTALLDIR?= /usr/lib
.else
SHLIBINSTALLDIR?= /lib
.endif

.if ${MKDYNAMICROOT} == "no" || \
    (${BINDIR:Ux} != "/bin" && ${BINDIR:Ux} != "/sbin")
SHLIBDIR?=	/usr/lib
.else
SHLIBDIR?=	/lib
.endif

.if ${USE_SHLIBDIR:Uno} != "no"
_LIBSODIR?=	${SHLIBINSTALLDIR}
.else
_LIBSODIR?=	${LIBDIR}
.endif

.if ${MKDYNAMICROOT} == "no"
SHLINKINSTALLDIR?= /usr/libexec
.else
SHLINKINSTALLDIR?= /libexec
.endif

.if ${MKDYNAMICROOT} == "no" || \
    (${BINDIR:Ux} != "/bin" && ${BINDIR:Ux} != "/sbin")
SHLINKDIR?=	/usr/libexec
.else
SHLINKDIR?=	/libexec
.endif

.endif	# !defined(_BSD_SHLIB_MK_)
