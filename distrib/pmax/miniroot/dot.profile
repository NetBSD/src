#	$NetBSD: dot.profile,v 1.1.2.2 1998/11/24 21:50:46 cgd Exp $
PATH=/sbin:/usr/sbin:/usr.install/sbin:/bin:/usr/bin:/usr.install/bin:/
export PATH

BLOCKSIZE=1k
export BLOCKSIZE

HOME=/root
export HOME

echo "For a graphics console, use 'rcons' as your terminal type."
echo "For a serial console via an X terminal, use 'xterm'."
echo "Otherwise use 'vt100'."
echo ""
if [ -x /usr/bin/tset ]; then
	eval `/usr/bin/tset -sQ \?$TERM`
fi

umask 022

echo ""
echo "Mount the root filesystem read-write by typing:"
echo "	mount -u /dev/rz<SCSI-ID><PARTITION> /"
echo "where SCSI-ID is the SCSI id of the disk that contains the root"
echo "filesystem, and <PARTITION> is the partition that contains the"
echo "root filesystem (usually 'a' for the start of a disk or 'b' for"
echo "the swap partition."

echo ""
echo "To start sysinst, simply type:"
echo "	sysinst"
echo "Your terminal type must be set and the root filesystem mounted"
echo "read-write before starting sysinst."

echo ""
