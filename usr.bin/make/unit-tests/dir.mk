# $NetBSD: dir.mk,v 1.6 2020/10/31 21:12:36 rillig Exp $
#
# Tests for dir.c.

# Dependency lines may use braces for expansion.
# See DirExpandCurly for the implementation.
all: {one,two,three}

one:
	: 1
two:
	: 2
three:
	: 3

# The braces may start in the middle of a word.
all: f{our,ive}

four:
	: 4
five:
	: 5
six:
	: 6

# Nested braces work as expected since 2020-07-31 19:06 UTC.
# They had been broken at least since 2003-01-01, probably even longer.
all: {{thi,fou}r,fif}teen

thirteen:
	: 13
fourteen:
	: 14
fifteen:
	: 15

# There may be multiple brace groups side by side.
all: {pre-,}{patch,configure}

pre-patch patch pre-configure configure:
	: $@

# Empty pieces are allowed in the braces.
all: {fetch,extract}{,-post}

fetch fetch-post extract extract-post:
	: $@

# The expansions may have duplicates.
# These are merged together because of the dependency line.
all: dup-{1,1,1,1,1,1,1}

dup-1:
	: $@

# Other than in Bash, the braces are also expanded if there is no comma.
all: {{{{{{{{{{single-word}}}}}}}}}}

single-word:
	: $@
