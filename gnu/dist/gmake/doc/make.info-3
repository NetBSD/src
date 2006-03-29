This is make.info, produced by makeinfo version 4.2 from make.texi.

INFO-DIR-SECTION GNU Packages
START-INFO-DIR-ENTRY
* Make: (make).            Remake files automatically.
END-INFO-DIR-ENTRY

   This file documents the GNU Make utility, which determines
automatically which pieces of a large program need to be recompiled,
and issues the commands to recompile them.

   This is Edition 0.60, last updated 08 July 2002, of `The GNU Make
Manual', for `make', Version 3.80.

   Copyright 1988, 1989, 1990, 1991, 1992, 1993, 1994, 1995, 1996,
1997, 1998, 1999, 2000, 2002 Free Software Foundation, Inc.

   Permission is granted to copy, distribute and/or modify this document
under the terms of the GNU Free Documentation License, Version 1.1 or
any later version published by the Free Software Foundation; with no
Invariant Sections, with no Front-Cover Texts, and with no Back-Cover
Texts.  A copy of the license is included in the section entitled "GNU
Free Documentation License".


File: make.info,  Node: Multiple Targets,  Next: Multiple Rules,  Prev: Special Targets,  Up: Rules

Multiple Targets in a Rule
==========================

   A rule with multiple targets is equivalent to writing many rules,
each with one target, and all identical aside from that.  The same
commands apply to all the targets, but their effects may vary because
you can substitute the actual target name into the command using `$@'.
The rule contributes the same prerequisites to all the targets also.

   This is useful in two cases.

   * You want just prerequisites, no commands.  For example:

          kbd.o command.o files.o: command.h

     gives an additional prerequisite to each of the three object files
     mentioned.

   * Similar commands work for all the targets.  The commands do not
     need to be absolutely identical, since the automatic variable `$@'
     can be used to substitute the particular target to be remade into
     the commands (*note Automatic Variables: Automatic.).  For example:

          bigoutput littleoutput : text.g
                  generate text.g -$(subst output,,$@) > $@

     is equivalent to

          bigoutput : text.g
                  generate text.g -big > bigoutput
          littleoutput : text.g
                  generate text.g -little > littleoutput

     Here we assume the hypothetical program `generate' makes two types
     of output, one if given `-big' and one if given `-little'.  *Note
     Functions for String Substitution and Analysis: Text Functions,
     for an explanation of the `subst' function.

   Suppose you would like to vary the prerequisites according to the
target, much as the variable `$@' allows you to vary the commands.  You
cannot do this with multiple targets in an ordinary rule, but you can
do it with a "static pattern rule".  *Note Static Pattern Rules: Static
Pattern.


File: make.info,  Node: Multiple Rules,  Next: Static Pattern,  Prev: Multiple Targets,  Up: Rules

Multiple Rules for One Target
=============================

   One file can be the target of several rules.  All the prerequisites
mentioned in all the rules are merged into one list of prerequisites for
the target.  If the target is older than any prerequisite from any rule,
the commands are executed.

   There can only be one set of commands to be executed for a file.  If
more than one rule gives commands for the same file, `make' uses the
last set given and prints an error message.  (As a special case, if the
file's name begins with a dot, no error message is printed.  This odd
behavior is only for compatibility with other implementations of
`make'... you should avoid using it).  Occasionally it is useful to
have the same target invoke multiple commands which are defined in
different parts of your makefile; you can use "double-colon rules"
(*note Double-Colon::) for this.

   An extra rule with just prerequisites can be used to give a few extra
prerequisites to many files at once.  For example, makefiles often have
a variable, such as `objects', containing a list of all the compiler
output files in the system being made.  An easy way to say that all of
them must be recompiled if `config.h' changes is to write the following:

     objects = foo.o bar.o
     foo.o : defs.h
     bar.o : defs.h test.h
     $(objects) : config.h

   This could be inserted or taken out without changing the rules that
really specify how to make the object files, making it a convenient
form to use if you wish to add the additional prerequisite
intermittently.

   Another wrinkle is that the additional prerequisites could be
specified with a variable that you set with a command argument to `make'
(*note Overriding Variables: Overriding.).  For example,

     extradeps=
     $(objects) : $(extradeps)

means that the command `make extradeps=foo.h' will consider `foo.h' as
a prerequisite of each object file, but plain `make' will not.

   If none of the explicit rules for a target has commands, then `make'
searches for an applicable implicit rule to find some commands *note
Using Implicit Rules: Implicit Rules.).


File: make.info,  Node: Static Pattern,  Next: Double-Colon,  Prev: Multiple Rules,  Up: Rules

Static Pattern Rules
====================

   "Static pattern rules" are rules which specify multiple targets and
construct the prerequisite names for each target based on the target
name.  They are more general than ordinary rules with multiple targets
because the targets do not have to have identical prerequisites.  Their
prerequisites must be _analogous_, but not necessarily _identical_.

* Menu:

* Static Usage::                The syntax of static pattern rules.
* Static versus Implicit::      When are they better than implicit rules?


File: make.info,  Node: Static Usage,  Next: Static versus Implicit,  Prev: Static Pattern,  Up: Static Pattern

