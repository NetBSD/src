#	$NetBSD: dot.profile,v 1.4 2000/09/13 19:54:02 hubertf Exp $

PATH=$HOME/bin:/bin:/sbin:/usr/bin:/usr/sbin:/usr/X11R6/bin:/usr/pkg/bin
PATH=${PATH}:/usr/pkg/sbin:/usr/games:/usr/local/bin:/usr/local/sbin
export PATH

EDITOR=vi
export EDITOR
EXINIT='set autoindent'
export EXINIT
PAGER=more
export PAGER

umask 2
