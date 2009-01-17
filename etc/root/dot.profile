#	$NetBSD: dot.profile,v 1.18.26.1 2009/01/17 20:43:44 mjf Exp $

export PATH=/sbin:/usr/sbin:/bin:/usr/bin:/usr/pkg/sbin:/usr/pkg/bin
export PATH=${PATH}:/usr/X11R7/bin:/usr/X11R6/bin:/usr/local/sbin:/usr/local/bin

# Uncomment the following line to install binary packages
# from ftp.NetBSD.org via pkg_add.
#export PKG_PATH=ftp://ftp.netbsd.org/pkgsrc/packages/NetBSD/$(uname -m)/5.99/All

export BLOCKSIZE=1k

export HOST="$(hostname)"

if [ -x /usr/bin/tset ]; then
	eval `tset -sQrm 'unknown:?unknown'`
fi

umask 022
#ulimit -c 0

export ENV=/root/.shrc

# Do not display in 'su -' case
if [ -z "$SU_FROM" ]; then
        echo "We recommend creating a non-root account and using su(1) for root access."
fi
