#
# $NetBSD: Makefile.inc,v 1.5.8.1 2000/12/08 09:28:15 bouyer Exp $
#
SPDIR		= $S/arch/m68k/060sp

M060SPAS	= $(CC) ${AFLAGS} -m68060 ${CPPFLAGS} -I${SPDIR} \
		-c -o ${.TARGET} ${.IMPSRC}

netbsd.o:	${SPDIR}/netbsd.S .NOTMAIN

$(SPDIR)/netbsd060sp.S:	.NOTMAIN $(SPDIR)/inetbsd.S $(SPDIR)/fnetbsd.S
$(SPDIR)/inetbsd.S:	.NOTMAIN $(SPDIR)/isp.S
$(SPDIR)/fnetbsd.S:	.NOTMAIN $(SPDIR)/fpsp.S
$(SPDIR)/pfnetbsd.S:	.NOTMAIN $(SPDIR)/pfpsp.S
