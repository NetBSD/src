#	$NetBSD: Makefile,v 1.4 1999/04/10 21:36:26 tron Exp $

#
# Use this file to regenerate AMIGA and DRACO out of the GENERIC template.
#

all: AMIGA DRACO

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

AMIGA DRACO: GENERIC Makefile
