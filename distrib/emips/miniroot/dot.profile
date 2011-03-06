#	$NetBSD: dot.profile,v 1.1.2.2 2011/03/06 15:07:37 bouyer Exp $
PATH=/sbin:/bin:/usr/bin:/usr/sbin:/
export PATH

	# get the terminal type
	_loop=""
	while [ "X${_loop}" = X"" ]; do
		echo "" >& 2
		echo "Setting terminal type. Options:" >& 2
		echo "  ansi-nt	for Windows console window" >& 2
		echo "  vt100	for dumb serial terminal" >& 2
		echo "  xterm   for xterm." >& 2
		echo "" >& 2
		eval `tset -s -m ":?$TERM"`
		if [ "X${TERM}" != X"unknown" ]; then
			_loop="done"
		fi
	done

	sysinst
