#	$NetBSD: Makefile,v 1.1 1999/02/04 19:01:37 is Exp $

#
# Use this file to regenerate AMIGA and DRACO out of the GENERIC template.
#

all: AMIGA DRACO

AMIGA: 
	@echo '# This file was automatically created. Changes will be' > $@
	@echo '# lost when running make in this directory.' >> $@
	@echo '#' >> $@
	@echo '# Created from: ' >> $@
	@sed -e '1s/NetBSD/ NetBSD/' GENERIC |\
		cpp -P -DAMIGA_CONFIGURATION |\
		awk '{if (NF>1)count=0;else count++;if(count<=1)print}' >> $@

DRACO:
	@echo '# This file was automatically created. Changes will be' > $@
	@echo '# lost when running make in this directory.' >> $@
	@echo '#' >> $@
	@echo '# Created from: ' >> $@
	@sed -e '1s/NetBSD/ NetBSD/' GENERIC |\
		cpp -P -DDRACO_CONFIGURATION |\
		awk '{if (NF>1)count=0;else count++;if(count<=1)print}' >> $@

AMIGA DRACO: GENERIC Makefile
