# SPDX-FileCopyrightText: 2013 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
#
# SPDX-License-Identifier: MIT

# This makefile is purposefully kept simple to support GNU and BSD make.

all:
	$(AM_V_at)$(MAKE) -f Makefile.cds_lfq_enqueue
	$(AM_V_at)$(MAKE) -f Makefile.cds_lfq_dequeue

.PHONY: clean
clean:
	$(AM_V_at)$(MAKE) -f Makefile.cds_lfq_enqueue clean
	$(AM_V_at)$(MAKE) -f Makefile.cds_lfq_dequeue clean
