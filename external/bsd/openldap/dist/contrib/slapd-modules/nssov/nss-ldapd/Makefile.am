# Makefile.am - use automake to generate Makefile.in
#
# Copyright (C) 2006 Luke Howard
# Copyright (C) 2006 West Consulting
# Copyright (C) 2006, 2007, 2008 Arthur de Jong
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301 USA

SUBDIRS = compat common nss nslcd man tests

DEBIAN_FILES = debian/changelog debian/compat debian/control \
               debian/copyright debian/rules \
               debian/libnss-ldapd.docs \
               debian/libnss-ldapd.examples \
               debian/libnss-ldapd.lintian-overrides \
               debian/libnss-ldapd.nslcd.init \
               debian/libnss-ldapd.config \
               debian/libnss-ldapd.templates \
               debian/libnss-ldapd.postinst \
               debian/libnss-ldapd.postrm \
               $(wildcard debian/po/*.po) debian/po/templates.pot \
               debian/po/POTFILES.in

EXTRA_DIST = nss-ldapd.conf nslcd.h nslcd-common.h \
             $(wildcard m4/*.m4) HACKING $(DEBIAN_FILES)

DISTCHECK_CONFIGURE_FLAGS='--enable-warnings'

ACLOCAL_AMFLAGS = -I m4

NSS_LDAP_PATH_CONF = @NSS_LDAP_PATH_CONF@

install-data-local: install-ldap_conf
uninstall-local: uninstall-ldap_conf

# install a default configuration file if it is not already there
install-ldap_conf:
	@if [ -f $(DESTDIR)$(NSS_LDAP_PATH_CONF) ]; then \
	  echo "$(DESTDIR)$(NSS_LDAP_PATH_CONF) already exists, install will not overwrite"; \
	else \
	  $(INSTALL_DATA) -D $(srcdir)/nss-ldapd.conf $(DESTDIR)$(NSS_LDAP_PATH_CONF); \
	fi
uninstall-ldap_conf:
	-rm -f $(DESTDIR)$(NSS_LDAP_PATH_CONF)

# target for easily creating a Debian package
# the find is an ugly hack to fix a bug if being built on an nfs filesystem
deb: distdir
	find $(distdir) -type d | xargs touch
	cd $(distdir) && \
	  debuild
	rm -rf $(distdir)

# target for generating the ChangeLog file
changelog:
	( svn2cl -i --stdout -r HEAD:195 ; \
	  svn2cl -i --stdout -r 194:3 --strip-prefix='libnss_ldapd' ; \
	  svn2cl -i --stdout -r 2 --strip-prefix='libnss_ldap_251-5.2' ; \
	  svn2cl -i --stdout -r 1 --strip-prefix='nss_ldap-251' ; \
	) > ChangeLog

flawfinder.html:
	flawfinder --quiet --html --context --followdotdir . > $@

rats.html:
	rats --quiet --html --context . > $@

splint.txt:
	-env LARCH_PATH=/usr/share/splint/lib/ \
	    LCLIMPORTDIR=/usr/share/splint/imports/ \
	    splint -checks \
	           -warnposix +showsummary +showalluses +hints -namechecks \
	           -globstate -predboolint -mustfreeonly -temptrans -kepttrans \
	           -I. -I$(srcdir) -I$(top_builddir) $(DEFS) -D_REENTRANT -DDEBUG \
	           -D__signed__=signed -D__thread= -D__gnuc_va_list=__ptr_t \
	           -Dkrb5_int32=int32_t -Dkrb5_ui_4=uint32_t \
	           -D__u16=uint16_t -D__u32=uint32_t \
	           *.[ch] nss/*.[ch] nslcd/*.[ch] common/*.[ch] compat/*.[ch] > $@ 2>&1

.PHONY: flawfinder.html rats.html splint.txt
