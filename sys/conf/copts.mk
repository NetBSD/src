#	$NetBSD: copts.mk,v 1.3.4.2 2019/06/10 22:07:04 christos Exp $

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
