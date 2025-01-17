# SPDX-FileCopyrightText: 2013 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
#
# SPDX-License-Identifier: MIT

# This makefile is purposefully kept simple to support GNU and BSD make.

all:
	$(AM_V_at)$(MAKE) -f Makefile.cds_lfht_add
	$(AM_V_at)$(MAKE) -f Makefile.cds_lfht_add_unique
	$(AM_V_at)$(MAKE) -f Makefile.cds_lfht_add_replace
	$(AM_V_at)$(MAKE) -f Makefile.cds_lfht_del
	$(AM_V_at)$(MAKE) -f Makefile.cds_lfht_destroy
	$(AM_V_at)$(MAKE) -f Makefile.cds_lfht_lookup
	$(AM_V_at)$(MAKE) -f Makefile.cds_lfht_for_each_entry_duplicate

.PHONY: clean
clean:
	$(AM_V_at)$(MAKE) -f Makefile.cds_lfht_add clean
	$(AM_V_at)$(MAKE) -f Makefile.cds_lfht_add_unique clean
	$(AM_V_at)$(MAKE) -f Makefile.cds_lfht_add_replace clean
	$(AM_V_at)$(MAKE) -f Makefile.cds_lfht_del clean
	$(AM_V_at)$(MAKE) -f Makefile.cds_lfht_destroy clean
	$(AM_V_at)$(MAKE) -f Makefile.cds_lfht_lookup clean
	$(AM_V_at)$(MAKE) -f Makefile.cds_lfht_for_each_entry_duplicate clean
