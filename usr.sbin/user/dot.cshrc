alias mail Mail
set ignoreeof filec
set history=1000
set path=(/usr/pkg/bin /usr/pkg/sbin /usr/pkg/pthreads/bin /usr/pkg/postgres/bin /sbin /usr/sbin /bin /usr/bin /usr/X11R6/bin /usr/local/scripts /usr/local/bin /usr/local/sbin)

# directory stuff: cdpath/cd/back
set cdpath=(/sys /usr/src/{bin,sbin,usr.{bin,sbin},pgrm,lib,libexec,share,contrib,local,devel,games,old,})

setenv BLOCKSIZE 1k

setenv CVS_RSH	ssh
setenv RSYNC_RSH	ssh
setenv XWINHOME /usr/X11R6
setenv XAUTHORITY $HOME/.Xauthority
setenv EDITOR /usr/bin/vi
setenv VISUAL /usr/bin/vi

setenv CLASSPATH .:/usr/pkg/share/kaffe/classes.zip
setenv KAFFEHOME /usr/pkg/share/kaffe
setenv LD_LIBRARY_PATH /usr/pkg/lib:/usr/lib:/usr/X11R6/lib:/usr/local/lib

setenv XNLSPATH /usr/local/netscape/nls

setenv TMPDIR	/tmp

# package stuff
setenv DISTDIR /usr/pkgsrc/distfiles

setenv MANPATH "/usr/pkg/man:/usr/share/man:/usr/local/man:/usr/X11R6/man"

set id = `id`
set user = `expr "//$id" : '[^(]*(\([^)]*\)'`

set HOST = `hostname -s`

alias setprompt	'set prompt = "$user@$HOST":"$cwd(\\!)% "'
alias cd	'cd \!* ; setprompt'
alias pushd	'pushd \!* ; setprompt'
alias popd	'popd \!* ; setprompt'
alias h		history
alias j		jobs -l
alias L		'ls -alL \!*'
alias l		'ls -al \!*'
alias li	'ls -ail \!*'

alias	tset	'set noglob histchars=""; eval `\tset -s \!*`; unset noglob histchars'

if ($?prompt) then
	setprompt
endif
