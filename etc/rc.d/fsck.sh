#!/bin/sh
#
# $NetBSD: fsck.sh,v 1.1.1.1 2000/03/10 11:53:25 lukem Exp $
#

# PROVIDE: fsck
# REQUIRE: localswap ccd raidframe

fsck_start()
{
	if [ -e /fastboot ]; then
		echo "Fast boot: skipping disk checks."
	elif [ "$autoboot" = yes ]; then
					# During fsck ignore SIGQUIT
		trap : 3

		echo "Automatic boot in progress: starting file system checks."
		fsck -p
		case $? in
		0)
			;;
		2)
			exit 1
			;;
		4)
			echo "Rebooting..."
			reboot
			echo "Reboot failed; help!"
			exit 1
			;;
		8)
			echo "Automatic file system check failed; help!"
			exit 1
			;;
		12)
			echo "Boot interrupted."
			exit 1
			;;
		130)
			exit 1
			;;
		*)
			echo "Unknown error; help!"
			exit 1
			;;
		esac

					# Reset SIGQUIT handler.
		trap "echo 'Boot interrupted.'; exit 1" 3
	fi
}

case "$1" in
*start)
	fsck_start
	;;
esac
