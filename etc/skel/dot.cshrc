#	$NetBSD: dot.cshrc,v 1.1.6.1 2003/12/24 04:27:11 jmc Exp $
#csh .cshrc file

alias h		history 25
alias j		jobs -l
alias la	ls -a
alias lf	ls -FA
alias ll	ls -lA
alias su	su -m

setenv	EDITOR	vi
setenv	VISUAL	${EDITOR}
setenv	EXINIT	'set autoindent'
setenv	PAGER	more

set path = (~/bin /bin /sbin /usr/{bin,sbin,X11R6/bin,pkg/{,s}bin,games} \
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
