#	$NetBSD: Makefile,v 1.92 1999/07/01 10:55:53 itojun Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/18/93

MAN=	ahb.4 ahc.4 aria.4 atalk.4 audio.4 bha.4 bpf.4 ccd.4 cd.4 ch.4 \
	clnp.4 cltp.4 ddb.4 drum.4 eap.4 eisa.4 en.4 ep.4 esh.4 esis.4 \
	exphy.4 fd.4 fpa.4 fxp.4 gre.4 icmp.4 icsphy.4 idp.4 ifmedia.4 \
	inet.4 inphy.4 ip.4 ipip.4 iso.4 isp.4 lc.4 lkm.4 lo.4 lxtphy.4 md.4 \
	midi.4 mii.4 mtio.4 ncr.4 ne.4 netintro.4 ns.4 nsip.4 nsphy.4 \
	null.4 opl.4 options.4 pcdisplay.4 pciide.4 pckbc.4 pckbd.4 pcppi.4 \
	pcscp.4 pms.4 ppp.4 pty.4 puc.4 qsphy.4 raid.4 rnd.4 route.4 rtpphy.4 \
	scsi.4 sd.4 sl.4 sm.4 spp.4 sqphy.4 ss.4 st.4 sv.4 strip.4 tb.4 tcp.4 \
	termios.4 tl.4 tlphy.4 tp.4 tr.4 tty.4 tun.4 udp.4 uha.4 uk.4 unix.4 \
	vga.4 vnd.4 wd.4 wdc.4 wscons.4 wsdisplay.4 wskbd.4 wsmouse.4 ym.4 \
	zstty.4

# USB devices
MAN+=	uaudio.4 ugen.4 uhid.4 ukbd.4 ulpt.4 ums.4 usb.4

# machine-independent buses
MAN+=	eisa.4 isa.4 isapnp.4 pci.4 pcmcia.4

# machine-independent ISA devices
MAN+=	aha.4 aic.4 ast.4 boca.4 cy.4 ec.4 eg.4 el.4 esp.4 ess.4 \
	gus.4 guspnp.4 iy.4 mcd.4 pss.4 rtfps.4 sb.4 sea.4 tcom.4 \
	wds.4 we.4 wss.4 wt.4

# machine-independent ISA devices
MAN+=	ex.4

# machine-independent VME devices
MAN+=	si.4

# IPv6/IPsec
MAN+=	gif.4 ipsec.4 inet6.4

MLINKS+=bha.4 bt.4
MLINKS+=fd.4 stderr.4 fd.4 stdin.4 fd.4 stdout.4
MLINKS+=fpa.4 fea.4 fpa.4 fta.4
MLINKS+=netintro.4 networking.4
MLINKS+=pms.4 pmsi.4
MLINKS+=si.4 sw.4
MLINKS+=audio.4 sound.4
MLINKS+=audio.4 mixer.4

SUBDIR=	man4.amiga man4.arm32 man4.atari man4.hp300 man4.i386 \
	man4.mac68k man4.pc532 man4.pmax man4.sparc man4.sun3 \
	man4.vax man4.x68k

.include <bsd.man.mk>
.include <bsd.subdir.mk>
