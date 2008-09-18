#! /bin/sh
#
# lvm2		This script handles LVM2 initialization/shutdown.
#
#		Written by Andres Salomon <dilinger@mp3revolution.net>.
#

PATH=/sbin:/bin:/usr/sbin:/usr/bin
NAME=lvm2
DESC=LVM

test -x /sbin/vgchange || exit 0
modprobe dm-mod >/dev/null 2>&1

# Create necessary files in /dev for device-mapper
create_devfiles() {
	DIR="/dev/mapper"
	FILE="$DIR/control"
	major=$(grep "[0-9] misc$" /proc/devices | sed 's/[ ]\+misc//')
	minor=$(grep "[0-9] device-mapper$" /proc/misc | sed 's/[ ]\+device-mapper//')

	if test ! -d $DIR; then
		mkdir --mode=755 $DIR >/dev/null 2>&1
	fi

	if test ! -c $FILE -a ! -z "$minor"; then
		mknod --mode=600 $FILE c $major $minor >/dev/null 2>&1
	fi
}

case "$1" in
  start)
	echo -n "Initializing $DESC: "
	create_devfiles
	vgchange -a y

#	# Mount all LVM devices
#	for vg in $( vgchange -a y 2>/dev/null | grep active | awk -F\" '{print $2}' ); do
#		MTPT=$( grep $vg /etc/fstab | awk '{print $2}' )
#		mount $MTPT
#	done
	echo "$NAME."
	;;
  stop)
	echo -n "Shutting down $DESC: "
	# We don't really try all that hard to shut it down; far too many
	# things that can keep it from successfully shutting down.
	vgchange -a n
	echo "$NAME."
	;;
  restart|force-reload)
	echo -n "Restarting $DESC: "
	vgchange -a n
	sleep 1
	vgchange -a y
	echo "$NAME."
	;;
  *)
	echo "Usage: /etc/init.d/$NAME {start|stop|restart|force-reload}" >&2
	exit 1
	;;
esac

exit 0
