#	$NetBSD: Makefile.inc,v 1.14 2003/08/01 17:04:02 lukem Exp $

.if !target(assym.h)
S!=	cd ${.CURDIR}/../../..;pwd

NOMAN=	# defined

.include <bsd.own.mk>

DPSRCS+=	assym.h
CLEANFILES+=	assym.h

.if (${MACHINE_ARCH} == "powerpc")
.if (${MACHINE} == "macppc" || ${MACHINE} == "ofppc" || ${MACHINE} == "prep")
# use oea
GENASSYM_DIR= $S/arch/${MACHINE_ARCH}/oea
CPPFLAGS+=	-DPPC_OEA
.endif
.endif

.if exists($S/arch/${MACHINE}/${MACHINE}/genassym.cf)
GENASSYM_DIR?= $S/arch/${MACHINE}/${MACHINE}
.elif exists($S/arch/${MACHINE_CPU}/${MACHINE_CPU}/genassym.cf)
GENASSYM_DIR?= $S/arch/${MACHINE_CPU}/${MACHINE_CPU}
.else
GENASSYM_DIR?=
.endif

assym.h: ${S}/kern/genassym.sh ${GENASSYM_DIR}/genassym.cf
	sh $S/kern/genassym.sh ${CC} ${CFLAGS} ${CPPFLAGS} ${PROF} \
		< ${GENASSYM_DIR}/genassym.cf > assym.h.tmp && \
	mv -f assym.h.tmp assym.h

.if defined(COMPAT_UTIL_OVERRIDE) && !empty(COMPAT_UTIL_OVERRIDE)
# compat_util.c symbol redifinitions
CPPFLAGS+=	-Demul_find=${COMPAT_UTIL_OVERRIDE}_emul_find				\
		-Dstackgap_init=${COMPAT_UTIL_OVERRIDE}_stackgap_init			\
		-Demul_flags_translate=${COMPAT_UTIL_OVERRIDE}_emul_flags_translate	\
		-Dstackgap_alloc=${COMPAT_UTIL_OVERRIDE}_stackgap_alloc		 	\
		-Dcompat_offseterr=${COMPAT_UTIL_OVERRIDE}_compat_offseterr		\
		-Demul_find_interp=${COMPAT_UTIL_OVERRIDE}_emul_find_interp

# get emul_find() and friends
.PATH: $S/compat/common
SRCS+=	compat_util.c

.endif

.include "../Makefile.inc"
.endif
