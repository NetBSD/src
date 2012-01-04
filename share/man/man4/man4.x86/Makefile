#	$NetBSD: Makefile,v 1.13 2012/01/04 16:25:17 yamt Exp $

MAN=	amdpcib.4 apic.4 balloon.4 coretemp.4 est.4 fdc.4 \
	fwhrng.4 hpet.4 ichlpcib.4 lpt.4 mem.4 odcm.4 powernow.4 \
	tprof_amdpmi.4 tprof_pmi.4 vmt.4

MLINKS+=apic.4 ioapic.4 \
	apic.4 lapic.4
MLINKS+=lpt.4 lpa.4
MLINKS+=mem.4 kmem.4

MANSUBDIR=/x86

.include <bsd.man.mk>
