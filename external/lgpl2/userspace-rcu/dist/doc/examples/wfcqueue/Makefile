# SPDX-FileCopyrightText: 2013 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
#
# SPDX-License-Identifier: MIT

# This makefile is purposefully kept simple to support GNU and BSD make.

all:
	$(AM_V_at)$(MAKE) -f Makefile.cds_wfcq_enqueue
	$(AM_V_at)$(MAKE) -f Makefile.cds_wfcq_dequeue
	$(AM_V_at)$(MAKE) -f Makefile.cds_wfcq_splice

.PHONY: clean
clean:
	$(AM_V_at)$(MAKE) -f Makefile.cds_wfcq_enqueue clean
	$(AM_V_at)$(MAKE) -f Makefile.cds_wfcq_dequeue clean
	$(AM_V_at)$(MAKE) -f Makefile.cds_wfcq_splice clean
