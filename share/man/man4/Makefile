#	$NetBSD: Makefile,v 1.495 2009/08/09 06:40:09 kiyohara Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/18/93

MAN=	aac.4 ac97.4 acardide.4 aceride.4 acphy.4 acpidalb.4 \
	adbbt.4 adbkbd.4 adbms.4 \
	adc.4 admtemp.4 adv.4 adw.4 age.4 agp.4 agr.4 ahb.4 ahc.4 \
	ahcisata.4 ahd.4 \
	aiboost.4 ale.4 alipm.4 altmem.4 amdpm.4 amdtemp.4 amhphy.4 \
	amr.4 aps.4 \
	an.4 arcmsr.4 aria.4 artsata.4 ata.4 atalk.4 ataraid.4 \
	ath.4 atphy.4 atppc.4 attimer.4 atw.4 \
	auacer.4 audio.4 audiocs.4 auich.4 \
	auixp.4 autri.4 auvia.4 awi.4 azalia.4 \
	battery_pmu.4 bba.4 bce.4 bcsp.4 be.4 bge.4 bnx.4 bha.4 \
	bio.4 bktr.4 bluetooth.4 bmtphy.4 bpf.4 \
	brgphy.4 bridge.4 bthidev.4 bthub.4 btkbd.4 btms.4 btsco.4 btuart.4 \
	bwi.4 \
	cac.4 cardbus.4 carp.4 ccd.4 cd.4 \
	cec.4 cgd.4 cfb.4 ch.4 chipsfb.4 ciphy.4 ciss.4 clcs.4 clct.4 clnp.4 \
	clockctl.4 cltp.4 cmdide.4 cmpci.4 cms.4 cnw.4 \
	com.4 coretemp.4 crypto.4 cs80bus.4 cuda.4 cypide.4 \
	ddb.4 ddc.4 de.4 dge.4 dk.4 dmoverio.4 \
	dmphy.4 dpt.4 dpti.4 drm.4 drum.4 \
	eap.4 ebus.4 edc.4 elmc.4 emuxki.4 en.4 envsys.4 ep.4 esh.4 esis.4 \
	esa.4 esiop.4 esm.4 eso.4 etherip.4 exphy.4 \
	fast_ipsec.4 fd.4 finsio.4 fpa.4 fms.4 fss.4 fxp.4 \
	gcscaudio.4 gem.4 genfb.4 gentbi.4 geodeide.4 \
	glxtphy.4 gpib.4 gpio.4 gpiosim.4 gre.4 gphyter.4 gsip.4 hifn.4 hme.4 \
	hpqlb.4 hptide.4 \
	ichlpcib.4 ichsmb.4 icmp.4 icp.4 icsphy.4 iee.4 ieee80211.4 \
	ifmedia.4 igsfb.4 iha.4 iic.4 inet.4 ikphy.4 inphy.4 intersil7170.4 \
	ioasic.4 ioat.4 iop.4 iophy.4 iopsp.4 ip.4 ipkdb.4 ipmi.4 ipw.4 \
	iso.4 isp.4 isv.4 itesio.4 iteide.4 iwi.4 iwn.4 ixpide.4 \
	jme.4 jmide.4 joy.4 \
	kloader.4 kse.4 ksyms.4 kttcp.4 \
	lc.4 ld.4 lii.4 lo.4 lxtphy.4 \
	mainbus.4 makphy.4 mbe.4 mca.4 mcclock.4 md.4 mfb.4 mfi.4 mhzc.4 \
	midi.4 mii.4 mk48txx.4 mlx.4 mly.4 mpt.4 mpu.4 mtd.4 mtio.4 \
	multicast.4 mvsata.4 \
	nadb.4 ne.4 neo.4 netintro.4 nfe.4 nfsmb.4 njata.4 njs.4 \
	nsclpcsio.4 nsp.4 nsphy.4 nsphyter.4 ntwoc.4 null.4 nsmb.4 \
	oak.4 oosiop.4 opl.4 options.4 optiide.4 osiop.4 \
	pad.4 pas.4 pcdisplay.4 pciide.4 pckbc.4 pckbd.4 pcn.4 pcppi.4 \
	pcscp.4 pcweasel.4 pdcide.4 pdcsata.4 piixide.4 piixpcib.4 piixpm.4 \
	pim.4 plip.4 pms.4 pmu.4 pnaphy.4 ppbus.4 ppp.4 pppoe.4 pseye.4 \
	ptm.4 pty.4 puc.4 pud.4 puffs.4 px.4 pxg.4 \
	qe.4 qec.4 qsphy.4 \
	raid.4 ral.4 ray.4 rcons.4 re.4 rgephy.4 rlphy.4 rnd.4 route.4 \
	rs5c372rtc.4 rtk.4 rtw.4 rum.4 \
	satalink.4 sbus.4 scc.4 scsi.4 sd.4 se.4 seeprom.4 sem.4 \
	ses.4 sf.4 sfb.4 sgsmix.4 shb.4 \
	shpcic.4 siisata.4 siop.4 sip.4 siside.4 sk.4 sl.4 slide.4 \
	sm.4 smsh.4 sn.4 sony.4 spc.4 speaker.4 spif.4 sqphy.4 ss.4 \
	st.4 ste.4 stge.4 sti.4 stpcide.4 sv.4 strip.4 \
	svwsata.4 swwdog.4 \
	tap.4 tc.4 tcds.4 tcp.4 termios.4 tfb.4 thinkpad.4 \
	ti.4 tl.4 tlp.4 tlphy.4 \
	tp.4 tr.4 tra.4 trm.4 tty.4 tun.4 tqphy.4 twa.4 twe.4 txp.4 \
	ubsec.4 udp.4 uep.4 ug.4 uha.4 uk.4 ukphy.4 unix.4 userconf.4 \
	veriexec.4 vga.4 vge.4 viaide.4 video.4 vlan.4 vmmon.4 vmnet.4 \
	vnd.4 vr.4 \
	wapbl.4 wd.4 wdc.4 wi.4 wm.4 wpi.4 wscons.4 wsdisplay.4 wsfont.4 \
	wskbd.4 wsmouse.4 wsmux.4 \
	xbox.4 xge.4 \
	yds.4 ym.4 \
	zero.4 zstty.4 zyd.4

