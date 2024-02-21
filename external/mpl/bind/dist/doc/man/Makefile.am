include $(top_srcdir)/Makefile.top
include $(top_srcdir)/Makefile.docs

MANPAGES_RST =				\
	arpaname.rst			\
	delv.rst			\
	dig.rst				\
	ddns-confgen.rst		\
	dnssec-cds.rst			\
	dnssec-dsfromkey.rst		\
	dnssec-importkey.rst		\
	dnssec-keyfromlabel.rst		\
	dnssec-keygen.rst		\
	dnssec-revoke.rst		\
	dnssec-settime.rst		\
	dnssec-signzone.rst		\
	dnssec-verify.rst		\
	dnstap-read.rst			\
	filter-aaaa.rst			\
	filter-a.rst			\
	host.rst			\
	index.rst			\
	mdig.rst			\
	named-checkconf.rst		\
	named-checkzone.rst		\
	named-compilezone.rst		\
	named-journalprint.rst		\
	named-nzd2nzf.rst		\
	named-rrchecker.rst		\
	named.conf.rst			\
	named.rst			\
	nsec3hash.rst			\
	nslookup.rst			\
	nsupdate.rst			\
	rndc-confgen.rst		\
	rndc.conf.rst			\
	rndc.rst			\
	tsig-keygen.rst			\
	../../bin/check/named-checkconf.rst \
	../../bin/check/named-checkzone.rst \
	../../bin/check/named-compilezone.rst \
	../../bin/confgen/ddns-confgen.rst \
	../../bin/confgen/rndc-confgen.rst \
	../../bin/confgen/tsig-keygen.rst \
	../../bin/delv/delv.rst \
	../../bin/dig/dig.rst \
	../../bin/dig/host.rst \
	../../bin/dig/nslookup.rst \
	../../bin/dnssec/dnssec-cds.rst \
	../../bin/dnssec/dnssec-dsfromkey.rst \
	../../bin/dnssec/dnssec-importkey.rst \
	../../bin/dnssec/dnssec-keyfromlabel.rst \
	../../bin/dnssec/dnssec-keygen.rst \
	../../bin/dnssec/dnssec-revoke.rst \
	../../bin/dnssec/dnssec-settime.rst \
	../../bin/dnssec/dnssec-signzone.rst \
	../../bin/dnssec/dnssec-verify.rst \
	../../bin/named/named.conf.rst \
	../../bin/named/named.rst \
	../../bin/nsupdate/nsupdate.rst \
	../../bin/plugins/filter-aaaa.rst \
	../../bin/plugins/filter-a.rst \
	../../bin/rndc/rndc.conf.rst \
	../../bin/rndc/rndc.rst \
	../../bin/tools/arpaname.rst \
	../../bin/tools/dnstap-read.rst \
	../../bin/tools/mdig.rst \
	../../bin/tools/named-journalprint.rst \
	../../bin/tools/named-nzd2nzf.rst \
	../../bin/tools/named-rrchecker.rst \
	../../bin/tools/nsec3hash.rst

man_MANS =				\
	arpaname.1			\
	ddns-confgen.8			\
	delv.1				\
	dig.1				\
	host.1				\
	mdig.1				\
	named-rrchecker.1		\
	nslookup.1			\
	nsupdate.1			\
	named.conf.5			\
	rndc.conf.5			\
	dnssec-cds.1			\
	dnssec-dsfromkey.1		\
	dnssec-importkey.1		\
	dnssec-keyfromlabel.1		\
	dnssec-keygen.1			\
	dnssec-revoke.1			\
	dnssec-settime.1		\
	dnssec-signzone.1		\
	dnssec-verify.1			\
	filter-aaaa.8			\
	filter-a.8			\
	named-checkconf.1		\
	named-checkzone.1		\
	named-compilezone.1		\
	named-journalprint.1		\
	named.8				\
	nsec3hash.1			\
	rndc-confgen.8			\
	rndc.8				\
	tsig-keygen.8

if HAVE_DNSTAP
man_MANS +=				\
	dnstap-read.1
endif HAVE_DNSTAP

if HAVE_LMDB
man_MANS +=				\
	named-nzd2nzf.1
endif HAVE_LMDB

MANPAGES_IN =				\
	$(man_MANS:=in)			\
	dnstap-read.1in			\
	named-nzd2nzf.1in

EXTRA_DIST =				\
	conf.py				\
	$(MANPAGES_RST)			\
	$(MANPAGES_IN)

CLEANFILES =				\
	$(man_MANS)

#
# Build rules for pre-generated manpages
#

man_SUBST =							\
	$(AM_V_SED)$(SED)					\
	  -e 's,[@]PACKAGE_VERSION@,$(PACKAGE_VERSION),'	\
	  -e 's,[@]RELEASE_DATE@,$(RELEASE_DATE),'		\
	  -e 's,[@]libdir[@],$(libdir),g'			\
	  -e 's,[@]runstatedir[@],$(runstatedir),g'		\
	  -e 's,[@]sysconfdir[@],$(sysconfdir),g'		\
	  $(srcdir)/$@in >$@

.1in.1:
	$(man_SUBST)

.5in.5:
	$(man_SUBST)

.8in.8:
	$(man_SUBST)

.NOTPARALLEL: man
man: Makefile $(man_MANS)

doc-local: man

clean-local::
	-rm -rf $(SPHINXBUILDDIR)


CLEANFILES +=				\
	manpages.stamp

if MAINTAINER_MODE

MAINTAINERCLEANFILES =			\
	$(MANPAGES_IN)

endif MAINTAINER_MODE

#
# Build rules for generating pre-generated manpages
#

if HAVE_SPHINX_BUILD
#
# See https://www.gnu.org/software/automake/manual/html_node/Multiple-Outputs.html
#
manpages.stamp: $(MANPAGES_RST)
	@rm -f manpages.tmp
	@touch manpages.tmp
	echo "${man_RST_EPILOG}"
	$(AM_V_SPHINX)$(SPHINX_BUILD) -b man -d $(SPHINXBUILDDIR)/.doctrees/man $(man_SPHINXOPTS) $(SPHINXBUILDDIR)/man
	for f in $(SPHINXBUILDDIR)/man/*; do \
	  cp -a "$$f" "$(srcdir)/$$(basename $$f)in"; \
	done
	@mv -f manpages.tmp $@

$(MANPAGES_IN): manpages.stamp
## Recover from the removal of $@
        @dry=; for f in x $$MAKEFLAGS; do \
          case $$f in \
            *=*|--*);; \
            *n*) dry=:;; \
          esac; \
        done; \
        if test -f $@; then :; else \
          $$dry trap 'rm -rf manpages.lock manpages.stamp' 1 2 13 15; \
          if $$dry mkdir manpages.lock 2>/dev/null; then \
## This code is being executed by the first process.
            $$dry rm -f manpages.stamp; \
            $(MAKE) $(AM_MAKEFLAGS) manpages.stamp; \
            $$dry rmdir manpages.lock; \
          else \
## This code is being executed by the follower processes.
## Wait until the first process is done.
            while test -d manpages.lock && test -z "$$dry"; do \
              sleep 1; \
            done; \
## Succeed if and only if the first process succeeded.
            $$dry test -f manpages.stamp; exit $$?; \
          fi; \
        fi

endif HAVE_SPHINX_BUILD
