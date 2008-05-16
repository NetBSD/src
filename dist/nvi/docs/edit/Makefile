# $Id: Makefile,v 1.1.1.1 2008/05/16 18:03:31 aymeric Exp $ (Berkeley) $Date: 2008/05/16 18:03:31 $

ROFF=	groff
TBL=	tbl

all: edittut.ps ../html/edittut.html

../html/%.html: %.ms
	test -d ../html || mkdir ../html
	${TBL} $< | ${ROFF} -ms -Thtml | \
	    sed 's/<img src="$<.*png">//' > $@
	rm -f "$<"*png
	chmod 444 $@

%.ps: %.ms
	${TBL} $< | ${ROFF} -ms > $@
	chmod 444 $@

clean:
	rm -f edittut.ps ../html/edittut.html