Syntax of Static Pattern Rules
------------------------------

   Here is the syntax of a static pattern rule:

     TARGETS ...: TARGET-PATTERN: PREREQ-PATTERNS ...
             COMMANDS
             ...

The TARGETS list specifies the targets that the rule applies to.  The
targets can contain wildcard characters, just like the targets of
ordinary rules (*note Using Wildcard Characters in File Names:
Wildcards.).

   The TARGET-PATTERN and PREREQ-PATTERNS say how to compute the
prerequisites of each target.  Each target is matched against the
TARGET-PATTERN to extract a part of the target name, called the "stem".
This stem is substituted into each of the PREREQ-PATTERNS to make the
prerequisite names (one from each PREREQ-PATTERN).

   Each pattern normally contains the character `%' just once.  When the
TARGET-PATTERN matches a target, the `%' can match any part of the
target name; this part is called the "stem".  The rest of the pattern
must match exactly.  For example, the target `foo.o' matches the
pattern `%.o', with `foo' as the stem.  The targets `foo.c' and
`foo.out' do not match that pattern.

   The prerequisite names for each target are made by substituting the
stem for the `%' in each prerequisite pattern.  For example, if one
prerequisite pattern is `%.c', then substitution of the stem `foo'
gives the prerequisite name `foo.c'.  It is legitimate to write a
prerequisite pattern that does not contain `%'; then this prerequisite
is the same for all targets.

   `%' characters in pattern rules can be quoted with preceding
backslashes (`\').  Backslashes that would otherwise quote `%'
characters can be quoted with more backslashes.  Backslashes that quote
`%' characters or other backslashes are removed from the pattern before
it is compared to file names or has a stem substituted into it.
Backslashes that are not in danger of quoting `%' characters go
unmolested.  For example, the pattern `the\%weird\\%pattern\\' has
`the%weird\' preceding the operative `%' character, and `pattern\\'
following it.  The final two backslashes are left alone because they
cannot affect any `%' character.

   Here is an example, which compiles each of `foo.o' and `bar.o' from
the corresponding `.c' file:

     objects = foo.o bar.o
     
     all: $(objects)
     
     $(objects): %.o: %.c
             $(CC) -c $(CFLAGS) $< -o $@

Here `$<' is the automatic variable that holds the name of the
prerequisite and `$@' is the automatic variable that holds the name of
the target; see *Note Automatic Variables: Automatic.

   Each target specified must match the target pattern; a warning is
issued for each target that does not.  If you have a list of files,
only some of which will match the pattern, you can use the `filter'
function to remove nonmatching file names (*note Functions for String
Substitution and Analysis: Text Functions.):

     files = foo.elc bar.o lose.o
     
     $(filter %.o,$(files)): %.o: %.c
             $(CC) -c $(CFLAGS) $< -o $@
     $(filter %.elc,$(files)): %.elc: %.el
             emacs -f batch-byte-compile $<

In this example the result of `$(filter %.o,$(files))' is `bar.o
lose.o', and the first static pattern rule causes each of these object
files to be updated by compiling the corresponding C source file.  The
result of `$(filter %.elc,$(files))' is `foo.elc', so that file is made
from `foo.el'.

   Another example shows how to use `$*' in static pattern rules:

     bigoutput littleoutput : %output : text.g
             generate text.g -$* > $@

When the `generate' command is run, `$*' will expand to the stem,
either `big' or `little'.


File: make.info,  Node: Static versus Implicit,  Prev: Static Usage,  Up: Static Pattern

Static Pattern Rules versus Implicit Rules
------------------------------------------

   A static pattern rule has much in common with an implicit rule
defined as a pattern rule (*note Defining and Redefining Pattern Rules:
Pattern Rules.).  Both have a pattern for the target and patterns for
constructing the names of prerequisites.  The difference is in how
`make' decides _when_ the rule applies.

   An implicit rule _can_ apply to any target that matches its pattern,
but it _does_ apply only when the target has no commands otherwise
specified, and only when the prerequisites can be found.  If more than
one implicit rule appears applicable, only one applies; the choice
depends on the order of rules.

   By contrast, a static pattern rule applies to the precise list of
targets that you specify in the rule.  It cannot apply to any other
target and it invariably does apply to each of the targets specified.
If two conflicting rules apply, and both have commands, that's an error.

   The static pattern rule can be better than an implicit rule for these
reasons:

   * You may wish to override the usual implicit rule for a few files
     whose names cannot be categorized syntactically but can be given
     in an explicit list.

   * If you cannot be sure of the precise contents of the directories
     you are using, you may not be sure which other irrelevant files
     might lead `make' to use the wrong implicit rule.  The choice
     might depend on the order in which the implicit rule search is
     done.  With static pattern rules, there is no uncertainty: each
     rule applies to precisely the targets specified.


File: make.info,  Node: Double-Colon,  Next: Automatic Prerequisites,  Prev: Static Pattern,  Up: Rules

Double-Colon Rules
==================

   "Double-colon" rules are rules written with `::' instead of `:'
after the target names.  They are handled differently from ordinary
rules when the same target appears in more than one rule.

   When a target appears in multiple rules, all the rules must be the
