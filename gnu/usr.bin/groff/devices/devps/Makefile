#	$NetBSD: Makefile,v 1.8 2001/04/19 14:41:07 wiz Exp $

.include "../../Makefile.path"

.PATH:		${IIDIST}/font/devps

DEVICE=		ps
FONTFILES=	$(FONTS) $(SUPPORT) $(PSFILES) $(DEVGENFILES) DESC
FONTS=		S ZD ZDR SS AB ABI AI AR BMB BMBI BMI BMR \
		CB CBI CI CR HB HBI HI HR HNB HNBI HNI HNR \
		NB NBI NI NR PB PBI PI PR TB TBI TI TR ZCMI
SUPPORT=	download
PSFILES=	prologue symbolsl.pfa text.enc zapfdr.pfa
DEVGENFILES=	generate/Makefile generate/afmname generate/dingbatsmap \
		generate/dingbatsrmap generate/lgreekmap generate/symbol.sed \
		generate/symbolchars generate/symbolsl.afm generate/textmap

CLEANFILES+=	$(PSFILES) DESC

prologue: prologue.ps
zapfdr.pfa: zapfdr.ps
symbolsl.pfa: symbolsl.ps

${PSFILES}:
	@-rm -f $@
	@sed -f ${IIDIST}/font/devps/psstrip.sed $> > $@

DESC: DESC.in
	@cat $> >$@
	@echo broken $(BROKEN_SPOOLER_FLAGS) >>$@
	@if test "$(PAGE)" = A4; then \
		echo "paperlength 841890" >>$@; \
	else \
		echo "paperlength 792000" >>$@; \
	fi
	@test -z '$(PSPRINT)' || echo print '$(PSPRINT)' >>$@

.include "../Makefile.dev"
