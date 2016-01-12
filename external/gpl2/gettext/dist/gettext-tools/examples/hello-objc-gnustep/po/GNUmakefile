# Example for use of GNU gettext.
# Copyright (C) 2003 Free Software Foundation, Inc.
# This file is in the public domain.
#
# Makefile - processed by GNU make.

# NLS is enabled by default.
USE_NLS = yes

# List of files which contain translatable strings.
POTFILES = \
  Hello.m

# Usually the message domain is the same as the package name in lower case.
DOMAIN = hello-objc-gnustep

# These options get passed to xgettext.
XGETTEXT_OPTIONS =

# This is the copyright holder that gets inserted into the header of the
# $(DOMAIN).pot file.  Set this to the copyright holder of the surrounding
# package.  (Note that the msgstr strings, extracted from the package's
# sources, belong to the copyright holder of the package.)  Translators are
# expected to transfer the copyright for their translations to this person
# or entity, or to disclaim their copyright.  The empty string stands for
# the public domain; in this case the translators are expected to disclaim
# their copyright.
COPYRIGHT_HOLDER = Yoyodyne, Inc.

# This is the email address or URL to which the translators shall report
# bugs in the untranslated strings:
# - Strings which are not entire sentences, see the maintainer guidelines
#   in the GNU gettext documentation, section 'Preparing Strings'.
# - Strings which use unclear terms or require additional context to be
#   understood.
# - Strings which make invalid assumptions about notation of date, time or
#   money.
# - Pluralisation problems.
# - Incorrect English spelling.
# - Incorrect formatting.
# It can be your email address, or a mailing list address where translators
# can write to without being subscribed, or the URL of a web page through
# which the translators can contact you.
MSGID_BUGS_ADDRESS = bug-gnu-gettext@gnu.org

-include $(GNUSTEP_MAKEFILES)/rules.make

XGETTEXT = xgettext
MSGMERGE = msgmerge
MSGMERGE_UPDATE = msgmerge --update
MSGINIT = msginit
MSGCONV = msgconv
MSGFILTER = msgfilter

# Set of available languages.
LINGUAS := $(shell sed -e "/^\#/d" LINGUAS)
# Set of languages which use msgen.
ENLINGUAS := $(filter-out $(LINGUAS), en)

top_srcdir = ..

include LocaleAliases

POTFILES_DEPS = $(foreach file, $(POTFILES), $(top_srcdir)/$(file))
POFILES = $(foreach lang, $(LINGUAS), $(lang).po)
UPDATEPOFILES = $(foreach lang, $(LINGUAS), $(lang).po-update)
DUMMYPOFILES = $(foreach lang, $(LINGUAS), $(lang).nop)
STRINGSFILES = $(foreach lang, $(LINGUAS), $(top_srcdir)/$(NSLOCALE_$(lang)).lproj/Localizable.strings)
ENSTRINGSFILES = $(foreach lang, $(ENLINGUAS), $(top_srcdir)/$(NSLOCALE_$(lang)).lproj/Localizable.strings)
CATALOGS = $(STRINGSFILES) $(ENSTRINGSFILES)

.SUFFIXES: .po .mo .sed .sin .nop .po-update

.sin.sed:
	sed -e '/^#/d' $< > t-$@
	mv t-$@ $@


internal-all:: all-local-$(USE_NLS)

all-local-yes: $(CATALOGS)
all-local-no:

# Note: Target 'all' must not depend on target '$(DOMAIN).pot-update',
# otherwise packages like GCC can not be built if only parts of the source
# have been downloaded.

