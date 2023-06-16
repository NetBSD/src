# $NetBSD: directive-include-guard.mk,v 1.1 2023/06/16 09:25:13 rillig Exp $
#
# Tests for multiple-inclusion guards in makefiles.
#
# A file that is guarded by a multiple-inclusion guard has the following form:
#
#	.ifndef GUARD_NAME
#	GUARD_NAME=	# any value
#	...
#	.endif
#
# When such a file is included for the second time, it has no effect as all
# its content is skipped.
#
# TODO: In these cases, do not include the file, to increase performance.


# This is the canonical form of a multiple-inclusion guard.
INCS+=	guarded-ifndef
LINES.guarded-ifndef= \
	'.ifndef GUARDED_IFNDEF' \
	'GUARDED_IFNDEF=' \
	'.endif'

# Comments and empty lines have no influence on the multiple-inclusion guard.
INCS+=	comments
LINES.comments= \
	'\# comment' \
	'' \
	'.ifndef GUARD' \
	'\# comment' \
	'GUARD=\#comment' \
	'.endif' \
	'\# comment'

# An alternative form uses the 'defined' function.  It is more verbose than
# the canonical form.  There are other possible forms as well, such as with a
# triple negation, but these are not recognized as they are not common.
INCS+=	guarded-if
LINES.guarded-if= \
	'.if !defined(GUARDED_IF)' \
	'GUARDED_IF=' \
	'.endif'

# Triple negation is so uncommon that it's not recognized.
INCS+=	triple-negation
LINES.triple-negation= \
	'.if !!!defined(TRIPLE_NEGATION)' \
	'TRIPLE_NEGATION=' \
	'.endif'

# The variable names in the '.if' and the assignment must be the same.
INCS+=	varname-mismatch
LINES.varname-mismatch= \
	'.ifndef VARNAME_MISMATCH' \
	'OTHER_NAME=' \
	'.endif'

# The variable name in the assignment must only contain alphanumeric
# characters and underscores, in particular, it must not be a dynamically
# computed name.
INCS+=	varname-indirect
LINES.varname-indirect= \
	'.ifndef VARNAME_INDIRECT' \
	'VARNAME_$${:UINDIRECT}=' \
	'.endif'

# The variable assignment for the guard must directly follow the conditional.
INCS+=	late-assignment
LINES.late-assignment= \
	'.ifndef LATE_ASSIGNMENT' \
	'OTHER=' \
	'LATE_ASSIGNMENT=' \
	'.endif'

# There must be no other condition between the guard condition and the
# variable assignment.
INCS+=	two-conditions
LINES.two-conditions= \
	'.ifndef TWO_CONDITIONS' \
	'.  if 0' \
	'TWO_CONDITIONS=' \
	'.  endif' \
	'.endif'

# If the guard variable is already set before the file is included for the
# first time, that file is not considered to be guarded.  It's a situation
# that is uncommon in practice.
INCS+=	already-set
LINES.already-set= \
	'.ifndef ALREADY_SET' \
	'ALREADY_SET=' \
	'.endif'
ALREADY_SET=

# The whole file content must be guarded by a single '.if' conditional, not by
# several, even if they have the same effect.
INCS+=	twice
LINES.twice= \
	'.ifndef TWICE' \
	'TWICE=' \
	'.endif' \
	'.ifndef TWICE' \
	'TWICE=' \
	'.endif'

# When multiple files use the same guard variable name, they exclude each
# other.  It's the responsibility of the makefile authors to choose suitable
# variable names.  Typical choices are ${PROJECT}_${DIR}_${FILE}_MK.
INCS+=	reuse
LINES.reuse= \
	${LINES.guarded-if}

# The conditional must come before the assignment, otherwise the conditional
# is useless, as it always evaluates to false.
INCS+=	swapped
LINES.swapped= \
	'SWAPPED=' \
	'.ifndef SWAPPED' \
	'.endif'


# Include each of the files twice.  The directive-include-guard.exp file
# contains a single entry for the files whose multiple-inclusion guard works,
# and two entries for the files that are not protected against multiple
# inclusion.
#
# Some debug output lines are suppressed in the .exp file, see ./Makefile.
.for i in ${INCS}
.  for fname in directive-include-guard-$i.tmp
_!=	printf '%s\n' ${LINES.$i} > ${fname}
.MAKEFLAGS: -dp
.include "${.CURDIR}/${fname}"
.include "${.CURDIR}/${fname}"
.MAKEFLAGS: -d0
_!=	rm ${fname}
.  endfor
.endfor

all:
