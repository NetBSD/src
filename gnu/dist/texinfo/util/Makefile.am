# Id: Makefile.am,v 1.6 2003/01/31 16:26:23 karl Exp
# Makefile.am for texinfo/util.
# Run automake in .. to produce Makefile.in from this.
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

bin_PROGRAMS = ginstall-info texindex
bin_SCRIPTS = texi2dvi

# Use `ginstall-info' in the definition of PROGRAMS and in dependencies
# because automake generates an install-info target in every Makefile (a
# bug, IMHO, but this is easier than patching Automake.)
ginstall_info_SOURCES = install-info.c 

# Transform `ginstall-info' to `install-info' before applying any
# user-specified name transformations.
transform = s/ginstall-info/install-info/; @program_transform_name@

localedir = $(datadir)/locale
INCLUDES = -I$(top_srcdir)/lib -I../intl -DLOCALEDIR=\"$(localedir)\"
LDADD = ../lib/libtxi.a @LIBINTL@

# Most of these are for fun.  The only one that's really supported is texi2dvi.
# 
# automake.diff is necessary to apply if you change configure.ac, are
# building from cvs, or otherwise need to run automake.  Otherwise
# install-info will be built on every make, and make distcleancheck will
# fail.  (These bugs have been reported [several times] to bug-automake.)
# 
EXTRA_DIST = README deref.c dir-example fix-info-dir fixfonts \
  fixref.gawk gen-dir-node infosrch install-info-html outline.gawk \
  prepinfo.awk tex3patch texi-docstring-magic.el texi2dvi txitextest