same type: all ordinary, or all double-colon.  If they are
double-colon, each of them is independent of the others.  Each
double-colon rule's commands are executed if the target is older than
any prerequisites of that rule.  If there are no prerequisites for that
rule, its commands are always executed (even if the target already
exists).  This can result in executing none, any, or all of the
double-colon rules.

   Double-colon rules with the same target are in fact completely
separate from one another.  Each double-colon rule is processed
individually, just as rules with different targets are processed.

   The double-colon rules for a target are executed in the order they
appear in the makefile.  However, the cases where double-colon rules
really make sense are those where the order of executing the commands
would not matter.

   Double-colon rules are somewhat obscure and not often very useful;
they provide a mechanism for cases in which the method used to update a
target differs depending on which prerequisite files caused the update,
and such cases are rare.

   Each double-colon rule should specify commands; if it does not, an
implicit rule will be used if one applies.  *Note Using Implicit Rules:
Implicit Rules.


File: make.info,  Node: Automatic Prerequisites,  Prev: Double-Colon,  Up: Rules

Generating Prerequisites Automatically
======================================

   In the makefile for a program, many of the rules you need to write
often say only that some object file depends on some header file.  For
example, if `main.c' uses `defs.h' via an `#include', you would write:

     main.o: defs.h

You need this rule so that `make' knows that it must remake `main.o'
whenever `defs.h' changes.  You can see that for a large program you
would have to write dozens of such rules in your makefile.  And, you
must always be very careful to update the makefile every time you add
or remove an `#include'.

   To avoid this hassle, most modern C compilers can write these rules
for you, by looking at the `#include' lines in the source files.
Usually this is done with the `-M' option to the compiler.  For
example, the command:

     cc -M main.c

generates the output:

     main.o : main.c defs.h

Thus you no longer have to write all those rules yourself.  The
compiler will do it for you.

   Note that such a prerequisite constitutes mentioning `main.o' in a
makefile, so it can never be considered an intermediate file by implicit
rule search.  This means that `make' won't ever remove the file after
using it; *note Chains of Implicit Rules: Chained Rules..

   With old `make' programs, it was traditional practice to use this
compiler feature to generate prerequisites on demand with a command like
`make depend'.  That command would create a file `depend' containing
all the automatically-generated prerequisites; then the makefile could
use `include' to read them in (*note Include::).

   In GNU `make', the feature of remaking makefiles makes this practice
obsolete--you need never tell `make' explicitly to regenerate the
prerequisites, because it always regenerates any makefile that is out
of date.  *Note Remaking Makefiles::.

   The practice we recommend for automatic prerequisite generation is
to have one makefile corresponding to each source file.  For each
source file `NAME.c' there is a makefile `NAME.d' which lists what
files the object file `NAME.o' depends on.  That way only the source
files that have changed need to be rescanned to produce the new
prerequisites.

   Here is the pattern rule to generate a file of prerequisites (i.e.,
a makefile) called `NAME.d' from a C source file called `NAME.c':

     %.d: %.c
             
              $(CC) -M $(CPPFLAGS) $< > $@.$$$$; \
              sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
              rm -f $@.$$$$

*Note Pattern Rules::, for information on defining pattern rules.  The
`-e' flag to the shell causes it to exit immediately if the `$(CC)'
command (or any other command) fails (exits with a nonzero status).

   With the GNU C compiler, you may wish to use the `-MM' flag instead
of `-M'.  This omits prerequisites on system header files.  *Note
Options Controlling the Preprocessor: (gcc.info)Preprocessor Options,
for details.

   The purpose of the `sed' command is to translate (for example):

     main.o : main.c defs.h

into:

     main.o main.d : main.c defs.h

This makes each `.d' file depend on all the source and header files
that the corresponding `.o' file depends on.  `make' then knows it must
regenerate the prerequisites whenever any of the source or header files
changes.

   Once you've defined the rule to remake the `.d' files, you then use
the `include' directive to read them all in.  *Note Include::.  For
example:

     sources = foo.c bar.c
     
     include $(sources:.c=.d)

(This example uses a substitution variable reference to translate the
list of source files `foo.c bar.c' into a list of prerequisite
makefiles, `foo.d bar.d'.  *Note Substitution Refs::, for full
information on substitution references.)  Since the `.d' files are
makefiles like any others, `make' will remake them as necessary with no
further work from you.  *Note Remaking Makefiles::.

   Note that the `.d' files contain target definitions; you should be
sure to place the `include' directive _after_ the first, default target
in your makefiles or run the risk of having a random object file become
the default target.  *Note How Make Works::.


File: make.info,  Node: Commands,  Next: Using Variables,  Prev: Rules,  Up: Top

Writing the Commands in Rules
*****************************

   The commands of a rule consist of shell command lines to be executed
one by one.  Each command line must start with a tab, except that the
first command line may be attached to the target-and-prerequisites line
with a semicolon in between.  Blank lines and lines of just comments
may appear among the command lines; they are ignored.  (But beware, an
apparently "blank" line that begins with a tab is _not_ blank!  It is an
empty command; *note Empty Commands::.)

   Users use many different shell programs, but commands in makefiles
are always interpreted by `/bin/sh' unless the makefile specifies
otherwise.  *Note Command Execution: Execution.

   The shell that is in use determines whether comments can be written
