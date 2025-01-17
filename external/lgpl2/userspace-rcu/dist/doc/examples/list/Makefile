# SPDX-FileCopyrightText: 2013 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
#
# SPDX-License-Identifier: MIT

# This makefile is purposefully kept simple to support GNU and BSD make.

all:
	$(AM_V_at)$(MAKE) -f Makefile.cds_list_add_rcu
	$(AM_V_at)$(MAKE) -f Makefile.cds_list_add_tail_rcu
	$(AM_V_at)$(MAKE) -f Makefile.cds_list_del_rcu
	$(AM_V_at)$(MAKE) -f Makefile.cds_list_for_each_rcu
	$(AM_V_at)$(MAKE) -f Makefile.cds_list_for_each_entry_rcu
	$(AM_V_at)$(MAKE) -f Makefile.cds_list_replace_rcu

.PHONY: clean
clean:
	$(AM_V_at)$(MAKE) -f Makefile.cds_list_add_rcu clean
	$(AM_V_at)$(MAKE) -f Makefile.cds_list_add_tail_rcu clean
	$(AM_V_at)$(MAKE) -f Makefile.cds_list_del_rcu clean
	$(AM_V_at)$(MAKE) -f Makefile.cds_list_for_each_rcu clean
	$(AM_V_at)$(MAKE) -f Makefile.cds_list_for_each_entry_rcu clean
	$(AM_V_at)$(MAKE) -f Makefile.cds_list_replace_rcu clean
