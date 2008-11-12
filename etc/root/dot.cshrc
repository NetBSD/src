#	$NetBSD: dot.cshrc,v 1.16.26.1 2008/11/12 23:33:45 snj Exp $

alias	h	history
alias	j	jobs -l
alias	hup	'( set pid=$< ; kill -HUP $pid ) < /var/run/\!$.pid'
alias	la	ls -a
alias	lf	ls -FA
alias	ll	ls -l

alias	x	exit
alias	z	suspend

alias	back	'set back="$old"; set old="$cwd"; cd "$back"; unset back; dirs'
alias	cd	'set old="$cwd"; chdir \!*'
alias	pd	pushd
alias	pd2	pushd +2
alias	pd3	pushd +3
alias	pd4	pushd +4
alias	tset	'set noglob histchars=""; eval `\tset -s \!*`; unset noglob histchars'

# Uncomment the following line to install binary packages
# from ftp.NetBSD.org via pkg_add.
#setenv PKG_PATH ftp://ftp.netbsd.org/pkgsrc/packages/NetBSD/`uname -m`/5.0/All

setenv BLOCKSIZE 1k

set history=1000
set path=(/sbin /usr/sbin /bin /usr/bin /usr/pkg/sbin /usr/pkg/bin /usr/X11R6/bin /usr/local/sbin /usr/local/bin)

# directory stuff: cdpath/cd/back
set cdpath=(/usr/src/{sys,bin,sbin,usr.{bin,sbin},lib,libexec,share,local,games,gnu/{usr.{bin,sbin},libexec,lib,games}})

if ($?prompt && -x /usr/bin/id ) then
	if (`/usr/bin/id -u` == 0) then
		set prompt="`hostname -s`# "
	else
		set prompt="`hostname -s`% "
	endif
endif

umask 022
