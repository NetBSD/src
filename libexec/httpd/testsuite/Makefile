#	$eterna: Makefile,v 1.13 2008/03/04 04:46:38 mrg Exp $

SIMPLETESTS=	t1 t2 t3 t4 t5 t6 t7 t8 t9 t10
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
