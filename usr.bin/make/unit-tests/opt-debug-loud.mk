# $NetBSD: opt-debug-loud.mk,v 1.1 2020/09/05 06:20:51 rillig Exp $
#
# Tests for the -dl command line option, which prints the commands before
# running them, ignoring the command line option for silent mode (-s) as
# well as the .SILENT special source and target, as well as the '@' prefix
# for shell commands.

# TODO: Implementation

# TODO: What about ${:!cmd!}?

all:
	@:;
