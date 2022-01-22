# $NetBSD: opt-debug-hash.mk,v 1.2 2022/01/22 17:10:51 rillig Exp $
#
# Tests for the -dh command line option, which adds debug logging for
# hash tables.  Even more detailed logging is available by compiling
# make with -DDEBUG_HASH_LOOKUP.

.MAKEFLAGS: -dh

.error

# FIXME: There is a newline missing between 'continueHashTable'.
# expect: make: Fatal errors encountered -- cannot continueHashTable targets: size=16 numEntries=0 maxchain=0
