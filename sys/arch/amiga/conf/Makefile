#	$NetBSD: Makefile,v 1.13.16.1 2009/01/17 13:27:50 mjf Exp $

#
# Use this file to regenerate DRACO, GENERIC, INSTALL
# out of the GENERIC.in template.
#

all: DRACO GENERIC INSTALL

DRACO: __generate_config

GENERIC: __generate_config

INSTALL: __generate_config

__generate_config: .USE GENERIC.in Makefile
	@echo Generating ${.TARGET}...
	@echo -n '# '$$ > ${.TARGET}
	@echo NetBSD$$ >> ${.TARGET}
	@echo '#' >> ${.TARGET}
	@echo '# This file was automatically created.' >> ${.TARGET}
	@echo '# Changes will be lost when make is run in this directory.' >> ${.TARGET}
	@echo '#' >> ${.TARGET}
	@echo -n '# Created from: ' >> ${.TARGET}
	${TOOL_SED} -e '1s/$$NetBSD/NetBSD/' GENERIC.in |\
		m4 -P -DM4_Target=${.TARGET} >> ${.TARGET}
