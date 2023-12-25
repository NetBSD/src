# Usage: 
#	make makefiles [name=value]...
#
# See makedefs for a description of available options.
# Examples:
#
#	make makefiles
#	make makefiles CC="purify cc"
#	make makefiles CC=cc OPT=
#
SHELL	= /bin/sh

default: update

update depend printfck clean tidy depend_update: Makefiles
	$(MAKE) MAKELEVEL= $@

install upgrade:
	@echo Please review the INSTALL instructions first.

makefiles Makefiles:
	$(MAKE) -f Makefile.in MAKELEVEL= Makefiles
