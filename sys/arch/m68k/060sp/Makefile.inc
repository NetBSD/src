#
# $NetBSD: Makefile.inc,v 1.5 1999/02/22 21:25:05 is Exp $
#
SPDIR		= $S/arch/m68k/060sp

M060SPAS	= $(AS) -m68060 -I${SPDIR} -o ${.TARGET} ${.IMPSRC}

netbsd.o:	${SPDIR}/netbsd.S .NOTMAIN

$(SPDIR)/netbsd060sp.S:	.NOTMAIN $(SPDIR)/inetbsd.S $(SPDIR)/fnetbsd.S
$(SPDIR)/inetbsd.S:	.NOTMAIN $(SPDIR)/isp.S
$(SPDIR)/fnetbsd.S:	.NOTMAIN $(SPDIR)/fpsp.S
$(SPDIR)/pfnetbsd.S:	.NOTMAIN $(SPDIR)/pfpsp.S
