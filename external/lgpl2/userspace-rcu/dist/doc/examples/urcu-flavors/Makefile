# SPDX-FileCopyrightText: 2013 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
#
# SPDX-License-Identifier: MIT

# This makefile is purposefully kept simple to support GNU and BSD make.

all:
	$(AM_V_at)$(MAKE) -f Makefile.qsbr
	$(AM_V_at)$(MAKE) -f Makefile.mb
	$(AM_V_at)$(MAKE) -f Makefile.membarrier
	$(AM_V_at)$(MAKE) -f Makefile.bp

.PHONY: clean
clean:
	$(AM_V_at)$(MAKE) -f Makefile.qsbr clean
	$(AM_V_at)$(MAKE) -f Makefile.mb clean
	$(AM_V_at)$(MAKE) -f Makefile.membarrier clean
	$(AM_V_at)$(MAKE) -f Makefile.bp clean
