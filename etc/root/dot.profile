#	$NetBSD: dot.profile,v 1.30 2018/03/01 06:12:09 snj Exp $

export PATH=/sbin:/usr/sbin:/bin:/usr/bin:/usr/pkg/sbin:/usr/pkg/bin
export PATH=${PATH}:/usr/X11R7/bin:/usr/local/sbin:/usr/local/bin

# Uncomment the following line(s) to install binary packages
# from cdn.NetBSD.org via pkg_add.  (See also pkg_install.conf)
#export PKG_PATH="http://cdn.NetBSD.org/pub/pkgsrc/packages/NetBSD/$(uname -p)/$(uname -r|cut -f '1 2' -d.|cut -f 1 -d_)/All"

export BLOCKSIZE=1k

export HOST="$(hostname)"

umask 022
#ulimit -c 0

export ENV=/root/.shrc

# Do not display in 'su -' case
if [ -z "$SU_FROM" ]; then
        echo "We recommend that you create a non-root account and use su(1) for root access."
fi
