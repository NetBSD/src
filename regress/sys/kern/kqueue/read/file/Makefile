# $NetBSD: Makefile,v 1.1.1.1 2002/10/03 07:46:03 jdolecek Exp $

CLEANFILES+= afile

.include <bsd.prog.mk>

regress: ${PROG}
	touch afile
	./${PROG} afile &
	@echo foo1 >> afile && sleep 1
	@echo foo2 >> afile && sleep 1
	@echo foo3 >> afile && sleep 1
	@echo foo4 >> afile && sleep 1
	@echo EVFILT_READ for common file test done
