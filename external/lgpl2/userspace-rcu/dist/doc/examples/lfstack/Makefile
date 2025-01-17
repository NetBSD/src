# SPDX-FileCopyrightText: 2013 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
#
# SPDX-License-Identifier: MIT

# This makefile is purposefully kept simple to support GNU and BSD make.

all:
	$(AM_V_at)$(MAKE) -f Makefile.cds_lfs_push
	$(AM_V_at)$(MAKE) -f Makefile.cds_lfs_pop_blocking
	$(AM_V_at)$(MAKE) -f Makefile.cds_lfs_pop_all_blocking

.PHONY: clean
clean:
	$(AM_V_at)$(MAKE) -f Makefile.cds_lfs_push clean
	$(AM_V_at)$(MAKE) -f Makefile.cds_lfs_pop_blocking clean
	$(AM_V_at)$(MAKE) -f Makefile.cds_lfs_pop_all_blocking clean
