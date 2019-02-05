#	$NetBSD: copts.mk,v 1.1 2019/02/05 08:33:25 mrg Exp $

# MI per-file compiler options required.

.ifndef _SYS_CONF_COPTS_MK_
_SYS_CONF_COPTS_MK_=1

COPTS.zlib.c+=	-Wno-error=implicit-fallthrough

.endif
