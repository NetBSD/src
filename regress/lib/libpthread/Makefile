#	$NetBSD: Makefile,v 1.8 2003/02/18 09:35:31 uwe Exp $

.include <bsd.own.mk>

.if exists(${.CURDIR}/../../../lib/libpthread/arch/${MACHINE_ARCH})
ARCHSUBDIR=     ${MACHINE_ARCH}
.elif exists(${.CURDIR}/../../../lib/libpthread/arch/${MACHINE_CPU}) 
ARCHSUBDIR=     ${MACHINE_CPU}
.else
.BEGIN:
	@echo "WARNING: no ARCHSUBDIR for ${MACHINE_ARCH}/${MACHINE_CPU}; skipping..."
.endif

.if defined(ARCHSUBDIR)

SUBDIR+= barrier1 cond1 cond2 cond3 cond4 cond5 \
	 mutex1 mutex2 mutex3 mutex4 once1 once2 sem

.endif

.include <bsd.subdir.mk>
