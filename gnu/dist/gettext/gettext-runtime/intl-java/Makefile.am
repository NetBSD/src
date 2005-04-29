## Makefile for the gettext-runtime/intl-java subdirectory of GNU gettext
## Copyright (C) 2001-2003 Free Software Foundation, Inc.
##
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2, or (at your option)
## any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

## Process this file with automake to produce Makefile.in.

AUTOMAKE_OPTIONS = 1.2 gnits
EXTRA_DIST =
CLEANFILES =

RM = rm -f

jardir = $(datadir)/gettext
docdir = @docdir@
htmldir = $(docdir)

JAR = @JAR@
JAVACOMP = $(SHELL) ../lib/javacomp.sh
JAVADOC1 = test ! -d /usr/lib/jdk1.1.8 || env PATH=/usr/lib/jdk1.1.8/bin:$$PATH javadoc
JAVADOC2 = test ! -d /usr/lib/jdk1.3.1 || env PATH=/usr/lib/jdk1.3.1/bin:$$PATH javadoc


all-local: all-classes all-javadoc1 all-javadoc2
install-data-local: install-classes install-javadoc1 install-javadoc2
installdirs-local: installdirs-classes installdirs-javadoc1 installdirs-javadoc2
uninstall-local: uninstall-classes uninstall-javadoc1 uninstall-javadoc2


# Special rules for Java compilation.

all-classes: all-classes-@BUILDJAVA@
all-classes-no:
all-classes-yes: libintl.jar

gnu/gettext/GettextResource.class: $(srcdir)/gnu/gettext/GettextResource.java
	$(JAVACOMP) -d . $(srcdir)/gnu/gettext/GettextResource.java

libintl.jar: gnu/gettext/GettextResource.class
	$(JAR) cf $@ gnu/gettext/GettextResource*.class

EXTRA_DIST += gnu/gettext/GettextResource.java

CLEANFILES += libintl.jar gnu/gettext/*.class

install-classes: install-classes-@BUILDJAVA@
install-classes-no:
	$(mkinstalldirs) $(DESTDIR)$(jardir)
install-classes-yes: all-classes-yes
	$(mkinstalldirs) $(DESTDIR)$(jardir)
	$(INSTALL_DATA) libintl.jar $(DESTDIR)$(jardir)/libintl.jar

installdirs-classes:
	$(mkinstalldirs) $(DESTDIR)$(jardir)

uninstall-classes:
	$(RM) $(DESTDIR)$(jardir)/libintl.jar


# Special rules for Javadoc (JDK 1.1) documentation.

all-javadoc1: $(srcdir)/javadoc1/tree.html

$(srcdir)/javadoc1/tree.html: $(srcdir)/gnu/gettext/GettextResource.java
	cd $(srcdir) && $(JAVADOC1) -d javadoc1 gnu/gettext/*.java

JAVADOC1_FILES = \
  javadoc1/packages.html javadoc1/tree.html javadoc1/AllNames.html \
  javadoc1/gnu.gettext.GettextResource.html \
  javadoc1/images/blue-ball-small.gif javadoc1/images/blue-ball.gif \
  javadoc1/images/class-index.gif \
  javadoc1/images/constructor-index.gif \
  javadoc1/images/constructors.gif \
  javadoc1/images/cyan-ball-small.gif javadoc1/images/cyan-ball.gif \
  javadoc1/images/error-index.gif \
  javadoc1/images/exception-index.gif \
  javadoc1/images/green-ball-small.gif javadoc1/images/green-ball.gif \
  javadoc1/images/interface-index.gif \
  javadoc1/images/magenta-ball-small.gif javadoc1/images/magenta-ball.gif \
  javadoc1/images/method-index.gif \
  javadoc1/images/methods.gif \
  javadoc1/images/package-index.gif \
  javadoc1/images/red-ball-small.gif javadoc1/images/red-ball.gif \
  javadoc1/images/variable-index.gif \
  javadoc1/images/variables.gif \
  javadoc1/images/yellow-ball-small.gif javadoc1/images/yellow-ball.gif

EXTRA_DIST += $(JAVADOC1_FILES)

install-javadoc1: all-javadoc1
	$(mkinstalldirs) $(DESTDIR)$(htmldir)/javadoc1
	$(mkinstalldirs) $(DESTDIR)$(htmldir)/javadoc1/images
	@for f in $(JAVADOC1_FILES); do \
	  echo "$(INSTALL_DATA) $(srcdir)/$$f $(DESTDIR)$(htmldir)/$$f"; \
	  $(INSTALL_DATA) $(srcdir)/$$f $(DESTDIR)$(htmldir)/$$f; \
	done

installdirs-javadoc1:
	$(mkinstalldirs) $(DESTDIR)$(htmldir)/javadoc1
	$(mkinstalldirs) $(DESTDIR)$(htmldir)/javadoc1/images

uninstall-javadoc1:
	@for f in $(JAVADOC1_FILES); do \
	  echo "$(RM) $(DESTDIR)$(htmldir)/$$f"; \
	  $(RM) $(DESTDIR)$(htmldir)/$$f; \
	done


# Special rules for Javadoc (JDK 1.2) documentation.

all-javadoc2: $(srcdir)/javadoc2/index.html

$(srcdir)/javadoc2/index.html: $(srcdir)/gnu/gettext/GettextResource.java
	cd $(srcdir) && $(JAVADOC2) -d javadoc2 gnu/gettext/*.java

JAVADOC2_FILES = \
  javadoc2/index.html \
  javadoc2/allclasses-frame.html \
  javadoc2/overview-tree.html \
  javadoc2/deprecated-list.html \
  javadoc2/serialized-form.html \
  javadoc2/index-all.html \
  javadoc2/help-doc.html \
  javadoc2/packages.html \
  javadoc2/package-list \
  javadoc2/stylesheet.css \
  javadoc2/gnu/gettext/package-summary.html \
  javadoc2/gnu/gettext/package-tree.html \
  javadoc2/gnu/gettext/package-frame.html \
  javadoc2/gnu/gettext/GettextResource.html

EXTRA_DIST += $(JAVADOC2_FILES)

install-javadoc2: all-javadoc2
	$(mkinstalldirs) $(DESTDIR)$(htmldir)/javadoc2
	$(mkinstalldirs) $(DESTDIR)$(htmldir)/javadoc2/gnu/gettext
	@for f in $(JAVADOC2_FILES); do \
	  echo "$(INSTALL_DATA) $(srcdir)/$$f $(DESTDIR)$(htmldir)/$$f"; \
	  $(INSTALL_DATA) $(srcdir)/$$f $(DESTDIR)$(htmldir)/$$f; \
	done

installdirs-javadoc2:
	$(mkinstalldirs) $(DESTDIR)$(htmldir)/javadoc2
	$(mkinstalldirs) $(DESTDIR)$(htmldir)/javadoc2/gnu/gettext

uninstall-javadoc2:
	@for f in $(JAVADOC2_FILES); do \
	  echo "$(RM) $(DESTDIR)$(htmldir)/$$f"; \
	  $(RM) $(DESTDIR)$(htmldir)/$$f; \
	done
