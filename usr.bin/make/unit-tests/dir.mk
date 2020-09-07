# $NetBSD: dir.mk,v 1.5 2020/09/07 19:48:08 rillig Exp $
#
# Tests for dir.c.

# Dependency lines may use braces for expansion.
# See DirExpandCurly for the implementation.
all: {one,two,three}

one:
	@echo 1
two:
	@echo 2
three:
	@echo 3

# The braces may start in the middle of a word.
all: f{our,ive}

four:
	@echo 4
five:
	@echo 5
six:
	@echo 6

# Nested braces work as expected since 2020-07-31 19:06 UTC.
# They had been broken at least since 2003-01-01, probably even longer.
all: {{thi,fou}r,fif}teen

thirteen:
	@echo 13
fourteen:
	@echo 14
fifteen:
	@echo 15

# There may be multiple brace groups side by side.
all: {pre-,}{patch,configure}

pre-patch patch pre-configure configure:
	@echo $@

# Empty pieces are allowed in the braces.
all: {fetch,extract}{,-post}

fetch fetch-post extract extract-post:
	@echo $@

# The expansions may have duplicates.
# These are merged together because of the dependency line.
all: dup-{1,1,1,1,1,1,1}

dup-1:
	@echo $@

# Other than in Bash, the braces are also expanded if there is no comma.
all: {{{{{{{{{{single-word}}}}}}}}}}

single-word:
	@echo $@
