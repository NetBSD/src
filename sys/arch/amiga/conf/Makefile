#	$NetBSD: Makefile,v 1.5 2000/04/01 20:21:27 is Exp $

#
# Use this file to regenerate AMIGA and DRACO out of the GENERIC template.
#

all: AMIGA DRACO INSTALL

AMIGA: GENERIC
	@echo -n '# '$$ > $@
	@echo NetBSD$$ >> $@
	@echo '#' >> $@
	@echo '# This file was automatically created. Changes will be' >> $@
	@echo '# lost when running make in this directory.' >> $@
	@echo '#' >> $@
	@echo '# Created from: ' >> $@
	@sed -e '1s/NetBSD/ NetBSD/' GENERIC |\
		cpp -undef -P -DAMIGA_CONFIGURATION 2>/dev/null |\
		awk '{if (NF>1)count=0;else count++;if(count<=1)print}' >> $@

DRACO: GENERIC
	@echo -n '# '$$ > $@
	@echo NetBSD$$ >> $@
	@echo '#' >> $@
	@echo '# This file was automatically created. Changes will be' >> $@
	@echo '# lost when running make in this directory.' >> $@
	@echo '#' >> $@
	@echo '# Created from: ' >> $@
	@sed -e '1s/NetBSD/ NetBSD/' GENERIC |\
		cpp -undef -P -DDRACO_CONFIGURATION 2>/dev/null |\
		awk '{if (NF>1)count=0;else count++;if(count<=1)print}' >> $@

INSTALL: GENERIC
	@echo -n '# '$$ > $@
	@echo NetBSD$$ >> $@
	@echo '#' >> $@
	@echo '# This file was automatically created. Changes will be' >> $@
	@echo '# lost when running make in this directory.' >> $@
	@echo '#' >> $@
	@echo '# Created from: ' >> $@
	@sed -e '1s/NetBSD/ NetBSD/' GENERIC |\
		cpp -undef -P \
			-DAMIGA_CONFIGURATION -DDRACO_CONFIGURATION \
			-DINSTALL_CONFIGURATION 2>/dev/null |\
		awk '{if (NF>1)count=0;else count++;if(count<=1)print}' >> $@

AMIGA DRACO INSTALL: GENERIC Makefile
