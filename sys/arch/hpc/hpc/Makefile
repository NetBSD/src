#	$NetBSD: Makefile,v 1.3.2.1 2002/01/10 19:43:17 thorpej Exp $

HOST_CC ?= /usr/bin/cc

test: platid_test
	./platid_test > platid_test.out
	diff  platid_test.out  platid_test.canon

platid_test: platid.c platid_mask.c platid_test.c platid_name.c
	ln -sf ../../hpcmips/include ./machine
	$(HOST_CC) -Dhpcmips -Dhpcarm -Dhpcsh -I/usr/include -I. -I../.. -DPLATID_TEST \
	    platid.c platid_mask.c platid_name.c platid_test.c -o $@

CLEANFILES+=	platid_test platid_test.out machine mips

clean:
	rm -f machine platid_test.out platid_test

.include <bsd.subdir.mk>
