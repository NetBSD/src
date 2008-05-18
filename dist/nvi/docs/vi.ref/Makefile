#	$Id: Makefile,v 1.1.1.1.2.2 2008/05/18 12:29:25 yamt Exp $ (Berkeley) $Date: 2008/05/18 12:29:25 $

MACROS=	-me
ROFF=	groff
TBL=	tbl

all: vi.ref.txt vi.ref.ps ../html/vi.ref.html

vi.ref.txt: vi.texi
	-makeinfo --no-headers vi.texi > $@ 2>/dev/null
	chmod 444 $@

vi.ref.ps: vi.texi
	texi2dvi -c vi.texi
	dvips -o $@ vi.dvi
	rm -f vi.dvi
	chmod 444 $@

../html/vi.ref.html: vi.texi
	test -d ../html || mkdir ../html
	-makeinfo --html --no-split vi.texi --output=$@ 2>/dev/null
	chmod 444 $@

clean:
	rm -f vi.ref.ps vi.ref.txt index index.so ../html/vi.ref.html
