#	$NetBSD: copts.mk,v 1.10 2023/06/03 21:30:21 lukem Exp $

# MI per-file compiler options required.

# Use -Wno-error=foo when the ultimate goal is to fix this warning
# with code change, and use -Wno-foo when the warning is bad.

.ifndef _SYS_CONF_COPTS_MK_
_SYS_CONF_COPTS_MK_=1

COPTS.zlib.c+=		${CC_WNO_IMPLICIT_FALLTHROUGH}
COPTS.pf.c+=		${CC_WNO_IMPLICIT_FALLTHROUGH}
COPTS.radeon_cs.c+=	${CC_WNO_IMPLICIT_FALLTHROUGH}
COPTS.via_dmablit.c+=	${CC_WNO_IMPLICIT_FALLTHROUGH}

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
