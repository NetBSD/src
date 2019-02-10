#	$NetBSD: copts.mk,v 1.3 2019/02/10 05:01:59 mrg Exp $

# MI per-file compiler options required.

.ifndef _SYS_CONF_COPTS_MK_
_SYS_CONF_COPTS_MK_=1

.if defined(HAVE_GCC) && ${HAVE_GCC} == 7 && ${ACTIVE_CC} == "gcc"
COPTS.zlib.c+=		-Wno-error=implicit-fallthrough
COPTS.pf.c+=		-Wno-error=implicit-fallthrough
COPTS.radeon_cs.c+=	-Wno-error=implicit-fallthrough
COPTS.via_dmablit.c+=	-Wno-error=implicit-fallthrough
.endif

.endif
