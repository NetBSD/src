#	$eterna: Makefile,v 1.11 2002/10/02 15:47:33 mrg Exp $

SIMPLETESTS=	t1 t2 t3 t4 t5 t6
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
