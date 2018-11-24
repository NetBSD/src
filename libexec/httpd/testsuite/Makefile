#	$eterna: Makefile,v 1.14 2009/05/22 21:51:39 mrg Exp $

SIMPLETESTS=	t1 t2 t3 t4 t5 t6 t7 t8 t9 t10 t12 t13 t14 t15
CGITESTS=	t11
BIGFILETESTS=	partial4000 partial8000

BOZOHTTPD?=	../bozohttpd
BOZOHTTPD?=	../debug/bozohttpd-debug
WGET?=		wget
DATA?=		$(.CURDIR)/data 
VERBOSE?=	yes
HOST?=		test.eterna

.if ${VERBOSE} != "yes"
SILENT=		@
.else
SILENT=
.endif

all:

clean:
	for a in $(SIMPLETESTS) $(BIGFILETESTS); do \
		rm -f tmp.$$a.out tmp.$$a.err; \
	done

check: check-simple check-cgi check-bigfile

check-simple:
.for a in $(SIMPLETESTS)
	${SILENT}$(.CURDIR)/test-simple "$a" "${BOZOHTTPD}" "${DATA}" "${.CURDIR}" "${VERBOSE}" "${HOST}"
.endfor

check-cgi:
.for a in $(CGITESTS)
	${SILENT}$(.CURDIR)/test-simple "$a" "${BOZOHTTPD}" "${DATA}" "${.CURDIR}" "${VERBOSE}" "${HOST}" -c "${.CURDIR}/cgi-bin"
.endfor

check-bigfile:
.for a in $(BIGFILETESTS)
	${SILENT}$(.CURDIR)/test-bigfile "$a" "${BOZOHTTPD}" "${WGET}" "${DATA}" "${VERBOSE}" "${HOST}"
.endfor

.include <bsd.obj.mk>
