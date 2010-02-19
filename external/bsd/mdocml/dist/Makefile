.SUFFIXES:	.html .xml .sgml .1 .3 .7 .md5 .tar.gz .1.txt .3.txt .7.txt .1.sgml .3.sgml .7.sgml

BINDIR		= $(PREFIX)/bin
INCLUDEDIR	= $(PREFIX)/include
LIBDIR		= $(PREFIX)/lib
MANDIR		= $(PREFIX)/man
EXAMPLEDIR	= $(PREFIX)/share/examples/mandoc
INSTALL_PROGRAM	= install -m 0755
INSTALL_DATA	= install -m 0444
INSTALL_LIB	= install -m 0644
INSTALL_MAN	= $(INSTALL_DATA)

VERSION	   = 1.9.15
VDATE	   = 18 February 2010

VFLAGS     = -DVERSION="\"$(VERSION)\"" -DHAVE_CONFIG_H
WFLAGS     = -W -Wall -Wstrict-prototypes -Wno-unused-parameter -Wwrite-strings
CFLAGS    += -g $(VFLAGS) $(WFLAGS)
#CFLAGS	  += -DOSNAME="\"OpenBSD 4.5\""
LINTFLAGS += $(VFLAGS)

MANDOCFLAGS = -Wall -fstrict
MANDOCHTML = -Thtml -Ostyle=style.css,man=%N.%S.html,includes=%I.html

MDOCLNS	   = mdoc_macro.ln mdoc.ln mdoc_hash.ln mdoc_strings.ln \
	     mdoc_argv.ln mdoc_validate.ln mdoc_action.ln \
	     lib.ln att.ln arch.ln vol.ln msec.ln st.ln \
	     mandoc.ln
MDOCOBJS   = mdoc_macro.o mdoc.o mdoc_hash.o mdoc_strings.o \
	     mdoc_argv.o mdoc_validate.o mdoc_action.o lib.o att.o \
	     arch.o vol.o msec.o st.o mandoc.o
MDOCSRCS   = mdoc_macro.c mdoc.c mdoc_hash.c mdoc_strings.c \
	     mdoc_argv.c mdoc_validate.c mdoc_action.c lib.c att.c \
	     arch.c vol.c msec.c st.c mandoc.c

MANLNS	   = man_macro.ln man.ln man_hash.ln man_validate.ln \
	     man_action.ln mandoc.ln man_argv.ln
MANOBJS	   = man_macro.o man.o man_hash.o man_validate.o \
	     man_action.o mandoc.o man_argv.o
MANSRCS	   = man_macro.c man.c man_hash.c man_validate.c \
	     man_action.c mandoc.c man_argv.c

MAINLNS	   = main.ln mdoc_term.ln chars.ln term.ln tree.ln \
	     compat.ln man_term.ln html.ln mdoc_html.ln \
	     man_html.ln out.ln
MAINOBJS   = main.o mdoc_term.o chars.o term.o tree.o compat.o \
	     man_term.o html.o mdoc_html.o man_html.o out.o
MAINSRCS   = main.c mdoc_term.c chars.c term.c tree.c compat.c \
	     man_term.c html.c mdoc_html.c man_html.c out.c

LLNS	   = llib-llibmdoc.ln llib-llibman.ln llib-lmandoc.ln
LNS	   = $(MAINLNS) $(MDOCLNS) $(MANLNS)
LIBS	   = libmdoc.a libman.a
OBJS	   = $(MDOCOBJS) $(MAINOBJS) $(MANOBJS)
SRCS	   = $(MDOCSRCS) $(MAINSRCS) $(MANSRCS)
DATAS	   = arch.in att.in lib.in msec.in st.in \
	     vol.in chars.in
HEADS	   = mdoc.h libmdoc.h man.h libman.h term.h \
	     libmandoc.h html.h chars.h out.h main.h
GSGMLS	   = mandoc.1.sgml mdoc.3.sgml mdoc.7.sgml manuals.7.sgml \
	     mandoc_char.7.sgml man.7.sgml man.3.sgml
SGMLS	   = index.sgml
HTMLS	   = ChangeLog.html index.html
XSLS	   = ChangeLog.xsl
GHTMLS	   = mandoc.1.html mdoc.3.html man.3.html mdoc.7.html \
	     man.7.html mandoc_char.7.html manuals.7.html
