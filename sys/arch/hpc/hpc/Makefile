#	$NetBSD: Makefile,v 1.2 2001/05/01 13:24:02 enami Exp $

SUBDIR=	platid_gen

HOST_CC ?= /usr/bin/cc

test: platid_test
	./platid_test > platid_test.out
	diff  platid_test.out  platid_test.canon

platid_test: platid.c platid_mask.c platid_test.c platid_name.c
	ln -sf ./ ./machine
	ln -sf ../../mips/include ./mips
	$(HOST_CC) -I. -DPLATID_TEST \
	    platid.c platid_mask.c platid_name.c platid_test.c -o $@

CLEANFILES+=	platid_test platid_test.out machine mips

clean:
	rm -f machine mips platid_test.out platid_test

.include <bsd.subdir.mk>
