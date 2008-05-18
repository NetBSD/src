# $Id: Makefile,v 1.1.1.1.2.2 2008/05/18 12:29:25 yamt Exp $ (Berkeley) $Date: 2008/05/18 12:29:25 $

ROFF=	groff

all: vi.0 vi.0.ps ../html/vi.0.html

vi.0: vi.1
	${ROFF} -man -Tascii < vi.1 > $@
	chmod 444 $@

vi.0.ps: vi.1
	${ROFF} -man < vi.1 > $@
	chmod 444 $@

../html/vi.0.html: vi.1
	test -d ../html || mkdir ../html
	${ROFF} -man -Thtml < vi.1 | \
	    sed 's/<img src="$<.*png">//' > $@
	rm -f "$<"*png
	chmod 444 $@

clean:
	rm -f vi.0 vi.0.ps ../html/vi.0.html
