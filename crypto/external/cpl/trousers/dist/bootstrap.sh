set -x
aclocal || exit 1
libtoolize --force -c || exit 1
automake --add-missing -c --foreign || exit 1
autoconf || exit 1
