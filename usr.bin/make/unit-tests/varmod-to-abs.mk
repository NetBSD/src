# $NetBSD: varmod-to-abs.mk,v 1.3 2020/11/14 18:47:21 rillig Exp $
#
# Tests for the :tA variable modifier, which returns the absolute path for
# each of the words in the variable value.

# TODO: Implementation

# Since 2016-06-03, it is possible to trick the :tA modifier into resolving
# completely unrelated absolute paths by defining a global variable that has
# the same name as the path that is to be resolved.  There are a few
# restrictions though: The "redirected" path must start with a slash, and it
# must exist. (See ModifyWord_Realpath).
#
# XXX: This is probably not intended.  It is caused by cached_realpath using
# a GNode for keeping the cache, instead of a simple HashTable.
does-not-exist.c=	/dev/null
.info ${does-not-exist.c:L:tA}

all:
	@:;
