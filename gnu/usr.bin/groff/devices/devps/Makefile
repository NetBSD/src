#	$NetBSD: Makefile,v 1.7 1999/02/13 02:54:27 lukem Exp $

DEVICE=		ps
FONTFILES=	$(FONTS) $(SUPPORT) $(DEVGENFILES) DESC
FONTS=		S ZD ZDR SS AB ABI AI AR BMB BMBI BMI BMR \
		CB CBI CI CR HB HBI HI HR HNB HNBI HNI HNR \
		NB NBI NI NR PB PBI PI PR TB TBI TI TR ZCMI
SUPPORT=	download prologue symbolsl.pfa text.enc zapfdr.pfa
DEVGENFILES=	generate/Makefile generate/afmname generate/dingbatsmap \
		generate/dingbatsrmap generate/lgreekmap generate/symbol.sed \
		generate/symbolchars generate/symbolsl.afm generate/textmap

MKOBJ=		no

clean cleandir distclean:

.include "../Makefile.dev"
