#	$NetBSD: Makefile,v 1.1.1.1 2001/04/19 14:33:07 wiz Exp $

.include "../../Makefile.path"

.PATH:		${IIDIST}/font/devlbp

DEVICE=		lbp
FONTFILES=	DESC HB HBI HI HR TB TBI TI TR

CLEANFILES=	DESC

LBPPRINT=	$(PSPRINT)

DESC: DESC.in 
	@cat $> >>$@
	if test "$(PAGE)" = A4; then \
  		echo "papersize a4" >>$@; \
	else \
		echo "papersize letter" >>$@; \
	fi
	test -z '$(LBPPRINT)' || echo print '$(LBPPRINT)' >>$@

.include "../Makefile.dev"