# USB devices
MAN+=	stuirda.4 u3g.4 uaudio.4 uberry.4 ubsa.4 ubt.4 uchcom.4 ucom.4 ucycom.4 \
	udsbr.4 uftdi.4 ugen.4 ugensa.4 uhid.4 \
	uhidev.4 uhmodem.4 uipaq.4 uirda.4 ukbd.4 ukyopon.4 ulpt.4 \
	umass.4 umct.4 umidi.4 umodem.4 ums.4 uplcom.4 urio.4 usb.4 \
	uscanner.4 uslsa.4 usscanner.4 ustir.4 uvisor.4 uvscom.4 uyap.4 \
	aue.4 atu.4 axe.4 cdce.4 cue.4 kue.4 upl.4 url.4 udav.4 \
	ehci.4 ohci.4 slhci.4 uhci.4 utoppy.4 uvideo.4

# Ir devices
MAN+=	irframe.4 cir.4 irframetty.4 oboe.4
#	smccir.4

# ACPI devices
MAN+=	acpi.4 acpiacad.4 acpibat.4 acpibut.4 acpiec.4 acpilid.4 \
	acpitz.4

# Radio devices
MAN+=	radio.4 az.4 mr.4 rt.4 rtii.4 sf2r.4

# machine-independent busses
MAN+=	eisa.4 isa.4 isapnp.4 ofisa.4 pci.4 pcmcia.4
MAN+=	podulebus.4 ieee1394if.4 spi.4

# machine-independent PCI devices
MAN+=	cz.4 epic.4 lmc.4 viaenv.4

# machine-independent ISA devices
MAN+=	aha.4 ai.4 aic.4 ast.4 ate.4 boca.4 cs.4 cy.4 ec.4 ef.4 \
	eg.4 el.4 esp.4 ess.4 ex.4 fmv.4 gus.4 guspnp.4 ix.4 iy.4 \
	le.4 lm.4 mcd.4 nca.4 rtfps.4 sb.4 sea.4 smsc.4 tcom.4 \
	wds.4 we.4 wss.4 wt.4

# machine-independent PCMCIA devices
MAN+=	bt3c.4 btbc.4 pcic.4 tcic.4 pcmcom.4 xi.4 xirc.4

# machine-independent obio (mac68k and macppc) devices
MAN+=	adb.4 akbd.4 ams.4 mc.4

# machine-independent podulebus devices
MAN+=	dtide.4 ea.4 eb.4 ei.4 hcide.4 sec.4

# machine-independent VME devices
MAN+=	sc.4 si.4

# machine-independent IEEE1394 devices
MAN+=	fwohci.4 fwip.4 sbp.4

# machine-independent I2C devices
MAN+=	dbcool.4 lmtemp.4 sdtemp.4 spdmem.4

# machine-independent SPI devices
MAN +=	m25p.4 tm121temp.4

