# $NetBSD: Makefile,v 1.1.1.1 2002/10/03 07:46:03 jdolecek Exp $

.include <bsd.prog.mk>

regress: ${PROG}
	for i in 0 1 2 3 4 5; do \
		./${PROG} $$i; 	\
	done
	./${PROG} 6 || true
	for i in WRITE READ AIO VNODE PROC SIGNAL; do \
		./${PROG} EVFILT_$$i; \
	done
	./${PROG} NOTREG_FILTER || true
	@echo Test ended without apparent errors
