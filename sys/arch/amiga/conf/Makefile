#	$NetBSD: Makefile,v 1.3 1999/03/09 15:14:32 is Exp $

#
# Use this file to regenerate AMIGA and DRACO out of the GENERIC template.
#

all: AMIGA DRACO

AMIGA: 
	@echo -n '# '$$ > $@
	@echo NetBSD$$ >> $@
	@echo '#' >> $@
	@echo '# This file was automatically created. Changes will be' >> $@
	@echo '# lost when running make in this directory.' >> $@
	@echo '#' >> $@
	@echo '# Created from: ' >> $@
	@sed -e '1s/NetBSD/ NetBSD/' GENERIC |\
		cpp -undef -P -DAMIGA_CONFIGURATION |\
		awk '{if (NF>1)count=0;else count++;if(count<=1)print}' >> $@

DRACO:
	@echo -n '# '$$ > $@
	@echo NetBSD$$ >> $@
	@echo '#' >> $@
	@echo '# This file was automatically created. Changes will be' >> $@
	@echo '# lost when running make in this directory.' >> $@
	@echo '#' >> $@
	@echo '# Created from: ' >> $@
	@sed -e '1s/NetBSD/ NetBSD/' GENERIC |\
		cpp -undef -P -DDRACO_CONFIGURATION |\
		awk '{if (NF>1)count=0;else count++;if(count<=1)print}' >> $@

AMIGA DRACO: GENERIC Makefile
