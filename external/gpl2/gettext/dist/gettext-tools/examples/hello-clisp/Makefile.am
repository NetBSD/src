# Example for use of GNU gettext.
# Copyright (C) 2003 Free Software Foundation, Inc.
# This file is in the public domain.
#
# Makefile configuration - processed by automake.

# General automake options.
AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS = -I m4

# The list of subdirectories containing Makefiles.
SUBDIRS = m4 po

# The list of programs that are built.
bin_SCRIPTS = hello

# Compilation of Common Lisp programs.
SUFFIXES = .lisp .fas
.lisp.fas:
	clisp -norc -q -c $< -o $@
CLEANFILES = *.fas *.lib

# Making a Common Lisp program executable.
hello: hello.fas
	(echo '#!@CLISP@'; cat $<) > $@
	chmod a+x $@
CLEANFILES += hello

# Additional files to be distributed.
EXTRA_DIST = autogen.sh autoclean.sh