on command lines, and what syntax they use.  When the shell is
`/bin/sh', a `#' starts a comment that extends to the end of the line.
The `#' does not have to be at the beginning of a line.  Text on a line
before a `#' is not part of the comment.

* Menu:

* Echoing::                     How to control when commands are echoed.
* Execution::                   How commands are executed.
* Parallel::                    How commands can be executed in parallel.
* Errors::                      What happens after a command execution error.
* Interrupts::                  What happens when a command is interrupted.
* Recursion::                   Invoking `make' from makefiles.
* Sequences::                   Defining canned sequences of commands.
* Empty Commands::              Defining useful, do-nothing commands.


File: make.info,  Node: Echoing,  Next: Execution,  Prev: Commands,  Up: Commands

Command Echoing
===============

   Normally `make' prints each command line before it is executed.  We
call this "echoing" because it gives the appearance that you are typing
the commands yourself.

   When a line starts with `@', the echoing of that line is suppressed.
The `@' is discarded before the command is passed to the shell.
Typically you would use this for a command whose only effect is to print
something, such as an `echo' command to indicate progress through the
makefile:

     @echo About to make distribution files

   When `make' is given the flag `-n' or `--just-print' it only echoes
commands, it won't execute them.  *Note Summary of Options: Options
Summary.  In this case and only this case, even the commands starting
with `@' are printed.  This flag is useful for finding out which
commands `make' thinks are necessary without actually doing them.

   The `-s' or `--silent' flag to `make' prevents all echoing, as if
all commands started with `@'.  A rule in the makefile for the special
target `.SILENT' without prerequisites has the same effect (*note
Special Built-in Target Names: Special Targets.).  `.SILENT' is
essentially obsolete since `@' is more flexible.


File: make.info,  Node: Execution,  Next: Parallel,  Prev: Echoing,  Up: Commands

Command Execution
=================

   When it is time to execute commands to update a target, they are
executed by making a new subshell for each line.  (In practice, `make'
may take shortcuts that do not affect the results.)

   *Please note:* this implies that shell commands such as `cd' that
set variables local to each process will not affect the following
command lines. (1)  If you want to use `cd' to affect the next command,
put the two on a single line with a semicolon between them.  Then
`make' will consider them a single command and pass them, together, to
a shell which will execute them in sequence.  For example:

     foo : bar/lose
             cd bar; gobble lose > ../foo

   If you would like to split a single shell command into multiple
lines of text, you must use a backslash at the end of all but the last
subline.  Such a sequence of lines is combined into a single line, by
deleting the backslash-newline sequences, before passing it to the
shell.  Thus, the following is equivalent to the preceding example:

     foo : bar/lose
             cd bar;  \
             gobble lose > ../foo

   The program used as the shell is taken from the variable `SHELL'.
By default, the program `/bin/sh' is used.

   On MS-DOS, if `SHELL' is not set, the value of the variable
`COMSPEC' (which is always set) is used instead.

   The processing of lines that set the variable `SHELL' in Makefiles
is different on MS-DOS.  The stock shell, `command.com', is
ridiculously limited in its functionality and many users of `make' tend
to install a replacement shell.  Therefore, on MS-DOS, `make' examines
the value of `SHELL', and changes its behavior based on whether it
points to a Unix-style or DOS-style shell.  This allows reasonable
functionality even if `SHELL' points to `command.com'.

   If `SHELL' points to a Unix-style shell, `make' on MS-DOS
additionally checks whether that shell can indeed be found; if not, it
ignores the line that sets `SHELL'.  In MS-DOS, GNU `make' searches for
the shell in the following places:

  1. In the precise place pointed to by the value of `SHELL'.  For
     example, if the makefile specifies `SHELL = /bin/sh', `make' will
     look in the directory `/bin' on the current drive.

  2. In the current directory.

  3. In each of the directories in the `PATH' variable, in order.


   In every directory it examines, `make' will first look for the
specific file (`sh' in the example above).  If this is not found, it
will also look in that directory for that file with one of the known
extensions which identify executable files.  For example `.exe',
`.com', `.bat', `.btm', `.sh', and some others.

   If any of these attempts is successful, the value of `SHELL' will be
set to the full pathname of the shell as found.  However, if none of
these is found, the value of `SHELL' will not be changed, and thus the
line that sets it will be effectively ignored.  This is so `make' will
only support features specific to a Unix-style shell if such a shell is
actually installed on the system where `make' runs.

   Note that this extended search for the shell is limited to the cases
where `SHELL' is set from the Makefile; if it is set in the environment
or command line, you are expected to set it to the full pathname of the
shell, exactly as things are on Unix.

   The effect of the above DOS-specific processing is that a Makefile
that says `SHELL = /bin/sh' (as many Unix makefiles do), will work on
MS-DOS unaltered if you have e.g. `sh.exe' installed in some directory
along your `PATH'.

   Unlike most variables, the variable `SHELL' is never set from the
environment.  This is because the `SHELL' environment variable is used
to specify your personal choice of shell program for interactive use.
It would be very bad for personal choices like this to affect the
functioning of makefiles.  *Note Variables from the Environment:
Environment.  However, on MS-DOS and MS-Windows the value of `SHELL' in
the environment *is* used, since on those systems most users do not set
this variable, and therefore it is most likely set specifically to be
used by `make'.  On MS-DOS, if the setting of `SHELL' is not suitable
for `make', you can set the variable `MAKESHELL' to the shell that
`make' should use; this will override the value of `SHELL'.

   ---------- Footnotes ----------

   (1) On MS-DOS, the value of current working directory is *global*,
