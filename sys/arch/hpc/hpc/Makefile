#	$NetBSD: Makefile,v 1.4 2001/09/24 14:29:30 takemura Exp $

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
