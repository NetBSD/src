#	$NetBSD: copts.mk,v 1.2 2019/02/05 09:29:19 mrg Exp $

# MI per-file compiler options required.

.ifndef _SYS_CONF_COPTS_MK_
_SYS_CONF_COPTS_MK_=1

.if defined(HAVE_GCC) && ${HAVE_GCC} == 7 && ${ACTIVE_CC} == "gcc"
COPTS.zlib.c+=	-Wno-error=implicit-fallthrough
.endif

.endif
