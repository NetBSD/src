#	$NetBSD: Makefile,v 1.40 1997/04/27 09:09:51 jonathan Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/18/93

MAN=	ahb.4 ahc.4 audio.4 bha.4 bpf.4 ccd.4 cd.4 clnp.4 cltp.4 ddb.4 \
	drum.4 eisa.4 esis.4 ep.4 fd.4 fpa.4 icmp.4 idp.4 imp.4 inet.4 ip.4 \
	iso.4 lkm.4 lo.4 md.4 ncr.4 netintro.4 ns.4 nsip.4 null.4 \
	options.4 ppp.4 pty.4 route.4 sd.4 sl.4 spp.4 st.4 \
	strip.4 tb.4 tcp.4 termios.4 tp.4 tty.4 tun.4 udp.4 uha.4 uk.4 \
	unix.4 vnd.4

# machine-independent buses
MAN+=	eisa.4 isa.4 isapnp.4 pci.4

# machine-independent ISA devices
MAN+=	aha.4 aic.4 ast.4 boca.4 cy.4 eg.4 el.4 gus.4 iy.4 mcd.4 pss.4	\
	rtfps.4 sb.4 sea.4 wds.4 wss.4 wt.4

MLINKS+=bha.4 bt.4
MLINKS+=fd.4 stderr.4 fd.4 stdin.4 fd.4 stdout.4
MLINKS+=fpa.4 fea.4
MLINKS+=netintro.4 networking.4

SUBDIR=	man4.amiga man4.arm32 man4.atari man4.hp300 man4.i386 \
	man4.mac68k man4.pc532 man4.pmax man4.sparc man4.sun3 \
	man4.vax

.include <bsd.prog.mk>
