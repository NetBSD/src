#	$NetBSD: Makefile,v 1.2 2002/09/19 03:09:40 lukem Exp $

.include "../../Makefile.path"

.PATH:		${IDIST}/font/devlbp

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
