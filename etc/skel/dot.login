#	$NetBSD: dot.login,v 1.1.6.1 2002/07/09 01:15:08 lukem Exp $
#csh .login file

if ( ! $?SHELL ) then
  setenv SHELL /bin/csh
endif

set noglob
eval `tset -s -m 'network:?xterm'`
unset noglob
stty status '^T' crt -tostop

if ( -x /usr/games/fortune ) /usr/games/fortune
