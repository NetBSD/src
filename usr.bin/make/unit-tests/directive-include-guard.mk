# $NetBSD: directive-include-guard.mk,v 1.6 2023/06/19 20:07:35 rillig Exp $
#
# Tests for multiple-inclusion guards in makefiles.
#
# A file that is guarded by a multiple-inclusion guard has the following form:
#
#	.ifndef GUARD_NAME
#	...
#	GUARD_NAME=	# any value, may also be empty
#	...
#	.endif
#
# When such a file is included later and the guard variable is set, including
# the file has no effect, as all its content is skipped.
#
# See also:
#	https://gcc.gnu.org/onlinedocs/cppinternals/Guard-Macros.html


# This is the canonical form of a multiple-inclusion guard.
INCS+=	guarded-ifndef
LINES.guarded-ifndef= \
	'.ifndef GUARDED_IFNDEF' \
	'GUARDED_IFNDEF=' \
	'.endif'
# expect: Parse_PushInput: file guarded-ifndef.tmp, line 1
# expect: Skipping 'guarded-ifndef.tmp' because 'GUARDED_IFNDEF' is already set

# Comments and empty lines have no influence on the multiple-inclusion guard.
INCS+=	comments
LINES.comments= \
	'\# comment' \
	'' \
	'.ifndef COMMENTS' \
	'\# comment' \
	'COMMENTS=\#comment' \
	'.endif' \
	'\# comment'
# expect: Parse_PushInput: file comments.tmp, line 1
# expect: Skipping 'comments.tmp' because 'COMMENTS' is already set

# An alternative form uses the 'defined' function.  It is more verbose than
# the canonical form.  There are other possible forms as well, such as with a
# triple negation, but these are not recognized as they are not common.
INCS+=	guarded-if
LINES.guarded-if= \
	'.if !defined(GUARDED_IF)' \
	'GUARDED_IF=' \
	'.endif'
# expect: Parse_PushInput: file guarded-if.tmp, line 1
# expect: Skipping 'guarded-if.tmp' because 'GUARDED_IF' is already set

# Triple negation is so uncommon that it's not recognized.
INCS+=	triple-negation
LINES.triple-negation= \
	'.if !!!defined(TRIPLE_NEGATION)' \
	'TRIPLE_NEGATION=' \
	'.endif'
# expect: Parse_PushInput: file triple-negation.tmp, line 1
# expect: Parse_PushInput: file triple-negation.tmp, line 1

# A conditional other than '.if' or '.ifndef' marks the file as non-guarded,
# even if it would actually work as a multiple-inclusion guard.
INCS+=	ifdef-negated
LINES.ifdef-negated= \
	'.ifdef !IFDEF_NEGATED' \
	'IFDEF_NEGATED=' \
	'.endif'
# expect: Parse_PushInput: file ifdef-negated.tmp, line 1
# expect: Parse_PushInput: file ifdef-negated.tmp, line 1

# The variable names in the '.if' and the assignment must be the same.
INCS+=	varname-mismatch
LINES.varname-mismatch= \
	'.ifndef VARNAME_MISMATCH' \
	'OTHER_NAME=' \
	'.endif'
# expect: Parse_PushInput: file varname-mismatch.tmp, line 1
# expect: Parse_PushInput: file varname-mismatch.tmp, line 1

# The guard condition must consist of only the guard variable, nothing else.
INCS+=	ifndef-plus
LINES.ifndef-plus= \
	'.ifndef IFNDEF_PLUS && IFNDEF_SECOND' \
	'IFNDEF_PLUS=' \
	'IFNDEF_SECOND=' \
	'.endif'
# expect: Parse_PushInput: file ifndef-plus.tmp, line 1
# expect: Parse_PushInput: file ifndef-plus.tmp, line 1

# The guard condition must consist of only the guard variable, nothing else.
INCS+=	if-plus
LINES.if-plus= \
	'.if !defined(IF_PLUS) && !defined(IF_SECOND)' \
	'IF_PLUS=' \
	'IF_SECOND=' \
	'.endif'
# expect: Parse_PushInput: file if-plus.tmp, line 1
# expect: Parse_PushInput: file if-plus.tmp, line 1

# The variable name in an '.ifndef' guard must be given directly, it must not
# contain any '$' expression.
INCS+=	ifndef-indirect
LINES.ifndef-indirect= \
	'.ifndef $${IFNDEF_INDIRECT:L}' \
	'IFNDEF_INDIRECT=' \
	'.endif'
# expect: Parse_PushInput: file ifndef-indirect.tmp, line 1
# expect: Parse_PushInput: file ifndef-indirect.tmp, line 1

# The variable name in an '.if' guard must be given directly, it must not contain
# any '$' expression.
INCS+=	if-indirect
LINES.if-indirect= \
	'.if !defined($${IF_INDIRECT:L})' \
	'IF_INDIRECT=' \
	'.endif'
# expect: Parse_PushInput: file if-indirect.tmp, line 1
# expect: Parse_PushInput: file if-indirect.tmp, line 1

# The variable name in the guard condition must only contain alphanumeric
# characters and underscores.  The guard variable is more flexible, it can be
# set anywhere, as long as it is set when the guarded file is included next.
INCS+=	varassign-indirect
LINES.varassign-indirect= \
	'.ifndef VARASSIGN_INDIRECT' \
	'$${VARASSIGN_INDIRECT:L}=' \
	'.endif'
# expect: Parse_PushInput: file varassign-indirect.tmp, line 1
# expect: Skipping 'varassign-indirect.tmp' because 'VARASSIGN_INDIRECT' is already set

