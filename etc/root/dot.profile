#	$NetBSD: dot.profile,v 1.9.2.2 2000/02/22 22:27:30 he Exp $

PATH=/sbin:/usr/sbin:/bin:/usr/bin:/usr/pkg/sbin:/usr/pkg/bin
PATH=${PATH}:/usr/X11R6/bin:/usr/local/sbin:/usr/local/bin
export PATH

BLOCKSIZE=1k
export BLOCKSIZE

if [ -x /usr/bin/tset ]; then
	eval `/usr/bin/tset -sQrm 'unknown:?unknown'`
fi

umask 022

hup(){ kill -HUP `cat /var/run/$1.pid` ; }
ll(){ ls -l $* ; }
x(){ exit ; }

PS1="`hostname -s`$PS1"

# Do not display in 'su -' case
if [ -z "$SU_FROM" ]; then
	echo "We recommend creating a non-root account and using su(1) for access."
fi
