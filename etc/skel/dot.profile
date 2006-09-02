#	$NetBSD: dot.profile,v 1.3.12.1 2006/09/02 23:56:14 riz Exp $
#
# This is the default .profile file.
# Users are expected to edit it to meet their own needs.
#
# The commands in this file are executed when an sh user first
# logs in.
#
# See sh(1) for details.
#

PATH=$HOME/bin:/bin:/sbin:/usr/bin:/usr/sbin:/usr/X11R6/bin:/usr/pkg/bin
PATH=${PATH}:/usr/pkg/sbin:/usr/games:/usr/local/bin:/usr/local/sbin
export PATH

export EDITOR=vi
export EXINIT='set autoindent'
export PAGER=more

export ENV=$HOME/.shrc
