#	$NetBSD: Makefile,v 1.7.4.2 2002/03/16 15:55:48 jdolecek Exp $

#
# Use this file to regenerate AMIGA and DRACO out of the GENERIC template.
#

all: AMIGA DRACO INSTALL WSCONS

AMIGA: GENERIC
	@echo Generating AMIGA...
	@echo -n '# '$$ > $@
	@echo NetBSD$$ >> $@
	@echo '#' >> $@
	@echo '# This file was automatically created. Changes will be' >> $@
	@echo '# lost when running make in this directory.' >> $@
	@echo '#' >> $@
	@echo -n '# Created from: ' >> $@
	@sed -e '1s/$$NetBSD/NetBSD/' GENERIC |\
		cpp -undef -P -DAMIGA_CONFIGURATION 2>/dev/null |\
		sed -e 's/\([ 	][ 	]*\)\#\([^ ]\)/\1# \2/' |\
		awk '{if (NF>1)count=0;else count++;if(count<=1)print}' >> $@

DRACO: GENERIC
	@echo Generating DRACO...
	@echo -n '# '$$ > $@
	@echo NetBSD$$ >> $@
	@echo '#' >> $@
	@echo '# This file was automatically created. Changes will be' >> $@
	@echo '# lost when running make in this directory.' >> $@
	@echo '#' >> $@
	@echo -n '# Created from: ' >> $@
	@sed -e '1s/$$NetBSD/NetBSD/' GENERIC |\
		cpp -undef -P -DDRACO_CONFIGURATION 2>/dev/null |\
		sed -e 's/\([ 	][ 	]*\)\#\([^ ]\)/\1# \2/' |\
		awk '{if (NF>1)count=0;else count++;if(count<=1)print}' >> $@

INSTALL: GENERIC
	@echo Generating INSTALL...
	@echo -n '# '$$ > $@
	@echo NetBSD$$ >> $@
	@echo '#' >> $@
	@echo '# This file was automatically created. Changes will be' >> $@
	@echo '# lost when running make in this directory.' >> $@
	@echo '#' >> $@
	@echo -n '# Created from: ' >> $@
	@sed -e '1s/$$NetBSD/NetBSD/' \
		-e 's/	pty	/	pty 2	/' GENERIC |\
		cpp -undef -P \
			-DAMIGA_CONFIGURATION -DDRACO_CONFIGURATION \
			-DINSTALL_CONFIGURATION 2>/dev/null |\
		sed -e 's/\([ 	][ 	]*\)\#\([^ ]\)/\1# \2/' |\
		awk '{if (NF>1)count=0;else count++;if(count<=1)print}' >> $@

#
# Generate WSCONS by uncommenting needed things and commenting
# out ite.
#
WSCONS: GENERIC
	@echo Generating $@ ...
	@echo -n '# '$$ > $@
	@echo NetBSD$$ >> $@
	@echo '#' >> $@
	@echo '# This file was automatically created. Changes will be' >> $@
	@echo '# lost when running make in this directory.' >> $@
	@echo '#' >> $@
	@echo -n '# Created from: ' >> $@
	@cat GENERIC | \
		sed -e 's/^ite/#ite/' | \
		sed -e 's/^grf/#grf/' | \
		sed -e 's/^#ws/ws/' | \
		sed -e 's/^#amidisplay/amidisplay/' | \
		sed -e 's/^#\(options.*[[:<:]]WS.*\)/\1/' | \
		sed -e 's/^#\(options.*[[:<:]]FONT_.*\)/\1/' | \
		cpp -undef -P 2>/dev/null \
			-DAMIGA_CONFIGURATION \
			-DDRACO_CONFIGURATION | \
		sed -e '1s/$$NetBSD/NetBSD/' | \
		awk '{if (NF>1)count=0;else count++;if(count<=1)print}' | \
		cat >>$@

AMIGA DRACO INSTALL WSCONS: GENERIC Makefile
