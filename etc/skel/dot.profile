#	$NetBSD: dot.profile,v 1.12 2024/09/23 12:12:36 uwe Exp $
#
# This is the default .profile file.
# Users are expected to edit it to meet their own needs.
#
# The commands in this file are executed when an sh user first
# logs in.
#
# See sh(1) for details.
#

# Set your editor.  Can be set to emacs or nano or whatever other
# editor you may prefer, but of course those editors must be installed
# before you can use them.  Default to explicitly setting vi, as
# otherwise some software (including sh's own fc) will run ed and
# other software will fail.
export EDITOR=vi

# vi settings: set show-match auto-indent always-redraw shift-width=4
#export EXINIT="se sm ai redraw sw=4"

# VISUAL sets the "visual" editor, i.e., vi rather than ed, which if
# set will be run by preference to $EDITOR by some software. It is
# mostly historical and usually does not need to be set.
#export VISUAL=${EDITOR}

# Set the pager. This is used by, among other things, man(1) for
# showing man pages. The default is "more". Another reasonable choice
# (included with the system by default) is "less".
#export PAGER=less

# Many modern terminal emulators don't provide a way to disable
# alternate screen switching (like xterm's titeInhibit).  less(1) has
# its own option (-X) for that that you can use to alleviate the pain,
# as PAGER is the most common scenario for hitting this.  Add other
# flags according to taste.
#export LESS='-i -M -X'

# Set your default printer, if desired.
#export PRINTER=change-this-to-a-printer

# Set the search path for programs.
PATH=$HOME/bin:/bin:/sbin:/usr/bin:/usr/sbin:/usr/X11R7/bin:/usr/pkg/bin
PATH=${PATH}:/usr/pkg/sbin:/usr/games:/usr/local/bin:/usr/local/sbin
export PATH

# Configure the shell to load .shrc at startup time.
# This will happen for every shell started, not just login shells.
export ENV=$HOME/.shrc