TEXTS	   = mandoc.1.txt mdoc.3.txt man.3.txt mdoc.7.txt man.7.txt \
	     mandoc_char.7.txt manuals.7.txt ChangeLog.txt
EXAMPLES   = example.style.css
XMLS	   = ChangeLog.xml
STATICS	   = index.css style.css external.png
MD5S	   = mdocml-$(VERSION).md5 
TARGZS	   = mdocml-$(VERSION).tar.gz
MANS	   = mandoc.1 mdoc.3 mdoc.7 manuals.7 mandoc_char.7 \
	     man.7 man.3
BINS	   = mandoc
TESTS	   = test-strlcat.c test-strlcpy.c
CONFIGS	   = config.h.pre config.h.post
CLEAN	   = $(BINS) $(LNS) $(LLNS) $(LIBS) $(OBJS) $(HTMLS) \
	     $(TARGZS) tags $(MD5S) $(XMLS) $(TEXTS) $(GSGMLS) \
	     $(GHTMLS) config.h config.log
INSTALL	   = $(SRCS) $(HEADS) Makefile $(MANS) $(SGMLS) $(STATICS) \
	     $(DATAS) $(XSLS) $(EXAMPLES) $(TESTS) $(CONFIGS)

all:	$(BINS)

lint:	$(LLNS)

clean:
	rm -f $(CLEAN)

cleanlint:
	rm -f $(LNS) $(LLNS)

cleanhtml:
	rm -f $(HTMLS) $(GSGMLS) $(GHTMLS)

dist:	mdocml-$(VERSION).tar.gz

www:	all $(GSGMLS) $(GHTMLS) $(HTMLS) $(TEXTS) $(MD5S) $(TARGZS)

htmls:	all $(GSGMLS) $(GHTMLS)

installwww: www
	install -m 0444 $(GHTMLS) $(HTMLS) $(TEXTS) $(STATICS) $(PREFIX)/
	install -m 0444 mdocml-$(VERSION).tar.gz $(PREFIX)/snapshots/
	install -m 0444 mdocml-$(VERSION).md5 $(PREFIX)/snapshots/
	install -m 0444 mdocml-$(VERSION).tar.gz $(PREFIX)/snapshots/mdocml.tar.gz
	install -m 0444 mdocml-$(VERSION).md5 $(PREFIX)/snapshots/mdocml.md5

install:
	mkdir -p $(BINDIR)
	mkdir -p $(EXAMPLEDIR)
	mkdir -p $(MANDIR)/man1
	mkdir -p $(MANDIR)/man7
	$(INSTALL_PROGRAM) mandoc $(BINDIR)
	$(INSTALL_MAN) mandoc.1 $(MANDIR)/man1
	$(INSTALL_MAN) man.7 mdoc.7 $(MANDIR)/man7
	$(INSTALL_DATA) example.style.css $(EXAMPLEDIR)

uninstall:
	rm -f $(BINDIR)/mandoc
	rm -f $(MANDIR)/man1/mandoc.1
	rm -f $(MANDIR)/man7/mdoc.7
	rm -f $(MANDIR)/man7/man.7
	rm -f $(EXAMPLEDIR)/example.style.css

$(OBJS): config.h

$(LNS): config.h

man_macro.ln: man_macro.c libman.h
man_macro.o: man_macro.c libman.h

lib.ln: lib.c lib.in libmdoc.h
lib.o: lib.c lib.in libmdoc.h

att.ln: att.c att.in libmdoc.h
att.o: att.c att.in libmdoc.h

arch.ln: arch.c arch.in libmdoc.h
arch.o: arch.c arch.in libmdoc.h

vol.ln: vol.c vol.in libmdoc.h
vol.o: vol.c vol.in libmdoc.h

chars.ln: chars.c chars.in chars.h
chars.o: chars.c chars.in chars.h

msec.ln: msec.c msec.in libmdoc.h
msec.o: msec.c msec.in libmdoc.h

st.ln: st.c st.in libmdoc.h
st.o: st.c st.in libmdoc.h

mdoc_macro.ln: mdoc_macro.c libmdoc.h
mdoc_macro.o: mdoc_macro.c libmdoc.h

mdoc_term.ln: mdoc_term.c term.h mdoc.h
mdoc_term.o: mdoc_term.c term.h mdoc.h

mdoc_strings.ln: mdoc_strings.c libmdoc.h
mdoc_strings.o: mdoc_strings.c libmdoc.h