# This target rebuilds $(DOMAIN).pot; it is an expensive operation.
# Note that $(DOMAIN).pot is not touched if it doesn't need to be changed.
$(DOMAIN).pot-update: $(POTFILES_DEPS) remove-potcdate.sed
	$(XGETTEXT) --default-domain=$(DOMAIN) --directory=$(top_srcdir) \
	  --add-comments=TRANSLATORS: $(XGETTEXT_OPTIONS) \
	  --copyright-holder='$(COPYRIGHT_HOLDER)' \
	  --msgid-bugs-address='$(MSGID_BUGS_ADDRESS)' \
	  $(POTFILES)
	test ! -f $(DOMAIN).po || { \
	  if test -f $(DOMAIN).pot; then \
	    sed -f remove-potcdate.sed < $(DOMAIN).pot > $(DOMAIN).1po && \
	    sed -f remove-potcdate.sed < $(DOMAIN).po > $(DOMAIN).2po && \
	    if cmp $(DOMAIN).1po $(DOMAIN).2po >/dev/null 2>&1; then \
	      rm -f $(DOMAIN).1po $(DOMAIN).2po $(DOMAIN).po; \
	    else \
	      rm -f $(DOMAIN).1po $(DOMAIN).2po $(DOMAIN).pot && \
	      mv $(DOMAIN).po $(DOMAIN).pot; \
	    fi; \
	  else \
	    mv $(DOMAIN).po $(DOMAIN).pot; \
	  fi; \
	}

# This rule has no dependencies: we don't need to update $(DOMAIN).pot at
# every "make" invocation, only create it when it is missing.
# Only "make $(DOMAIN).pot-update" or "make dist" will force an update.
$(DOMAIN).pot:
	$(MAKE) $(DOMAIN).pot-update

# This target rebuilds a PO file if $(DOMAIN).pot has changed.
# Note that a PO file is not touched if it doesn't need to be changed.
$(POFILES): $(DOMAIN).pot
	@lang=`echo $@ | sed -e 's,.*/,,' -e 's/\.po$$//'`; \
	echo "$(MSGMERGE_UPDATE) $${lang}.po $(DOMAIN).pot"; \
	$(MSGMERGE_UPDATE) $${lang}.po $(DOMAIN).pot

update-po:
	$(MAKE) $(DOMAIN).pot-update
	test -z "$(UPDATEPOFILES)" || $(MAKE) $(UPDATEPOFILES)
	$(MAKE) update-strings

# General rule for updating PO files.

.nop.po-update:
	@lang=`echo $@ | sed -e 's/\.po-update$$//'`; \
	tmpdir=`pwd`; \
	echo "$$lang:"; \
	echo "$(MSGMERGE) $$lang.po $(DOMAIN).pot -o $$lang.new.po"; \
	if $(MSGMERGE) $$lang.po $(DOMAIN).pot -o $$tmpdir/$$lang.new.po; then \
	  if cmp $$lang.po $$tmpdir/$$lang.new.po >/dev/null 2>&1; then \
	    rm -f $$tmpdir/$$lang.new.po; \
	  else \
	    if mv -f $$tmpdir/$$lang.new.po $$lang.po; then \
	      :; \
	    else \
	      echo "msgmerge for $$lang.po failed: cannot move $$tmpdir/$$lang.new.po to $$lang.po" 1>&2; \
	      exit 1; \
	    fi; \
	  fi; \
	else \
	  echo "msgmerge for $$lang.po failed!" 1>&2; \
	  rm -f $$tmpdir/$$lang.new.po; \
	fi

$(DUMMYPOFILES):


update-strings: $(CATALOGS)
	@:

# I don't know how to make a dependency from a particular .strings file to its
# corresponding .po file; therefore I put here a dependency towards all .po
# files. It's safer than no dependency at all.
$(STRINGSFILES): $(POFILES)
	$(MAKE) $(UNIXLOCALE_$(patsubst %.lproj,%, $(notdir $(patsubst %/,%, $(dir $@))))).po
	mkdir -p $(patsubst %/,%, $(dir $@))
	msgcat --stringtable-output -o $@.tmp $(UNIXLOCALE_$(patsubst %.lproj,%, $(notdir $(patsubst %/,%, $(dir $@))))).po
	mv $@.tmp $@

$(ENSTRINGSFILES): $(DOMAIN).pot
	mkdir -p $(patsubst %/,%, $(dir $@))
	$(MSGINIT) -i $(DOMAIN).pot --no-translator -l $(UNIXLOCALE_$(patsubst %.lproj,%, $(notdir $(patsubst %/,%, $(dir $@))))) --stringtable-output -o $@.tmp
	mv $@.tmp $@

echo-languages:
	@echo $(foreach lang, $(LINGUAS), $(NSLOCALE_$(lang)))

