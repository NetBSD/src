# $Id: escape.mk,v 1.2 2014/08/24 13:07:25 apb Exp $
#
# Test backslash escaping.

# Extracts from the POSIX 2008 specification
# <http://pubs.opengroup.org/onlinepubs/9699919799/utilities/make.html>:
#
#     Comments start with a <number-sign> ( '#' ) and continue until an
#     unescaped <newline> is reached.
#
#     When an escaped <newline> (one preceded by a <backslash>) is found
#     anywhere in the makefile except in a command line, an include
#     line, or a line immediately preceding an include line, it shall
#     be replaced, along with any leading white space on the following
#     line, with a single <space>.
#
#     When an escaped <newline> is found in a command line in a
#     makefile, the command line shall contain the <backslash>, the
#     <newline>, and the next line, except that the first character of
#     the next line shall not be included if it is a <tab>.
#
#     When an escaped <newline> is found in an include line or in a
#     line immediately preceding an include line, the behavior is
#     unspecified.
#
# Notice that the behaviour of <backslash><backslash> or
# <backslash><anything other than newline> is not mentioned.  I think
# this implies that <backslash> should be taken literally everywhere
# except before <newline>.

all: .PHONY
# We will add dependencies like "all: yet-another-test" later.

# Some variables to be expanded in tests
#
a = aaa
A = ${a}

# Backslash at end of line in a comment\
should continue the comment. \
# This is also tested in comment.mk.

# Embedded backslash in variable should be taken literally.
#
VAR1BS = 111\111
VAR1BSa = 111\${a}
VAR1BSA = 111\${A}
VAR1BSda = 111\$${a}
VAR1BSdA = 111\$${A}

all: var-1bs
var-1bs:	.PHONY
	@echo ${.TARGET}
	@echo VAR1BS=:${VAR1BS:Q}:
	@echo VAR1BSa=:${VAR1BSa:Q}:
	@echo VAR1BSA=:${VAR1BSA:Q}:
	@echo VAR1BSda=:${VAR1BSda:Q}:
	@echo VAR1BSdA=:${VAR1BSdA:Q}:

# Double backslash in variable should be taken as two literal backslashes.
#
VAR2BS = 222\\222
VAR2BSa = 222\\${a}
VAR2BSA = 222\\${A}
VAR2BSda = 222\\$${a}
VAR2BSdA = 222\\$${A}

all: var-2bs
var-2bs:	.PHONY
	@echo ${.TARGET}
	@echo VAR2BS=:${VAR2BS:Q}:
	@echo VAR2BSa=:${VAR2BSa:Q}:
	@echo VAR2BSA=:${VAR2BSA:Q}:
	@echo VAR2BSda=:${VAR2BSda:Q}:
	@echo VAR2BSdA=:${VAR2BSdA:Q}:

# Backslash-newline in a variable setting is replaced by a single space.
#
VAR1BSNL = 111\
111
VAR1BSNLa = 111\
${a}
VAR1BSNLA = 111\
${A}
VAR1BSNLda = 111\
$${a}
VAR1BSNLdA = 111\
$${A}
VAR1BSNLc = 111\
# this should not be a comment, it should be part of the value

all: var-1bsnl
var-1bsnl:	.PHONY
	@echo ${.TARGET}
	@echo VAR1BSNL=:${VAR1BSNL:Q}:
	@echo VAR1BSNLa=:${VAR1BSNLa:Q}:
	@echo VAR1BSNLA=:${VAR1BSNLA:Q}:
	@echo VAR1BSNLda=:${VAR1BSNLa:Q}:
	@echo VAR1BSNLdA=:${VAR1BSNLA:Q}:
	@echo VAR1BSNLc=:${VAR1BSNLc:Q}:

# Double-backslash-newline in a variable setting.
# First one should be taken literally, and last should escape the newline.
# XXX: Is the expected behaviour well defined?
#
# The second lines below start with '#' so they should not generate
# syntax errors regardless of whether or not they are treated as
# part of the value.
#
VAR2BSNL = 222\\
#222
VAR2BSNLa = 222\\
#${a}
VAR2BSNLA = 222\\
#${A}
VAR2BSNLda = 222\\
#$${a}
VAR2BSNLdA = 222\\
#$${A}
VAR2BSNLc = 222\\
# this should not be a comment, it should be part of the value

