#	$NetBSD: Makefile,v 1.3 2000/02/06 08:47:05 takemura Exp $

SUBDIR=	platid_gen

HOST_CC ?= /usr/bin/cc

test: platid_test
	./platid_test > platid_test.out
	diff  platid_test.out  platid_test.canon

platid_test: platid.c platid_mask.c platid_test.c
	ln -sf ../include ./machine
	ln -sf ../../mips/include ./mips
	$(HOST_CC) -I. -DPLATID_TEST platid.c platid_mask.c platid_test.c -o $@

CLEANFILES+=	platid_test platid_test.out machine mips

.include <bsd.subdir.mk>
