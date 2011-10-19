#	$NetBSD: dot.cshrc,v 1.5 2011/10/19 10:14:35 tsutsui Exp $
#
# This is the default .cshrc file.
# Users are expected to edit it to meet their own needs.
#
# The commands in this file are executed each time a new csh shell
# is started.
#
# See csh(1) for details.
#

#setenv	EDITOR	vi
#setenv	VISUAL	${EDITOR}
#setenv	PAGER	more

set path = (~/bin /bin /sbin /usr/{bin,sbin,X11R7/bin,X11R6/bin,pkg/{,s}bin,games} \
	    /usr/local/{,s}bin)

if ($?prompt) then
	# An interactive shell -- set some stuff up
	set filec
	set history = 1000
	set ignoreeof
	set mail = (/var/mail/$USER)
	set mch = `hostname -s`
	set prompt = "${mch:q}: {\!} "
endif
