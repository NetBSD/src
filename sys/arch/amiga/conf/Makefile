#	$NetBSD: Makefile,v 1.7.4.3 2002/09/06 08:31:44 jdolecek Exp $

#
# Use this file to regenerate AMIGA, DRACO, GENERIC, INSTALL and WSCONS
# out of the GENERIC.in template.
#

all: AMIGA DRACO GENERIC INSTALL WSCONS

AMIGA: __generate_config

DRACO: __generate_config

GENERIC: __generate_config

INSTALL: __generate_config

WSCONS: __generate_config

__generate_config: .USE GENERIC.in Makefile
	@echo Generating ${.TARGET}...
	@echo -n '# '$$ > ${.TARGET}
	@echo NetBSD$$ >> ${.TARGET}
	@echo '#' >> ${.TARGET}
	@echo '# This file was automatically created.' >> ${.TARGET}
	@echo '# Changes will be lost when make is run in this directory.' >> ${.TARGET}
	@echo '#' >> ${.TARGET}
	@echo -n '# Created from: ' >> ${.TARGET}
	sed -e '1s/$$NetBSD/NetBSD/' GENERIC.in |\
		m4 -P -DM4_Target=${.TARGET} >> ${.TARGET}
