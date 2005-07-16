#	$NetBSD: Makefile,v 1.28 2005/07/16 23:02:24 nathanw Exp $

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

SUBDIR+= atexit \
	 barrier1 \
	 cancel2 \
         cond1 cond2 cond3 cond4 cond5 cond6 condcancel1 conddestroy1 \
	 exit1 \
	 fork \
	 kill1 \
	 mutex1 mutex2 mutex3 mutex4 \
	 name \
	 once1 once2 once3 \
	 preempt1 \
	 resolv rwlock1 \
	 sem sem2 sigalarm sigmask1 sigmask2 sigmask3 siglongjmp1 sigsuspend \
	 sleep1

.endif

.include <bsd.subdir.mk>
