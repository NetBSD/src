#
# $NetBSD: Makefile,v 1.1 1996/05/15 21:06:18 is Exp $
#
AS		= as -m68040
LD		= ld

.SUFFIXES:	.o .S .sa .defs .h

.sa.S:
	sh ${.CURDIR}/asm2gas ${.IMPSRC} >${.TARGET}

060sp.o: netbsd.S inetbsd.S fnetbsd.S isp.S fpsp.S copyright.S
	$(AS) -o ${.TARGET} netbsd.S

clean:
	rm -f 060sp.o isp.S fpsp.S