man_hash.ln: man_hash.c libman.h
man_hash.o: man_hash.c libman.h

mdoc_hash.ln: mdoc_hash.c libmdoc.h
mdoc_hash.o: mdoc_hash.c libmdoc.h

mdoc.ln: mdoc.c libmdoc.h
mdoc.o: mdoc.c libmdoc.h

man.ln: man.c libman.h
man.o: man.c libman.h

main.ln: main.c mdoc.h
main.o: main.c mdoc.h

compat.ln: compat.c 
compat.o: compat.c

term.ln: term.c term.h man.h mdoc.h chars.h
term.o: term.c term.h man.h mdoc.h chars.h

html.ln: html.c html.h chars.h
html.o: html.c html.h chars.h

mdoc_html.ln: mdoc_html.c html.h mdoc.h
mdoc_html.o: mdoc_html.c html.h mdoc.h

man_html.ln: man_html.c html.h man.h out.h
man_html.o: man_html.c html.h man.h out.h

out.ln: out.c out.h
out.o: out.c out.h

tree.ln: tree.c man.h mdoc.h
tree.o: tree.c man.h mdoc.h

mdoc_argv.ln: mdoc_argv.c libmdoc.h
mdoc_argv.o: mdoc_argv.c libmdoc.h

man_argv.ln: man_argv.c libman.h
man_argv.o: man_argv.c libman.h

man_validate.ln: man_validate.c libman.h
man_validate.o: man_validate.c libman.h

mdoc_validate.ln: mdoc_validate.c libmdoc.h
mdoc_validate.o: mdoc_validate.c libmdoc.h

mdoc_action.ln: mdoc_action.c libmdoc.h
mdoc_action.o: mdoc_action.c libmdoc.h

libmdoc.h: mdoc.h

ChangeLog.xml:
	cvs2cl --xml --xml-encoding iso-8859-15 -t --noxmlns -f $@

ChangeLog.txt:
	cvs2cl -t -f $@

ChangeLog.html: ChangeLog.xml ChangeLog.xsl
	xsltproc -o $@ ChangeLog.xsl ChangeLog.xml

mdocml-$(VERSION).tar.gz: $(INSTALL)
	mkdir -p .dist/mdocml/mdocml-$(VERSION)/
	cp -f $(INSTALL) .dist/mdocml/mdocml-$(VERSION)/
	( cd .dist/mdocml/ && tar zcf ../../$@ mdocml-$(VERSION)/ )
	rm -rf .dist/

llib-llibmdoc.ln: $(MDOCLNS)
	$(LINT) -Clibmdoc $(MDOCLNS)

llib-llibman.ln: $(MANLNS)
	$(LINT) -Clibman $(MANLNS)

llib-lmandoc.ln: $(MAINLNS) llib-llibmdoc.ln
	$(LINT) -Cmandoc $(MAINLNS) llib-llibmdoc.ln

libmdoc.a: $(MDOCOBJS)
	$(AR) rs $@ $(MDOCOBJS)

libman.a: $(MANOBJS)
	$(AR) rs $@ $(MANOBJS)

mandoc: $(MAINOBJS) libmdoc.a libman.a
	$(CC) $(CFLAGS) -o $@ $(MAINOBJS) libmdoc.a libman.a

.sgml.html:
	validate --warn $<
	sed -e "s!@VERSION@!$(VERSION)!" -e "s!@VDATE@!$(VDATE)!" $< > $@

.1.1.txt .3.3.txt .7.7.txt:
	./mandoc $(MANDOCFLAGS) $< | col -b > $@

.1.1.sgml .3.3.sgml .7.7.sgml:
	./mandoc $(MANDOCFLAGS) $(MANDOCHTML) $< > $@

.tar.gz.md5:
	md5 $< > $@

config.h: config.h.pre config.h.post
	rm -f config.log
	( cat config.h.pre; \
	echo; \
	if $(CC) $(CFLAGS) -Werror -c test-strlcat.c >> config.log 2>&1; then \
		echo '#define HAVE_STRLCAT'; \
		rm test-strlcat.o; \
	fi; \
	if $(CC) $(CFLAGS) -Werror -c test-strlcpy.c >> config.log 2>&1; then \
		echo '#define HAVE_STRLCPY'; \
		rm test-strlcpy.o; \
	fi; \
	echo; \
	cat config.h.post \
	) > $@
