#	$NetBSD: Makefile,v 1.7.4.1 2002/02/11 20:06:49 jdolecek Exp $

#
# Use this file to regenerate AMIGA and DRACO out of the GENERIC template.
#

all: AMIGA DRACO INSTALL

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

AMIGA DRACO INSTALL: GENERIC Makefile