# machine-independent SD/MMC devices
MAN +=	sbt.4 sdhc.4 sdmmc.4

# IPv6/IPsec
MAN+=	faith.4 gif.4 inet6.4 icmp6.4 ip6.4 ipsec.4 stf.4

# ISDN devices
MAN+=	daic.4 isdntrc.4 isdntel.4 isdnbchan.4 ippp.4 irip.4 isdnctl.4 isdn.4 \
	ifpci.4 isic.4 iwic.4 isdncapi.4 iavc.4

# onewire bus and devices
MAN+=	gpioow.4 onewire.4 owtemp.4

# DRM devices
MLINKS+=drm.4 i915drm.4
MLINKS+=drm.4 mach64drm.4
MLINKS+=drm.4 mgadrm.4
MLINKS+=drm.4 r128drm.4
MLINKS+=drm.4 radeondrm.4
MLINKS+=drm.4 savagedrm.4
MLINKS+=drm.4 sisdrm.4
MLINKS+=drm.4 tdfxdrm.4
MLINKS+=drm.4 viadrm.4

MLINKS+=ata.4 atabus.4
MLINKS+=audio.4 audioctl.4
MLINKS+=audio.4 mixer.4
MLINKS+=audio.4 sound.4
MLINKS+=bha.4 bt.4
MLINKS+=bktr.4 tuner.4
MLINKS+=bktr.4 vbi.4
MLINKS+=cardbus.4 cardslot.4
MLINKS+=cardbus.4 cbb.4
MLINKS+=crypto.4 swcrypto.4
MLINKS+=dbcool.4 dbCool.4
MLINKS+=dbcool.4 adm1027.4
MLINKS+=dbcool.4 adm1030.4
MLINKS+=dbcool.4 adt7463.4
MLINKS+=dbcool.4 adt7466.4
MLINKS+=dbcool.4 adt7467.4
MLINKS+=dbcool.4 adt7468.4
MLINKS+=dbcool.4 adt7473.4
MLINKS+=dbcool.4 adt7475.4
MLINKS+=dbcool.4 adt7476.4
MLINKS+=fd.4 stderr.4 fd.4 stdin.4 fd.4 stdout.4
MLINKS+=fpa.4 fea.4 fpa.4 fta.4
MLINKS+=icp.4 icpsp.4
MLINKS+=irframe.4 irda.4
MLINKS+=le.4 bicc.4 le.4 nele.4 le.4 depca.4
MLINKS+=midi.4 music.4
MLINKS+=midi.4 rmidi.4
MLINKS+=midi.4 sequencer.4
MLINKS+=mii.4 phy.4
MLINKS+=netintro.4 networking.4
MLINKS+=nfsmb.4 nfsmbc.4
MLINKS+=nsmb.4 netsmb.4
MLINKS+=ntwoc.4 ntwo.4
MLINKS+=pci.4 pchb.4
MLINKS+=pci.4 ppb.4
MLINKS+=pcppi.4 isabeep.4
MLINKS+=pcppi.4 sysbeep.4
MLINKS+=pms.4 pmsi.4
MLINKS+=rcons.4 rasterconsole.4
MLINKS+=rnd.4 random.4
MLINKS+=rnd.4 urandom.4
MLINKS+=scsi.4 atapi.4
MLINKS+=scsi.4 atapibus.4
MLINKS+=scsi.4 scsibus.4
MLINKS+=si.4 sw.4
MLINKS+=sk.4 skc.4 sk.4 msk.4 sk.4 mskc.4
MLINKS+=sl.4 slip.4
MLINKS+=speaker.4 spkr.4
MLINKS+=tty.4 dty.4
MLINKS+=url.4 urlphy.4
MLINKS+=usb.4 uhub.4
MLINKS+=usb.4 usbtask.4
MLINKS+=wsmux.4 wsmuxctl.4
MLINKS+=zstty.4 zs.4 zstty.4 zsc.4

# This is a hack
MLINKS+=edc.4 i386/ed.4

SUBDIR= man4.acorn26 man4.acorn32 man4.alpha man4.amiga man4.arc man4.atari \
	man4.cobalt man4.dreamcast man4.evbarm man4.evbmips man4.evbppc \
	man4.hp300 man4.hp700 man4.hpcarm man4.hpcmips man4.hpcsh man4.i386 \
	man4.mac68k man4.macppc man4.mvme68k man4.pmax man4.prep \
	man4.sgimips man4.sparc man4.sparc64 man4.sun2 man4.sun3 man4.vax \
	man4.x68k

.include <bsd.man.mk>
.include <bsd.subdir.mk>