# The time at which the guard variable is set doesn't matter, as long as it is
# set when the file is included the next time.
INCS+=	late-assignment
LINES.late-assignment= \
	'.ifndef LATE_ASSIGNMENT' \
	'OTHER=' \
	'LATE_ASSIGNMENT=' \
	'.endif'
# expect: Parse_PushInput: file late-assignment.tmp, line 1
# expect: Skipping 'late-assignment.tmp' because 'LATE_ASSIGNMENT' is already set

# The time at which the guard variable is set doesn't matter, as long as it is
# set when the file is included the next time.
INCS+=	two-conditions
LINES.two-conditions= \
	'.ifndef TWO_CONDITIONS' \
	'.  if 1' \
	'TWO_CONDITIONS=' \
	'.  endif' \
	'.endif'
# expect: Parse_PushInput: file two-conditions.tmp, line 1
# expect: Skipping 'two-conditions.tmp' because 'TWO_CONDITIONS' is already set

# If the guard variable is already set before the file is included for the
# first time, the file is not considered guarded, as the makefile parser skips
# all lines in the inactive part between the '.ifndef' and the '.endif'.
INCS+=	already-set
LINES.already-set= \
	'.ifndef ALREADY_SET' \
	'ALREADY_SET=' \
	'.endif'
ALREADY_SET=
# expect: Parse_PushInput: file already-set.tmp, line 1
# expect: Parse_PushInput: file already-set.tmp, line 1

# The whole file content must be guarded by a single '.if' conditional, not by
# several, even if they have the same effect.
INCS+=	twice
LINES.twice= \
	'.ifndef TWICE_FIRST' \
	'TWICE_FIRST=' \
	'.endif' \
	'.ifndef TWICE_SECOND' \
	'TWICE_SECOND=' \
	'.endif'
# expect: Parse_PushInput: file twice.tmp, line 1
# expect: Parse_PushInput: file twice.tmp, line 1

# When multiple files use the same guard variable name, they exclude each
# other.  It's the responsibility of the makefile authors to choose unique
# variable names.  Typical choices are ${PROJECT}_${DIR}_${FILE}_MK.  This is
# the same situation as in the 'already-set' test, and the file is not
# considered guarded.
INCS+=	reuse
LINES.reuse= \
	${LINES.guarded-if}
# expect: Parse_PushInput: file reuse.tmp, line 1
# expect: Parse_PushInput: file reuse.tmp, line 1

# The conditional must come before the assignment, otherwise the conditional
# is useless, as it always evaluates to false.
INCS+=	swapped
LINES.swapped= \
	'SWAPPED=' \
	'.ifndef SWAPPED' \
	'.endif'
# expect: Parse_PushInput: file swapped.tmp, line 1
# expect: Parse_PushInput: file swapped.tmp, line 1

# If the guard variable is undefined at some later point, the guarded file is
# included again.
INCS+=	undef-between
LINES.undef-between= \
	'.ifndef UNDEF_BETWEEN' \
	'UNDEF_BETWEEN=' \
	'.endif'
# expect: Parse_PushInput: file undef-between.tmp, line 1
# expect: Parse_PushInput: file undef-between.tmp, line 1

# If the guarded file undefines the guard variable, the guarded file is
# included again.
INCS+=	undef-inside
LINES.undef-inside= \
	'.ifndef UNDEF_INSIDE' \
	'UNDEF_INSIDE=' \
	'.undef UNDEF_INSIDE' \
	'.endif'
# expect: Parse_PushInput: file undef-inside.tmp, line 1
# expect: Parse_PushInput: file undef-inside.tmp, line 1

# The outermost '.if' must not have an '.elif' branch.
INCS+=	if-elif
LINES.if-elif = \
	'.ifndef IF_ELIF' \
	'IF_ELIF=' \
	'.elif 1' \
	'.endif'
# expect: Parse_PushInput: file if-elif.tmp, line 1
# expect: Parse_PushInput: file if-elif.tmp, line 1

# The outermost '.if' must not have an '.else' branch.
INCS+=	if-else
LINES.if-else = \
	'.ifndef IF_ELSE' \
	'IF_ELSE=' \
	'.else' \
	'.endif'
# expect: Parse_PushInput: file if-else.tmp, line 1
# expect: Parse_PushInput: file if-else.tmp, line 1

# The inner '.if' directives may have an '.elif' or '.else'.
INCS+=	inner-if-elif-else
LINES.inner-if-elif-else = \
	'.ifndef INNER_IF_ELIF_ELSE' \
	'INNER_IF_ELIF_ELSE=' \
	'.  if 0' \
	'.  elif 0' \
	'.  else' \
	'.  endif' \
	'.  if 0' \
	'.  elif 1' \
	'.  else' \
	'.  endif' \
	'.  if 1' \
	'.  elif 1' \
	'.  else' \
	'.  endif' \
	'.endif'
# expect: Parse_PushInput: file inner-if-elif-else.tmp, line 1
# expect: Skipping 'inner-if-elif-else.tmp' because 'INNER_IF_ELIF_ELSE' is already set


# Include each of the files twice.  The directive-include-guard.exp file
# contains a single entry for the files whose multiple-inclusion guard works,
# and two entries for the files that are not protected against multiple
# inclusion.
#
# Some debug output lines are suppressed in the .exp file, see ./Makefile.
.for i in ${INCS}
.  for fname in $i.tmp
_!=	printf '%s\n' ${LINES.$i} > ${fname}
.MAKEFLAGS: -dp
.include "${.CURDIR}/${fname}"
.undef ${i:Mundef-between:%=UNDEF_BETWEEN}
.include "${.CURDIR}/${fname}"
.MAKEFLAGS: -d0
_!=	rm ${fname}
.  endfor
.endfor

all:
