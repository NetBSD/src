# $OpenLDAP: pkg/ldap/build/srv.mk,v 1.18.2.3 2008/02/11 23:26:38 kurt Exp $
## This work is part of OpenLDAP Software <http://www.openldap.org/>.
##
## Copyright 1998-2008 The OpenLDAP Foundation.
## All rights reserved.
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted only as authorized by the OpenLDAP
## Public License.
##
## A copy of this license is available in the file LICENSE in the
## top-level directory of the distribution or, alternatively, at
## <http://www.OpenLDAP.org/license.html>.
##---------------------------------------------------------------------------
#
# Makefile Template for Servers
#

all-common: all-$(BUILD_SRV)
all-no lint-no 5lint-no depend-no install-no:
	@echo "run configure with $(BUILD_OPT) to make $(PROGRAMS)"

clean-common: clean-srv FORCE
veryclean-common: veryclean-srv FORCE

lint-common: lint-$(BUILD_SRV)

5lint-common: 5lint-$(BUILD_SRV)

depend-common: depend-$(BUILD_SRV)

install-common: install-$(BUILD_SRV)

all-local-srv:
all-yes: all-local-srv FORCE

install-local-srv:
install-yes: install-local-srv FORCE

lint-local-srv:
lint-yes: lint-local-srv FORCE
	$(LINT) $(DEFS) $(DEFINES) $(SRCS)

5lint-local-srv:
5lint-yes: 5lint-local-srv FORCE
	$(5LINT) $(DEFS) $(DEFINES) $(SRCS)

clean-local-srv:
clean-srv: 	clean-local-srv FORCE
	$(RM) $(PROGRAMS) $(XPROGRAMS) $(XSRCS) *.o a.out core .libs/* *.exe

depend-local-srv:
depend-yes: depend-local-srv FORCE
	$(MKDEP) $(DEFS) $(DEFINES) $(SRCS)

veryclean-local-srv:
veryclean-srv: 	clean-srv veryclean-local-srv

Makefile: $(top_srcdir)/build/srv.mk