so changing it _will_ affect the following command lines on those
systems.


File: make.info,  Node: Parallel,  Next: Errors,  Prev: Execution,  Up: Commands

Parallel Execution
==================

   GNU `make' knows how to execute several commands at once.  Normally,
`make' will execute only one command at a time, waiting for it to
finish before executing the next.  However, the `-j' or `--jobs' option
tells `make' to execute many commands simultaneously.

   On MS-DOS, the `-j' option has no effect, since that system doesn't
support multi-processing.

   If the `-j' option is followed by an integer, this is the number of
commands to execute at once; this is called the number of "job slots".
If there is nothing looking like an integer after the `-j' option,
there is no limit on the number of job slots.  The default number of job
slots is one, which means serial execution (one thing at a time).

   One unpleasant consequence of running several commands
simultaneously is that output generated by the commands appears
whenever each command sends it, so messages from different commands may
be interspersed.

   Another problem is that two processes cannot both take input from the
same device; so to make sure that only one command tries to take input
from the terminal at once, `make' will invalidate the standard input
streams of all but one running command.  This means that attempting to
read from standard input will usually be a fatal error (a `Broken pipe'
signal) for most child processes if there are several.

   It is unpredictable which command will have a valid standard input
stream (which will come from the terminal, or wherever you redirect the
standard input of `make').  The first command run will always get it
first, and the first command started after that one finishes will get
it next, and so on.

   We will change how this aspect of `make' works if we find a better
alternative.  In the mean time, you should not rely on any command using
standard input at all if you are using the parallel execution feature;
but if you are not using this feature, then standard input works
normally in all commands.

   Finally, handling recursive `make' invocations raises issues.  For
more information on this, see *Note Communicating Options to a
Sub-`make': Options/Recursion.

   If a command fails (is killed by a signal or exits with a nonzero
status), and errors are not ignored for that command (*note Errors in
Commands: Errors.), the remaining command lines to remake the same
target will not be run.  If a command fails and the `-k' or
`--keep-going' option was not given (*note Summary of Options: Options
Summary.), `make' aborts execution.  If make terminates for any reason
(including a signal) with child processes running, it waits for them to
finish before actually exiting.

   When the system is heavily loaded, you will probably want to run
fewer jobs than when it is lightly loaded.  You can use the `-l' option
to tell `make' to limit the number of jobs to run at once, based on the
load average.  The `-l' or `--max-load' option is followed by a
floating-point number.  For example,

     -l 2.5

will not let `make' start more than one job if the load average is
above 2.5.  The `-l' option with no following number removes the load
limit, if one was given with a previous `-l' option.

   More precisely, when `make' goes to start up a job, and it already
has at least one job running, it checks the current load average; if it
is not lower than the limit given with `-l', `make' waits until the load
average goes below that limit, or until all the other jobs finish.

   By default, there is no load limit.


File: make.info,  Node: Errors,  Next: Interrupts,  Prev: Parallel,  Up: Commands

Errors in Commands
==================

   After each shell command returns, `make' looks at its exit status.
If the command completed successfully, the next command line is executed
in a new shell; after the last command line is finished, the rule is
finished.

   If there is an error (the exit status is nonzero), `make' gives up on
the current rule, and perhaps on all rules.

   Sometimes the failure of a certain command does not indicate a
problem.  For example, you may use the `mkdir' command to ensure that a
directory exists.  If the directory already exists, `mkdir' will report
an error, but you probably want `make' to continue regardless.

   To ignore errors in a command line, write a `-' at the beginning of
the line's text (after the initial tab).  The `-' is discarded before
the command is passed to the shell for execution.

   For example,

     clean:
             -rm -f *.o

This causes `rm' to continue even if it is unable to remove a file.

   When you run `make' with the `-i' or `--ignore-errors' flag, errors
are ignored in all commands of all rules.  A rule in the makefile for
the special target `.IGNORE' has the same effect, if there are no
prerequisites.  These ways of ignoring errors are obsolete because `-'
is more flexible.

   When errors are to be ignored, because of either a `-' or the `-i'
flag, `make' treats an error return just like success, except that it
prints out a message that tells you the status code the command exited
with, and says that the error has been ignored.

   When an error happens that `make' has not been told to ignore, it
implies that the current target cannot be correctly remade, and neither
can any other that depends on it either directly or indirectly.  No
further commands will be executed for these targets, since their
preconditions have not been achieved.

   Normally `make' gives up immediately in this circumstance, returning
a nonzero status.  However, if the `-k' or `--keep-going' flag is
specified, `make' continues to consider the other prerequisites of the
pending targets, remaking them if necessary, before it gives up and
returns nonzero status.  For example, after an error in compiling one
object file, `make -k' will continue compiling other object files even
though it already knows that linking them will be impossible.  *Note
Summary of Options: Options Summary.

   The usual behavior assumes that your purpose is to get the specified
