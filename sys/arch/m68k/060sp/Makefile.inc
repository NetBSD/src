#
# $NetBSD: Makefile.inc,v 1.3 1997/09/28 20:49:56 is Exp $
#
SPDIR		= $S/arch/m68k/060sp

M060SPAS	= as -m68060 -I${SPDIR} -o ${.TARGET} ${.IMPSRC}

netbsd.o:	${SPDIR}/netbsd.S .NOTMAIN

$(SPDIR)/netbsd060sp.S:	.NOTMAIN $(SPDIR)/inetbsd.S $(SPDIR)/fnetbsd.S
$(SPDIR)/inetbsd.S:	.NOTMAIN isp.S
$(SPDIR)/fnetbsd.S:	.NOTMAIN fpsp.S
$(SPDIR)/pfnetbsd.S:	.NOTMAIN pfpsp.S

isp.S:	.NOTMAIN ${SPDIR}/isp.sa
	sh ${SPDIR}/asm2gas ${SPDIR}/isp.sa >${.TARGET}

fpsp.S:	.NOTMAIN ${SPDIR}/fpsp.sa
	sh ${SPDIR}/asm2gas ${SPDIR}/fpsp.sa >${.TARGET}

pfpsp.S: .NOTMAIN ${SPDIR}/pfpsp.sa
	sh ${SPDIR}/asm2gas ${SPDIR}/pfpsp.sa >${.TARGET}
