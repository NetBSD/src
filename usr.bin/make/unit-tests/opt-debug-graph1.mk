# $NetBSD: opt-debug-graph1.mk,v 1.2 2020/09/05 06:36:40 rillig Exp $
#
# Tests for the -dg1 command line option, which prints the input
# graph before making anything.

.MAKEFLAGS: -dg1

all: made-target made-target-no-sources

made-target: made-source

made-source:

made-target-no-sources:

unmade-target: unmade-sources

unmade-target-no-sources:

all:
	@:;
