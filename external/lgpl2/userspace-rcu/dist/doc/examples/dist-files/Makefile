# SPDX-FileCopyrightText: 2013 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
#
# SPDX-License-Identifier: MIT

# This makefile is purposefully kept simple to support GNU and BSD make.

all:
	$(AM_V_at)$(MAKE) -C hlist
	$(AM_V_at)$(MAKE) -C list
	$(AM_V_at)$(MAKE) -C urcu-flavors
	$(AM_V_at)$(MAKE) -C wfcqueue
	$(AM_V_at)$(MAKE) -C rculfqueue
	$(AM_V_at)$(MAKE) -C wfstack
	$(AM_V_at)$(MAKE) -C lfstack
	$(AM_V_at)$(MAKE) -C rculfhash

.PHONY: clean
clean:
	$(AM_V_at)$(MAKE) -C hlist clean
	$(AM_V_at)$(MAKE) -C list clean
	$(AM_V_at)$(MAKE) -C urcu-flavors clean
	$(AM_V_at)$(MAKE) -C wfcqueue clean
	$(AM_V_at)$(MAKE) -C rculfqueue clean
	$(AM_V_at)$(MAKE) -C wfstack clean
	$(AM_V_at)$(MAKE) -C lfstack clean
	$(AM_V_at)$(MAKE) -C rculfhash clean
