#	$NetBSD: copts.mk,v 1.12 2024/07/04 01:11:34 rin Exp $

# MI per-file compiler options required.

# Use -Wno-error=foo when the ultimate goal is to fix this warning
# with code change, and use -Wno-foo when the warning is bad.

.ifndef _SYS_CONF_COPTS_MK_
_SYS_CONF_COPTS_MK_=1

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

# Some of these indicate a potential GCC bug:
#   https://gcc.gnu.org/bugzilla/show_bug.cgi?id=110878
.if defined(HAVE_GCC) && ${HAVE_GCC} >= 12 && ${ACTIVE_CC} == "gcc" && \
    (${MACHINE_ARCH} == "aarch64" || ${MACHINE_ARCH} == "aarch64eb")
COPTS.aes_armv8.c+=	${CC_WNO_STRINGOP_OVERREAD} ${CC_WNO_STRINGOP_OVERFLOW}
COPTS.aes_neon.c+=	${CC_WNO_STRINGOP_OVERREAD} ${CC_WNO_STRINGOP_OVERFLOW} -flax-vector-conversions
COPTS.aes_neon_subr.c+=	${CC_WNO_ARRAY_BOUNDS} -flax-vector-conversions
COPTS.chacha_neon.c+=	-flax-vector-conversions
.endif

.if ${MACHINE_ARCH} == "x86_64" || ${MACHINE_ARCH} == "i386"
COPTS.aes_ni.c+=	${CC_WNO_STRINGOP_OVERREAD} ${CC_WNO_STRINGOP_OVERFLOW}
COPTS.aes_sse2_subr.c+=	${CC_WNO_ARRAY_BOUNDS}
COPTS.aes_ssse3_subr.c+=${CC_WNO_ARRAY_BOUNDS}
COPTS.aes_via.c+=	${CC_WNO_ARRAY_BOUNDS}
.endif

.endif
