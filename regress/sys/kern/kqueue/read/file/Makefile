# $NetBSD: Makefile,v 1.2 2002/10/12 11:18:37 jdolecek Exp $

CLEANFILES+= afile

.include <bsd.prog.mk>

regress: ${PROG}
	touch afile
	./${PROG} -n 3 afile &
	@sleep 1
	@echo foo1 >> afile && sleep 1
	@echo foo2 >> afile && sleep 1
	@echo foo3 >> afile && sleep 1
	@echo foo4 >> afile && sleep 1
	@echo EVFILT_READ for common file test done
