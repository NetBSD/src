#	$NetBSD: Makefile,v 1.3 2001/03/31 11:37:48 aymeric Exp $
#
#	@(#)Makefile	8.20 (Berkeley) 8/18/96

DIR=		usd/13.viref
SRCS=		vi.ref ex.cmd.roff set.opt.roff vi.cmd.roff ref.so
MACROS=		-me
CLEANFILES+=	vi.ref.txt vi.ref.ps index index.so

all: vi.ref.txt vi.ref.ps

vi.ref.txt: vi.ref index.so
	soelim vi.ref | ${TBL} | groff ${MACROS} -Tascii > $@
	rm -f index
	chmod 444 $@

vi.ref.ps: vi.ref index.so
	soelim vi.ref | ${TBL} | ${ROFF} ${MACROS} > $@
	rm -f index
	chmod 444 $@

index.so: vi.ref
	# Build index.so, side-effect of building the paper.
	soelim vi.ref | ${TBL} | ${ROFF} ${MACROS} > /dev/null
	sed -e 's/MINUSSIGN/\\-/' \
	    -e 's/DOUBLEQUOTE/""/' \
	    -e "s/SQUOTE/'/" \
	    -e 's/ /__SPACE/g' < index | \
	sort -u '-t	' +0 -1 +1n | awk -f merge.awk | \
	sed -e 's/__SPACE/ /g' > $@
	rm -f index
	chmod 444 $@

.include <bsd.doc.mk>
