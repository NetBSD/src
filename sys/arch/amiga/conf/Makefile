#	$NetBSD: Makefile,v 1.2 1999/02/15 19:16:41 is Exp $

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
		cpp -P -DAMIGA_CONFIGURATION |\
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
		cpp -P -DDRACO_CONFIGURATION |\
		awk '{if (NF>1)count=0;else count++;if(count<=1)print}' >> $@

AMIGA DRACO: GENERIC Makefile
