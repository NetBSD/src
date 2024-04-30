#	$NetBSD: Makefile,v 1.24 2024/04/30 19:35:29 andvar Exp $

MAN=	amdccp.4 amdpcib.4 apic.4 autoconf.4 balloon.4 console.4 coretemp.4 \
	est.4 fdc.4 fwhrng.4 hpet.4 ichlpcib.4 imcsmb.4 lpt.4 mem.4 odcm.4 \
	powernow.4 soekrisgpio.4 tco.4 amdsmn.4 amdzentemp.4 viac7temp.4

MLINKS+=apic.4 ioapic.4 \
	apic.4 lapic.4
MLINKS+=console.4 cons.4
MLINKS+=lpt.4 lpa.4
MLINKS+=mem.4 kmem.4

MANSUBDIR=/x86

.include <bsd.man.mk>
