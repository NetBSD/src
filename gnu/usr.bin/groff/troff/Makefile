#	$NetBSD: Makefile,v 1.17 2001/04/19 14:41:47 wiz Exp $

.include "../Makefile.lib"

DIST=		${IDIST}/src/roff/troff
.PATH:		${DIST}

PROG=		troff
SRCS=		column.cc dictionary.cc div.cc env.cc input.cc node.cc \
		number.cc reg.cc symbol.cc majorminor.cc
LDADD+=		$(LIBGROFF) -lm
DPADD+=		$(LIBGROFF) $(LIBM)

CLEANFILES+=	majorminor.cc
majorminor.cc: $(IDIST)/VERSION
	@echo Making $@
	@-rm -f $@
	@echo const char \*major_version = \
	\"`sed -e 's/^\([^.]*\)\..*$$/\1/' $(IDIST)/VERSION`\"\; >$@
	@echo const char \*minor_version = \
	\"`sed -e 's/^[^.]*\.\([0-9]*\).*$$/\1/' $(IDIST)/VERSION`\"\; >>$@
	@echo const char \*revision = \"`cat $(IDIST)/REVISION`\"\; >>$@

.include <bsd.own.mk>

# XXX -- this depends on ../include/defs.h
TMACDIR=/usr/share/tmac

.if ${MKSHARE} != "no"
FILES=hyphen.us
FILESDIR=${TMACDIR}
.endif

.include <bsd.prog.mk>