targets up to date; once `make' learns that this is impossible, it
might as well report the failure immediately.  The `-k' option says
that the real purpose is to test as many of the changes made in the
program as possible, perhaps to find several independent problems so
that you can correct them all before the next attempt to compile.  This
is why Emacs' `compile' command passes the `-k' flag by default.

   Usually when a command fails, if it has changed the target file at
all, the file is corrupted and cannot be used--or at least it is not
completely updated.  Yet the file's time stamp says that it is now up to
date, so the next time `make' runs, it will not try to update that
file.  The situation is just the same as when the command is killed by a
signal; *note Interrupts::.  So generally the right thing to do is to
delete the target file if the command fails after beginning to change
the file.  `make' will do this if `.DELETE_ON_ERROR' appears as a
target.  This is almost always what you want `make' to do, but it is
not historical practice; so for compatibility, you must explicitly
request it.


File: make.info,  Node: Interrupts,  Next: Recursion,  Prev: Errors,  Up: Commands

Interrupting or Killing `make'
==============================

   If `make' gets a fatal signal while a command is executing, it may
delete the target file that the command was supposed to update.  This is
done if the target file's last-modification time has changed since
`make' first checked it.

   The purpose of deleting the target is to make sure that it is remade
from scratch when `make' is next run.  Why is this?  Suppose you type
`Ctrl-c' while a compiler is running, and it has begun to write an
object file `foo.o'.  The `Ctrl-c' kills the compiler, resulting in an
incomplete file whose last-modification time is newer than the source
file `foo.c'.  But `make' also receives the `Ctrl-c' signal and deletes
this incomplete file.  If `make' did not do this, the next invocation
of `make' would think that `foo.o' did not require updating--resulting
in a strange error message from the linker when it tries to link an
object file half of which is missing.

   You can prevent the deletion of a target file in this way by making
the special target `.PRECIOUS' depend on it.  Before remaking a target,
`make' checks to see whether it appears on the prerequisites of
`.PRECIOUS', and thereby decides whether the target should be deleted
if a signal happens.  Some reasons why you might do this are that the
target is updated in some atomic fashion, or exists only to record a
modification-time (its contents do not matter), or must exist at all
times to prevent other sorts of trouble.


File: make.info,  Node: Recursion,  Next: Sequences,  Prev: Interrupts,  Up: Commands

Recursive Use of `make'
=======================

   Recursive use of `make' means using `make' as a command in a
makefile.  This technique is useful when you want separate makefiles for
various subsystems that compose a larger system.  For example, suppose
you have a subdirectory `subdir' which has its own makefile, and you
would like the containing directory's makefile to run `make' on the
subdirectory.  You can do it by writing this:

     subsystem:
             cd subdir && $(MAKE)

or, equivalently, this (*note Summary of Options: Options Summary.):

     subsystem:
             $(MAKE) -C subdir

   You can write recursive `make' commands just by copying this example,
but there are many things to know about how they work and why, and about
how the sub-`make' relates to the top-level `make'.  You may also find
it useful to declare targets that invoke recursive `make' commands as
`.PHONY' (for more discussion on when this is useful, see *Note Phony
Targets::).

   For your convenience, GNU `make' sets the variable `CURDIR' to the
pathname of the current working directory for you.  If `-C' is in
effect, it will contain the path of the new directory, not the
original.  The value has the same precedence it would have if it were
set in the makefile (by default, an environment variable `CURDIR' will
not override this value).  Note that setting this variable has no
effect on the operation of `make'

* Menu:

* MAKE Variable::               The special effects of using `$(MAKE)'.
* Variables/Recursion::         How to communicate variables to a sub-`make'.
* Options/Recursion::           How to communicate options to a sub-`make'.
* -w Option::                   How the `-w' or `--print-directory' option
                                  helps debug use of recursive `make' commands.


File: make.info,  Node: MAKE Variable,  Next: Variables/Recursion,  Prev: Recursion,  Up: Recursion

How the `MAKE' Variable Works
-----------------------------

   Recursive `make' commands should always use the variable `MAKE', not
the explicit command name `make', as shown here:

     subsystem:
             cd subdir && $(MAKE)

   The value of this variable is the file name with which `make' was
invoked.  If this file name was `/bin/make', then the command executed
is `cd subdir && /bin/make'.  If you use a special version of `make' to
run the top-level makefile, the same special version will be executed
for recursive invocations.

   As a special feature, using the variable `MAKE' in the commands of a
rule alters the effects of the `-t' (`--touch'), `-n' (`--just-print'),
or `-q' (`--question') option.  Using the `MAKE' variable has the same
effect as using a `+' character at the beginning of the command line.
*Note Instead of Executing the Commands: Instead of Execution.

   Consider the command `make -t' in the above example.  (The `-t'
