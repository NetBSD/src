#       Id: Makefile,v 8.5 1999/09/23 22:36:36 ca Exp
#       $NetBSD: Makefile,v 1.1.1.3 2003/06/01 14:01:17 atatat Exp $

SHELL= /bin/sh
BUILD=   ./Build
OPTIONS= $(CONFIG) $(FLAGS)

all: FRC
	$(SHELL) $(BUILD) $(OPTIONS) $@
clean: FRC
	$(SHELL) $(BUILD) $(OPTIONS) $@
install: FRC
	$(SHELL) $(BUILD) $(OPTIONS) $@

fresh: FRC
	$(SHELL) $(BUILD) $(OPTIONS) -c

FRC:
