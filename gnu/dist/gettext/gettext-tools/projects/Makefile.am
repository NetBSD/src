## Makefile for the gettext-tools/projects subdirectory of GNU gettext
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

RM = rm -f

pkgdatadir = $(datadir)/gettext
projectsdir = $(pkgdatadir)/projects

PROJECTS = TP KDE GNOME
SCRIPTFILES = team-address \
	TP/trigger TP/team-address \
	KDE/trigger KDE/team-address \
	GNOME/trigger GNOME/team-address
DATAFILES = index \
	TP/teams.url TP/teams.html \
	KDE/teams.url KDE/teams.html \
	GNOME/teams.url GNOME/teams.html
EXTRA_DIST += $(SCRIPTFILES) $(DATAFILES)

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(projectsdir)
	@for p in $(PROJECTS); do \
	  echo "$(mkinstalldirs) $(DESTDIR)$(projectsdir)/$$p"; \
	  $(mkinstalldirs) $(DESTDIR)$(projectsdir)/$$p; \
	done
	@for f in $(SCRIPTFILES); do \
	  echo "$(INSTALL_SCRIPT) $(srcdir)/$$f $(DESTDIR)$(projectsdir)/$$f"; \
	  $(INSTALL_SCRIPT) $(srcdir)/$$f $(DESTDIR)$(projectsdir)/$$f; \
	done
	@for f in $(DATAFILES); do \
	  echo "$(INSTALL_DATA) $(srcdir)/$$f $(DESTDIR)$(projectsdir)/$$f"; \
	  $(INSTALL_DATA) $(srcdir)/$$f $(DESTDIR)$(projectsdir)/$$f; \
	done

installdirs-local:
	$(mkinstalldirs) $(DESTDIR)$(projectsdir)
	@for p in $(PROJECTS); do \
	  echo "$(mkinstalldirs) $(DESTDIR)$(projectsdir)/$$p"; \
	  $(mkinstalldirs) $(DESTDIR)$(projectsdir)/$$p; \
	done

uninstall-local:
	@for f in $(SCRIPTFILES); do \
	  echo "$(RM) $(DESTDIR)$(projectsdir)/$$f"; \
	  $(RM) $(DESTDIR)$(projectsdir)/$$f; \
	done
	@for f in $(DATAFILES); do \
	  echo "$(RM) $(DESTDIR)$(projectsdir)/$$f"; \
	  $(RM) $(DESTDIR)$(projectsdir)/$$f; \
	done


# VMS support.

EXTRA_DIST += Makefile.vms


# Woe32 support.

EXTRA_DIST += Makefile.msvc