option marks targets as up to date without actually running any
commands; see *Note Instead of Execution::.)  Following the usual
definition of `-t', a `make -t' command in the example would create a
file named `subsystem' and do nothing else.  What you really want it to
do is run `cd subdir && make -t'; but that would require executing the
command, and `-t' says not to execute commands.

   The special feature makes this do what you want: whenever a command
line of a rule contains the variable `MAKE', the flags `-t', `-n' and
`-q' do not apply to that line.  Command lines containing `MAKE' are
executed normally despite the presence of a flag that causes most
commands not to be run.  The usual `MAKEFLAGS' mechanism passes the
flags to the sub-`make' (*note Communicating Options to a Sub-`make':
Options/Recursion.), so your request to touch the files, or print the
commands, is propagated to the subsystem.


File: make.info,  Node: Variables/Recursion,  Next: Options/Recursion,  Prev: MAKE Variable,  Up: Recursion

Communicating Variables to a Sub-`make'
---------------------------------------

   Variable values of the top-level `make' can be passed to the
sub-`make' through the environment by explicit request.  These
variables are defined in the sub-`make' as defaults, but do not
override what is specified in the makefile used by the sub-`make'
makefile unless you use the `-e' switch (*note Summary of Options:
Options Summary.).

   To pass down, or "export", a variable, `make' adds the variable and
its value to the environment for running each command.  The sub-`make',
in turn, uses the environment to initialize its table of variable
values.  *Note Variables from the Environment: Environment.

   Except by explicit request, `make' exports a variable only if it is
either defined in the environment initially or set on the command line,
and if its name consists only of letters, numbers, and underscores.
Some shells cannot cope with environment variable names consisting of
characters other than letters, numbers, and underscores.

   The special variables `SHELL' and `MAKEFLAGS' are always exported
(unless you unexport them).  `MAKEFILES' is exported if you set it to
anything.

   `make' automatically passes down variable values that were defined
on the command line, by putting them in the `MAKEFLAGS' variable.
*Note Options/Recursion::.

   Variables are _not_ normally passed down if they were created by
default by `make' (*note Variables Used by Implicit Rules: Implicit
Variables.).  The sub-`make' will define these for itself.

   If you want to export specific variables to a sub-`make', use the
`export' directive, like this:

     export VARIABLE ...

If you want to _prevent_ a variable from being exported, use the
`unexport' directive, like this:

     unexport VARIABLE ...

In both of these forms, the arguments to `export' and `unexport' are
expanded, and so could be variables or functions which expand to a
(list of) variable names to be (un)exported.

   As a convenience, you can define a variable and export it at the same
time by doing:

     export VARIABLE = value

has the same result as:

     VARIABLE = value
     export VARIABLE

and

     export VARIABLE := value

has the same result as:

     VARIABLE := value
     export VARIABLE

   Likewise,

     export VARIABLE += value

is just like:

     VARIABLE += value
     export VARIABLE

*Note Appending More Text to Variables: Appending.

   You may notice that the `export' and `unexport' directives work in
`make' in the same way they work in the shell, `sh'.

   If you want all variables to be exported by default, you can use
`export' by itself:

     export

This tells `make' that variables which are not explicitly mentioned in
an `export' or `unexport' directive should be exported.  Any variable
given in an `unexport' directive will still _not_ be exported.  If you
use `export' by itself to export variables by default, variables whose
names contain characters other than alphanumerics and underscores will
not be exported unless specifically mentioned in an `export' directive.

   The behavior elicited by an `export' directive by itself was the
default in older versions of GNU `make'.  If your makefiles depend on
this behavior and you want to be compatible with old versions of
`make', you can write a rule for the special target
`.EXPORT_ALL_VARIABLES' instead of using the `export' directive.  This
will be ignored by old `make's, while the `export' directive will cause
a syntax error.

   Likewise, you can use `unexport' by itself to tell `make' _not_ to
export variables by default.  Since this is the default behavior, you
would only need to do this if `export' had been used by itself earlier
(in an included makefile, perhaps).  You *cannot* use `export' and
`unexport' by themselves to have variables exported for some commands
and not for others.  The last `export' or `unexport' directive that
appears by itself determines the behavior for the entire run of `make'.

   As a special feature, the variable `MAKELEVEL' is changed when it is
passed down from level to level.  This variable's value is a string
which is the depth of the level as a decimal number.  The value is `0'
for the top-level `make'; `1' for a sub-`make', `2' for a
sub-sub-`make', and so on.  The incrementation happens when `make' sets
up the environment for a command.

   The main use of `MAKELEVEL' is to test it in a conditional directive
(*note Conditional Parts of Makefiles: Conditionals.); this way you can
write a makefile that behaves one way if run recursively and another
way if run directly by you.

   You can use the variable `MAKEFILES' to cause all sub-`make'
commands to use additional makefiles.  The value of `MAKEFILES' is a
whitespace-separated list of file names.  This variable, if defined in
the outer-level makefile, is passed down through the environment; then
it serves as a list of extra makefiles for the sub-`make' to read
before the usual or specified ones.  *Note The Variable `MAKEFILES':
MAKEFILES Variable.


File: make.info,  Node: Options/Recursion,  Next: -w Option,  Prev: Variables/Recursion,  Up: Recursion

Communicating Options to a Sub-`make'
-------------------------------------

   Flags such as `-s' and `-k' are passed automatically to the
