Tue Dec 24 22:41:39 EST 1996

SCO's awk has a -e option which is similar to gawk's --source option,
allowing you to specify the script anywhere on the awk command line.

This can be a problem, since gawk will install itself as `awk' in
$(bindir). If this is ahead of /bin and /usr/bin in the search path,
several of SCO's scripts that use -e will break, since gawk does not
accept this option.

The solution is simple. After doing a `make install', do:

	rm -f /usr/local/bin/awk	# or wherever it is installed.

This removes the `awk' symlink so that SCO's programs will continue
to work.

If you complain to me about this, I will fuss at you for not having
done your homework.

Arnold Robbins
arnold@gnu.ai.mit.edu
