#	$NetBSD: Makefile.inc,v 1.5 2001/01/14 06:29:27 christos Exp $

.if !target(assym.h)
S!=	cd ${.CURDIR}/../../..;pwd

DPSRCS+= assym.h

.if exists($S/arch/${MACHINE}/${MACHINE}/genassym.cf)
GENASSYM_DIR?= $S/arch/${MACHINE}/${MACHINE}
.elif exists($S/arch/${MACHINE_ARCH}/${MACHINE_ARCH}/genassym.cf)
GENASSYM_DIR?= $S/arch/${MACHINE_ARCH}/${MACHINE_ARCH}
.else
GENASSYM_DIR?=
.endif

assym.h: ${S}/kern/genassym.sh ${GENASSYM_DIR}/genassym.cf
	sh $S/kern/genassym.sh ${CC} ${CFLAGS} ${CPPFLAGS} ${PROF} \
		< ${GENASSYM_DIR}/genassym.cf > assym.h.tmp && \
	mv -f assym.h.tmp assym.h

.include "../Makefile.inc"
.endif