all: var-2bsnl
var-2bsnl:	.PHONY
	@echo ${.TARGET}
	@echo VAR2BSNL=:${VAR2BSNL:Q}:
	@echo VAR2BSNLa=:${VAR2BSNLa:Q}:
	@echo VAR2BSNLA=:${VAR2BSNLA:Q}:
	@echo VAR2BSNLda=:${VAR2BSNLa:Q}:
	@echo VAR2BSNLdA=:${VAR2BSNLA:Q}:
	@echo VAR2BSNLc=:${VAR2BSNLc:Q}:

# Triple-backslash-newline in a variable setting.
# First two should be taken literally, and last should escape the newline.
# XXX: Is the expected behaviour well defined?
#
# The second lines below start with '#' so they should not generate
# syntax errors regardless of whether or not they ar treated as
# part of the value.
#
VAR3BSNL = 333\\\
#333
VAR3BSNLa = 333\\\
#${a}
VAR3BSNLA = 333\\\
#${A}
VAR3BSNLda = 333\\\
#$${a}
VAR3BSNLdA = 333\\\
#$${A}
VAR3BSNLc = 333\\\
# this should not be a comment, it should be part of the value

all: var-3bsnl
var-3bsnl:	.PHONY
	@echo ${.TARGET}
	@echo VAR3BSNL=:${VAR3BSNL:Q}:
	@echo VAR3BSNLa=:${VAR3BSNLa:Q}:
	@echo VAR3BSNLA=:${VAR3BSNLA:Q}:
	@echo VAR3BSNLda=:${VAR3BSNLa:Q}:
	@echo VAR3BSNLdA=:${VAR3BSNLA:Q}:
	@echo VAR3BSNLc=:${VAR3BSNLc:Q}:

# Backslash-newline in a variable setting, plus any amount of white space
# on the next line, is replaced by a single space.
#
VAR1BSNL00= first line\

# above line is entirely empty, and this is a comment
VAR1BSNL0= first line\
no space on second line
VAR1BSNLs= first line\
 one space on second line
VAR1BSNLss= first line\
  two spaces on second line
VAR1BSNLt= first line\
	one tab on second line
VAR1BSNLtt= first line\
		two tabs on second line
VAR1BSNLxx= first line\
  	 	 	 many spaces and tabs [  	 ] on second line

all: var-1bsnl-space
var-1bsnl-space:	.PHONY
	@echo ${.TARGET}
	@echo VAR1BSNL00=:${VAR1BSNL00:Q}:
	@echo VAR1BSNL0=:${VAR1BSNL0:Q}:
	@echo VAR1BSNLs=:${VAR1BSNLs:Q}:
	@echo VAR1BSNLss=:${VAR1BSNLss:Q}:
	@echo VAR1BSNLt=:${VAR1BSNLt:Q}:
	@echo VAR1BSNLtt=:${VAR1BSNLtt:Q}:
	@echo VAR1BSNLxx=:${VAR1BSNLxx:Q}:

# Backslash-newline in a command is retained.
#
# The "#" in "# second line without space" makes it a comment instead
# of a syntax error if the preceding line is parsed incorretly.
# The ":" in "third line':" makes it look like the start of a
# target instead of a syntax error if the first line is parsed incorrectly.
#
all: cmd-1bsnl
cmd-1bsnl: .PHONY
	@echo ${.TARGET}
	@echo :'first line\
#second line without space\
third line':
	@echo :'first line\
     second line spaces should be retained':
	@echo :'first line\
	second line tab should be elided':
	@echo :'first line\
		only one tab should be elided, second tab remains'

# Double-backslash-newline in a command is retained.
#
all: cmd-2bsnl
cmd-2bsnl: .PHONY
	@echo ${.TARGET}
	@echo :'first line\\
#second line without space\\
third line':
	@echo :'first line\\
     second line spaces should be retained':
	@echo :'first line\\
	second line tab should be elided':
	@echo :'first line\\
		only one tab should be elided, second tab remains'

# Triple-backslash-newline in a command is retained.
#
all: cmd-3bsnl
cmd-3bsnl: .PHONY
	@echo ${.TARGET}
	@echo :'first line\\\
#second line without space\\\
third line':
	@echo :'first line\\\
     second line spaces should be retained':
	@echo :'first line\\\
	second line tab should be elided':
	@echo :'first line\\\
		only one tab should be elided, second tab remains'
