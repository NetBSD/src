#	$NetBSD: copts.mk,v 1.9 2023/06/03 09:09:13 lukem Exp $

# MI per-file compiler options required.

# Use -Wno-error=foo when the ultimate goal is to fix this warning
# with code change, and use -Wno-foo when the warning is bad.

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

.if defined(HAVE_GCC) && ${HAVE_GCC} >= 10 && ${ACTIVE_CC} == "gcc"
COPTS.ath.c+=		-Wno-error=enum-conversion
COPTS.dpt.c+=		${CC_WNO_ADDRESS_OF_PACKED_MEMBER}
COPTS.ffs_appleufs.c+=	${CC_WNO_ADDRESS_OF_PACKED_MEMBER}
# These are wrong. The code explicitly avoids this case.
COPTS.in_pcb.c+=	${CC_WNO_RETURN_LOCAL_ADDR}
COPTS.in6_pcb.c+=	${CC_WNO_RETURN_LOCAL_ADDR}
# Also seems wrong.
COPTS.magma.c+=		${CC_WNO_MAYBE_UNINITIALIZED}
.endif

.endif
