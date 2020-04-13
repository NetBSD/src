#	$NetBSD: copts.mk,v 1.3.4.4 2020/04/13 08:04:17 martin Exp $

# MI per-file compiler options required.

.ifndef _SYS_CONF_COPTS_MK_
_SYS_CONF_COPTS_MK_=1

.if defined(HAVE_GCC) && ${HAVE_GCC} >= 7 && ${ACTIVE_CC} == "gcc"
COPTS.zlib.c+=		-Wno-error=implicit-fallthrough
COPTS.pf.c+=		-Wno-error=implicit-fallthrough
COPTS.radeon_cs.c+=	-Wno-error=implicit-fallthrough
COPTS.via_dmablit.c+=	-Wno-error=implicit-fallthrough
.endif

.if defined(HAVE_GCC) && ${HAVE_GCC} >= 8 && ${ACTIVE_CC} == "gcc" && \
    (${MACHINE_ARCH} == "mipseb" || ${MACHINE_ARCH} == "mipsel")
COPTS.linux_machdep.c+=	-Wno-error=unused-but-set-variable
.endif

.endif
