# $NetBSD: inst.profile,v 1.1 2013/03/19 22:16:53 garbled Exp $

PATH=/sbin:/bin:/usr/bin:/usr/sbin:/
export PATH
TERM=vt100
export TERM
HOME=/
export HOME

umask 022

if [ "X${DONEPROFILE}" = "X" ]; then
	DONEPROFILE=YES
	export DONEPROFILE

	# set up some sane defaults
	echo 'erase ^H, werase ^W, kill ^U, intr ^C'
	stty newcrt werase ^W intr ^C kill ^U erase ^H
	echo ''

	# run the installation or upgrade script.
	echo "starting sysinst!"
	cd /
	/sysinst
	echo "If you have completed your install, please reboot"
	echo "into your new OS, otherwise, run sysinst again."
fi
