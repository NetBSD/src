## Process this file with automake to produce Makefile.in

# Package: am-utils
# Level: Makefile for doc/ directory
# Author: Erez Zadok

@SET_MAKE@

info_TEXINFOS = am-utils.texi

EXTRA_DIST = hlfsd.ps

# include dvi/ps in distribution
dist-hook: am-utils.dvi am-utils.ps
	cp -p $^ $(distdir)

# a special target for making Postscript files
ps: am-utils.dvi am-utils.ps

am-utils-clean: clean maintainer-clean-aminfo maintainer-clean-vti

# personal rule for Erez Zadok to generate and install html docs
alldocs: ps install-NEWS install-INSTALL install-ChangeLog install-AUTHORS install-tasks install-autofs install-y2k install-BUGS install-html install-ps

install-html: $(HOME)/html/am-utils/am-utils_1.html
$(HOME)/html/am-utils/am-utils_1.html: am-utils.texi
	texi2html -debug -verbose -expandinfo -number -split_chapter $<
	mv *.html $(HOME)/html/am-utils
	chmod -R a+r $(HOME)/html/am-utils

install-ps: $(HOME)/html/am-utils/am-utils.ps
$(HOME)/html/am-utils/am-utils.ps: am-utils.ps
	install -c -m 644 $< $@
	(cd $(HOME)/html/research && ${MAKE} ${MFLAGS})

install-NEWS: $(HOME)/html/am-utils/NEWS.txt
$(HOME)/html/am-utils/NEWS.txt: $(top_srcdir)/NEWS
	install -c -m 644 $< $@

install-INSTALL: $(HOME)/html/am-utils/INSTALL.txt
$(HOME)/html/am-utils/INSTALL.txt: $(top_srcdir)/INSTALL
	install -c -m 644 $< $@

install-ChangeLog: $(HOME)/html/am-utils/ChangeLog.txt
$(HOME)/html/am-utils/ChangeLog.txt: $(top_srcdir)/ChangeLog
	install -c -m 644 $< $@

install-AUTHORS: $(HOME)/html/am-utils/AUTHORS.txt
$(HOME)/html/am-utils/AUTHORS.txt: $(top_srcdir)/AUTHORS
	install -c -m 644 $< $@

install-tasks: $(HOME)/html/am-utils/tasks.txt
$(HOME)/html/am-utils/tasks.txt: $(top_srcdir)/tasks
	install -c -m 644 $< $@

install-autofs: $(HOME)/html/am-utils/autofs.txt
$(HOME)/html/am-utils/autofs.txt: $(top_srcdir)/README.autofs
	install -c -m 644 $< $@

install-y2k: $(HOME)/html/am-utils/y2k.txt
$(HOME)/html/am-utils/y2k.txt: $(top_srcdir)/README.y2k
	install -c -m 644 $< $@

install-BUGS: $(HOME)/html/am-utils/BUGS.txt
$(HOME)/html/am-utils/BUGS.txt: $(top_srcdir)/BUGS
	install -c -m 644 $< $@
