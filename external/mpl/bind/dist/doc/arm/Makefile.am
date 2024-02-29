include $(top_srcdir)/Makefile.top
include $(top_srcdir)/Makefile.docs

EXTRA_DIST =					\
	advanced.inc.rst			\
	build.inc.rst				\
	catz.inc.rst				\
	chapter10.rst				\
	chapter1.rst				\
	chapter2.rst				\
	chapter3.rst				\
	chapter4.rst				\
	chapter5.rst				\
	chapter6.rst				\
	chapter7.rst				\
	chapter9.rst				\
	config-auth.inc.rst			\
	config-intro.inc.rst			\
	config-resolve.inc.rst			\
	conf.py					\
	dlz.inc.rst				\
	dns-ops.inc.rst				\
	dnssec-guide.rst			\
	dnssec.inc.rst				\
	dns-security-overview.dia		\
	dns-security-overview.png		\
	dns-servers.dia				\
	dns-servers.png				\
	dns-tree.dia				\
	dns-tree.png				\
	dyndb.inc.rst				\
	general.rst				\
	history.rst				\
	index.rst				\
	intro-dns-bind.inc.rst			\
	introduction.inc.rst			\
	intro-security.inc.rst			\
	isc-logo.pdf				\
	logging-categories.inc.rst		\
	managed-keys.inc.rst			\
	manpages.rst				\
	name-resolution.dia			\
	name-resolution.png			\
	notes.rst				\
	pkcs11.inc.rst				\
	platforms.inc.rst			\
	plugins.inc.rst				\
	primary-secondary.dia			\
	primary-secondary.png			\
	recursive-query.dia			\
	recursive-query.png			\
	reference.rst				\
	requirements.inc.rst			\
	requirements.txt			\
	resolver-forward.dia			\
	resolver-forward.png			\
	rpz.inc.rst				\
	security.inc.rst			\
	sig0.inc.rst				\
	tkey.inc.rst				\
	troubleshooting.inc.rst			\
	tsig.inc.rst				\
	zones.inc.rst				\
	_ext/iscconf.py				\
	_ext/mergegrammar.py			\
	_ext/namedconf.py			\
	_ext/rndcconf.py			\
	_static/custom.css			\
	../dnssec-guide				\
	../misc/options				\
	../misc/rndc.grammar			\
	../misc/delegation-only.zoneopt		\
	../misc/forward.zoneopt			\
	../misc/hint.zoneopt			\
	../misc/in-view.zoneopt			\
	../misc/mirror.zoneopt			\
	../misc/primary.zoneopt			\
	../misc/redirect.zoneopt		\
	../misc/secondary.zoneopt		\
	../misc/static-stub.zoneopt		\
	../misc/stub.zoneopt			\
	../notes/*.rst

html-local:
	$(AM_V_SPHINX)$(SPHINX_BUILD) -b html -d $(SPHINXBUILDDIR)/.doctrees/html $(ALLSPHINXOPTS) $(SPHINXBUILDDIR)/html

install-html-local:
	$(INSTALL) -d $(DESTDIR)/$(docdir) $(DESTDIR)/$(docdir)/_static
	$(INSTALL) -D $(SPHINXBUILDDIR)/html/*.html $(DESTDIR)/$(docdir)/
	cp -R $(SPHINXBUILDDIR)/html/_static/ $(DESTDIR)/$(docdir)/_static/

singlehtml:
	$(AM_V_SPHINX)$(SPHINX_BUILD) -b singlehtml -d $(SPHINXBUILDDIR)/.doctrees/singlehtml $(ALLSPHINXOPTS) $(SPHINXBUILDDIR)/singlehtml

install-singlehtml: singlehtml
	$(INSTALL) -d $(DESTDIR)/$(docdir) $(DESTDIR)/$(docdir)/_static
	$(INSTALL_DATA) $(SPHINXBUILDDIR)/singlehtml/*.html $(DESTDIR)/$(docdir)/
	cp -R $(SPHINXBUILDDIR)/singlehtml/_static/* $(DESTDIR)/$(docdir)/_static/

epub:
	$(AM_V_SPHINX)$(SPHINX_BUILD) -b epub -d $(SPHINXBUILDDIR)/.doctrees/epub $(ALLSPHINXOPTS) $(SPHINXBUILDDIR)/epub

install-epub:
	$(INSTALL) -d $(DESTDIR)/$(docdir)
	$(INSTALL_DATA) $(SPHINXBUILDDIR)/epub/*.epub $(DESTDIR)/$(docdir)/

if HAVE_XELATEX
pdf-local:
	$(AM_V_SPHINX)$(SPHINX_BUILD) -b latex -d $(SPHINXBUILDDIR)/.doctrees/latex $(ALLSPHINXOPTS) $(SPHINXBUILDDIR)/latex
	$(MAKE) -C $(SPHINXBUILDDIR)/latex all-pdf

install-pdf-local:
	$(INSTALL) -d $(DESTDIR)/$(docdir)
	$(INSTALL_DATA) $(SPHINXBUILDDIR)/latex/*.pdf $(DESTDIR)/$(docdir)/
endif

clean-local:
	-rm -rf $(SPHINXBUILDDIR)

doc-local: html singlehtml pdf epub