sub-`make' through the variable `MAKEFLAGS'.  This variable is set up
automatically by `make' to contain the flag letters that `make'
received.  Thus, if you do `make -ks' then `MAKEFLAGS' gets the value
`ks'.

   As a consequence, every sub-`make' gets a value for `MAKEFLAGS' in
its environment.  In response, it takes the flags from that value and
processes them as if they had been given as arguments.  *Note Summary
of Options: Options Summary.

   Likewise variables defined on the command line are passed to the
sub-`make' through `MAKEFLAGS'.  Words in the value of `MAKEFLAGS' that
contain `=', `make' treats as variable definitions just as if they
appeared on the command line.  *Note Overriding Variables: Overriding.

   The options `-C', `-f', `-o', and `-W' are not put into `MAKEFLAGS';
these options are not passed down.

   The `-j' option is a special case (*note Parallel Execution:
Parallel.).  If you set it to some numeric value `N' and your operating
system supports it (most any UNIX system will; others typically won't),
the parent `make' and all the sub-`make's will communicate to ensure
that there are only `N' jobs running at the same time between them all.
Note that any job that is marked recursive (*note Instead of Executing
the Commands: Instead of Execution.)  doesn't count against the total
jobs (otherwise we could get `N' sub-`make's running and have no slots
left over for any real work!)

   If your operating system doesn't support the above communication,
then `-j 1' is always put into `MAKEFLAGS' instead of the value you
specified.  This is because if the `-j' option were passed down to
sub-`make's, you would get many more jobs running in parallel than you
asked for.  If you give `-j' with no numeric argument, meaning to run
as many jobs as possible in parallel, this is passed down, since
multiple infinities are no more than one.

   If you do not want to pass the other flags down, you must change the
value of `MAKEFLAGS', like this:

     subsystem:
             cd subdir && $(MAKE) MAKEFLAGS=

   The command line variable definitions really appear in the variable
`MAKEOVERRIDES', and `MAKEFLAGS' contains a reference to this variable.
If you do want to pass flags down normally, but don't want to pass
down the command line variable definitions, you can reset
`MAKEOVERRIDES' to empty, like this:

     MAKEOVERRIDES =

This is not usually useful to do.  However, some systems have a small
fixed limit on the size of the environment, and putting so much
information into the value of `MAKEFLAGS' can exceed it.  If you see
the error message `Arg list too long', this may be the problem.  (For
strict compliance with POSIX.2, changing `MAKEOVERRIDES' does not
affect `MAKEFLAGS' if the special target `.POSIX' appears in the
makefile.  You probably do not care about this.)

   A similar variable `MFLAGS' exists also, for historical
compatibility.  It has the same value as `MAKEFLAGS' except that it
does not contain the command line variable definitions, and it always
begins with a hyphen unless it is empty (`MAKEFLAGS' begins with a
hyphen only when it begins with an option that has no single-letter
version, such as `--warn-undefined-variables').  `MFLAGS' was
traditionally used explicitly in the recursive `make' command, like
this:

     subsystem:
             cd subdir && $(MAKE) $(MFLAGS)

but now `MAKEFLAGS' makes this usage redundant.  If you want your
makefiles to be compatible with old `make' programs, use this
technique; it will work fine with more modern `make' versions too.

   The `MAKEFLAGS' variable can also be useful if you want to have
certain options, such as `-k' (*note Summary of Options: Options
Summary.), set each time you run `make'.  You simply put a value for
`MAKEFLAGS' in your environment.  You can also set `MAKEFLAGS' in a
makefile, to specify additional flags that should also be in effect for
that makefile.  (Note that you cannot use `MFLAGS' this way.  That
variable is set only for compatibility; `make' does not interpret a
value you set for it in any way.)

   When `make' interprets the value of `MAKEFLAGS' (either from the
environment or from a makefile), it first prepends a hyphen if the value
does not already begin with one.  Then it chops the value into words
separated by blanks, and parses these words as if they were options
given on the command line (except that `-C', `-f', `-h', `-o', `-W',
and their long-named versions are ignored; and there is no error for an
invalid option).

   If you do put `MAKEFLAGS' in your environment, you should be sure not
to include any options that will drastically affect the actions of
`make' and undermine the purpose of makefiles and of `make' itself.
For instance, the `-t', `-n', and `-q' options, if put in one of these
variables, could have disastrous consequences and would certainly have
at least surprising and probably annoying effects.


File: make.info,  Node: -w Option,  Prev: Options/Recursion,  Up: Recursion

The `--print-directory' Option
------------------------------

   If you use several levels of recursive `make' invocations, the `-w'
or `--print-directory' option can make the output a lot easier to
understand by showing each directory as `make' starts processing it and
as `make' finishes processing it.  For example, if `make -w' is run in
the directory `/u/gnu/make', `make' will print a line of the form:

     make: Entering directory `/u/gnu/make'.

before doing anything else, and a line of the form:

     make: Leaving directory `/u/gnu/make'.

when processing is completed.

   Normally, you do not need to specify this option because `make' does
it for you: `-w' is turned on automatically when you use the `-C'
option, and in sub-`make's.  `make' will not automatically turn on `-w'
if you also use `-s', which says to be silent, or if you use
`--no-print-directory' to explicitly disable it.

