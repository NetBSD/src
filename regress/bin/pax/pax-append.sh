#!/bin/sh

cleanup() {
	rm -f foo bar file1.tar file2.tar
}
trap cleanup 0 1 2 3 15

cleanup
touch foo bar

# store both foo and bar into file1.tar
pax -w -b 512 -x ustar -f file1.tar foo bar

# store foo into file2.tar, then append bar to file2.tar
pax -w -b 512 -x ustar -f file2.tar foo
pax -w -b 512 -x ustar -f file2.tar -a bar

# note that file1.tar and file2.tar are different
cmp file1.tar file2.tar
