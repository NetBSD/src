#	$NetBSD: Makefile,v 1.4 2000/05/20 03:41:28 shin Exp $

SUBDIR=	platid_gen

HOST_CC ?= /usr/bin/cc

test: platid_test
	./platid_test > platid_test.out
	diff  platid_test.out  platid_test.canon

platid_test: platid.c platid_mask.c platid_test.c platid_name.c
	ln -sf ../include ./machine
	ln -sf ../../mips/include ./mips
	$(HOST_CC) -I. -DPLATID_TEST platid.c platid_mask.c platid_name.c platid_test.c -o $@

CLEANFILES+=	platid_test platid_test.out machine mips

.include <bsd.subdir.mk>
