#	$NetBSD: Makefile,v 1.17 2018/01/25 01:25:39 christos Exp $

MAN=	amdpcib.4 apic.4 balloon.4 coretemp.4 est.4 fdc.4 \
	fwhrng.4 hpet.4 ichlpcib.4 lpt.4 mem.4 odcm.4 powernow.4 \
	soekrisgpio.4 tco.4 tprof_amdpmi.4 tprof_pmi.4 vmt.4 vmx.4 \
	amdsmn.4 amdzentemp.4

MLINKS+=apic.4 ioapic.4 \
	apic.4 lapic.4
MLINKS+=lpt.4 lpa.4
MLINKS+=mem.4 kmem.4

MANSUBDIR=/x86

.include <bsd.man.mk>
