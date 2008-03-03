#	$eterna: Makefile,v 1.12 2008/03/02 09:26:01 mrg Exp $

SIMPLETESTS=	t1 t2 t3 t4 t5 t6 t7 t8 t9
BOZOHTTPD?=	../bozohttpd

all:

clean:
	for a in $(SIMPLETESTS); do \
		rm -f tmp.$$a.out; \
	done

check:
.for a in $(SIMPLETESTS)
	echo "Running test $a"
	$(BOZOHTTPD) ./data < $(.CURDIR)/$a.in > tmp.$a.out || true
	$(.CURDIR)/html_cmp $(.CURDIR)/$a.out tmp.$a.out
.endfor

.include <bsd.obj.mk>
