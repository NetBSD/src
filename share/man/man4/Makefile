#	$NetBSD: Makefile,v 1.251 2002/03/25 19:15:35 martin Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/18/93

MAN=	acphy.4 adv.4 adw.4 agp.4 ahb.4 ahc.4 amhphy.4 an.4 aria.4 \
	atalk.4 audio.4 audiocs.4 auich.4 autri.4 \
	auvia.4 awi.4 bba.4 bha.4 bktr.4 bpf.4 bmtphy.4 brgphy.4 \
	bridge.4 cac.4 cardbus.4 ccd.4 cd.4 cfb.4 ch.4 clcs.4 clct.4 \
	clnp.4 clockctl.4 cltp.4 \
	cmpci.4 cms.4 cnw.4 com.4 ddb.4 de.4 dmphy.4 dpt.4 dpti.4 drum.4 eap.4 \
	ebus.4 edc.4 elmc.4 emuxki.4 en.4 envsys.4 ep.4 esh.4 esis.4 esa.4 \
	esl.4 esm.4 eso.4 exphy.4 \
	fd.4 fpa.4 fms.4 fxp.4 gentbi.4 glxtphy.4 gre.4 gphyter.4 gsip.4 hme.4 \
	icmp.4 icsphy.4 idp.4 ifmedia.4 iha.4 inet.4 inphy.4 intersil7170.4 \
	ioasic.4 ioat.4 \
	iop.4 iophy.4 iopsp.4 ip.4 ipkdb.4 iso.4 isp.4 joy.4 \
	lc.4 ld.4 lkm.4 \
	lo.4 lxtphy.4 mainbus.4 makphy.4 mbe.4 mca.4 mcclock.4 \
	md.4 mfb.4 mhzc.4 midi.4 mii.4 \
	mk48txx.4 mlx.4 mly.4 mpu.4 mtio.4 ne.4 neo.4 netintro.4 ns.4 nsip.4 \
	nsphy.4 nsphyter.4 ntwoc.4 null.4 opl.4 options.4 osiop.4 pas.4 \
	pcdisplay.4 pciide.4 pckbc.4 pckbd.4 pcn.4 pcppi.4 pcscp.4 pcweasel.4 \
	pms.4 pnaphy.4 ppp.4 pppoe.4 pty.4 puc.4 px.4 pxg.4 qsphy.4 \
	raid.4 ray.4 rcons.4 rnd.4 route.4 \
	rtk.4 sbus.4 scc.4 scsi.4 sd.4 se.4 ses.4 sf.4 sfb.4 siop.4 \
	sip.4 sl.4 sm.4 \
	speaker.4 spp.4 sqphy.4 ss.4 st.4 ste.4 stge.4 sv.4 strip.4 tb.4 \
	tc.4 tcds.4 tcp.4 \
	termios.4 tfb.4 ti.4 tl.4 tlp.4 tlphy.4 tp.4 tr.4 trm.4 tty.4 tun.4 \
	tqphy.4 twe.4 \
	udp.4 uha.4 uk.4 ukphy.4 unix.4 userconf.4 vga.4 vlan.4 vnd.4 \
	vr.4 wd.4 wdc.4 wi.4 wscons.4 wsdisplay.4 wskbd.4 wsmouse.4 \
	wsmux.4 yds.4 ym.4 zero.4 zstty.4

# USB devices
MAN+=	uaudio.4 ucom.4 udsbr.4 uftdi.4 ugen.4 uhid.4 \
	uhidev.4 uirda.4 ukbd.4 ulpt.4 \
	umass.4 umct.4 umidi.4 umodem.4 ums.4 uplcom.4 urio.4 usb.4 \
	uscanner.4 usscanner.4 ustir.4 uvisor.4 uvscom.4 uyap.4 \
	aue.4 cue.4 kue.4 upl.4 \
	ehci.4 ohci.4 uhci.4

# Ir devices
MAN+=	irframe.4 cir.4 irframetty.4 oboe.4
#	smccir.4

# Radio devices
MAN+=	radio.4 az.4 mr.4 rt.4 rtii.4 sf2r.4

# machine-independent busses
MAN+=	eisa.4 isa.4 isapnp.4 ofisa.4 pci.4 pcmcia.4

# machine-independent PCI devices
MAN+=	cz.4 epic.4 lmc.4 viaenv.4

# machine-independent ISA devices
MAN+=	aha.4 ai.4 aic.4 ast.4 ate.4 boca.4 cs.4 cy.4 ec.4 ef.4 \
	eg.4 el.4 esp.4 ess.4 ex.4 fmv.4 gus.4 guspnp.4 ix.4 iy.4 \
	le.4 lm.4 mcd.4 nca.4 pss.4 rtfps.4 sb.4 sea.4 tcom.4 \
	wds.4 we.4 wss.4 wt.4

# machine-independent PCMCIA devices
MAN+=	pcic.4 tcic.4 pcmcom.4 xi.4

# machine-independent podulebus devices
MAN+=	dtide.4 ea.4 eb.4 hcide.4

# machine-independent VME devices
MAN+=	sc.4 si.4

# IPv6/IPsec
MAN+=	faith.4 gif.4 inet6.4 icmp6.4 ip6.4 ipsec.4 stf.4

# ISDN devices
MAN+=	daic.4 isdntrc.4 isdntel.4 isdnbchan.4 ippp.4 irip.4 isdnctl.4 isdn.4 \
	ifpci.4 isic.4

MLINKS+=bha.4 bt.4
MLINKS+=cardbus.4 cbb.4
MLINKS+=fd.4 stderr.4 fd.4 stdin.4 fd.4 stdout.4
MLINKS+=fpa.4 fea.4 fpa.4 fta.4
MLINKS+=le.4 bicc.4 le.4 nele.4 le.4 depca.4
MLINKS+=netintro.4 networking.4
MLINKS+=pms.4 pmsi.4
MLINKS+=si.4 sw.4
MLINKS+=audio.4 sound.4
MLINKS+=audio.4 mixer.4
MLINKS+=midi.4 sequencer.4
MLINKS+=midi.4 music.4
MLINKS+=midi.4 rmidi.4
MLINKS+=scsi.4 atapi.4
MLINKS+=mii.4 phy.4
MLINKS+=usb.4 uhub.4
MLINKS+=ntwoc.4 ntwo.4
MLINKS+=scsi.4 scsibus.4
MLINKS+=scsi.4 atapibus.4
MLINKS+=speaker.4 spkr.4
MLINKS+=cardbus.4 cardslot.4
MLINKS+=pci.4 pchb.4
MLINKS+=pci.4 ppb.4
MLINKS+=pcppi.4 sysbeep.4
MLINKS+=pcppi.4 isabeep.4
MLINKS+=pss.4 sp.4
MLINKS+=viaenv.4 viapm.4
MLINKS+=tty.4 dty.4
MLINKS+=rcons.4 rasterconsole.4
MLINKS+=irframe.4 irda.4

# This is a hack
MLINKS+=edc.4 i386/ed.4

SUBDIR= man4.acorn26 man4.alpha man4.amiga man4.arm32 man4.atari man4.hp300 \
	man4.i386 man4.mac68k man4.macppc man4.mvme68k man4.pc532 man4.pmax \
	man4.sparc man4.sun2 man4.sun3 man4.vax man4.x68k

.include <bsd.man.mk>
.include <bsd.subdir.mk>
