#  Copyright (C) 2022 Yubico AB - See COPYING

dist_man1_MANS = pamu2fcfg.1
dist_man8_MANS = pam_u2f.8
MAINTAINERCLEANFILES = $(MANS)
EXTRA_DIST = $(MANS:=.txt)

SUFFIXES = .1.txt .1 .8.txt .8

.1.txt.1 .8.txt.8:
	$(AM_V_GEN)$(A2X) --format=manpage -L -a revdate="Version $(VERSION)" $<
