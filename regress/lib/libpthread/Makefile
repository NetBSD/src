#	$NetBSD: Makefile,v 1.16 2003/09/12 21:15:05 christos Exp $

.include <bsd.own.mk>

.if exists(${.CURDIR}/../../../lib/libpthread/arch/${MACHINE_ARCH})
ARCHSUBDIR=     ${MACHINE_ARCH}
.elif exists(${.CURDIR}/../../../lib/libpthread/arch/${MACHINE_CPU}) 
ARCHSUBDIR=     ${MACHINE_CPU}
.else
.BEGIN:
	@echo "WARNING: no ARCHSUBDIR for ${MACHINE_ARCH}/${MACHINE_CPU}"
	@echo "*** REFUSING TO CONTINUE! (nag, nag, nag) ***"
	@false	# this is intentional!
.endif

.if defined(ARCHSUBDIR)

SUBDIR+= atexit barrier1 cancel2 cond1 cond2 cond3 cond4 cond5 exit1 \
	 fork mutex1 mutex2 mutex3 mutex4 name once1 once2 preempt1 sem \
	 sigalarm sigsuspend

.endif

.include <bsd.subdir.mk>
