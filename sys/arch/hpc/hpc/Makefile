#	$NetBSD: Makefile,v 1.5 2003/05/04 06:36:37 lukem Exp $

HOST_CC ?= /usr/bin/cc

test: platid_test
	./platid_test > platid_test.out
	diff  platid_test.out  platid_test.canon

platid_test: platid.c platid_mask.c platid_test.c platid_name.c
	-rm -f machine
	ln -s ../../hpcmips/include machine
	$(HOST_CC) -Dhpcmips -Dhpcarm -Dhpcsh -I/usr/include -I. -I../.. -DPLATID_TEST \
	    platid.c platid_mask.c platid_name.c platid_test.c -o $@

CLEANFILES+=	platid_test platid_test.out machine mips

clean:
	rm -f machine platid_test.out platid_test

.include <bsd.subdir.mk>
