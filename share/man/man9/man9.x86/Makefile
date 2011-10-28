#	$NetBSD: Makefile,v 1.6 2011/10/28 07:23:18 jruoho Exp $

MAN=	nmi.9 rdmsr.9 tsc.9 x86_msr_xcall.9

MANSUBDIR=/x86

MLINKS+=nmi.9 nmi_establish.9 \
	nmi.9 nmi_disestablish.9

MLINKS+=rdmsr.9 msr.9 \
	rdmsr.9 rdmsr_safe.9 \
	rdmsr.9 wrmsr.9

MLINKS+=tsc.9 rdtsc.9 \
	tsc.9 tsc_tc_init.9 \
	tsc.9 tsc_sync_ap.9 \
	tsc.9 tsc_sync_bp.9 \
	tsc.9 tsc_sync_drift.9

.include <bsd.man.mk>
