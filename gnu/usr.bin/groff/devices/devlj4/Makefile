#	$NetBSD: Makefile,v 1.5 2001/04/19 14:41:00 wiz Exp $

.include "../../Makefile.path"

.PATH:		${IIDIST}/font/devlj4

DEVICE=		lj4
FONTFILES=	ALBB ALBR AOB AOI AOR CB CBI CI CR GB GBI GI GR LGB LGI LGR \
		OB OBI OI OR TB TBI TI TR UB UBI UI UR UCB UCBI UCI UCR \
		CLARENDON CORONET MARIGOLD S generate/Makefile \
		generate/text.map generate/special.map \
		DESC

CLEANFILES=	DESC

LJ4RES=		600
LJ4PRINT=	$(PSPRINT)

DESC: DESC.in
	@echo "res $(LJ4RES)" >$@
	@echo "unitwidth `expr 7620000 / $(LJ4RES)`" >>$@
	@cat $> >>$@
	@if test "$(PAGE)" = A4; then \
		echo "papersize a4" >>$@; \
	else \
		echo "papersize letter" >>$@; \
	fi
	@test -z '$(LJ4PRINT)' || echo print '$(LJ4PRINT)' >>$@

.include "../Makefile.dev"
