## Process this file with automake to produce Makefile.in -*-Makefile-*-

aclocaldir = @aclocaldir@
aclocal_DATA = gettext.m4 lcmessage.m4 progtest.m4

# Generate this list with
# find . -type f -name '*.m4' -printf '%f\n'|sort |fmt |tr '\012' @ \
#   |sed 's/@$/%/;s/@/ \\@/g' |tr @% '\012\012'
EXTRA_DIST = README \
gettext.m4  lcmessage.m4  progtest.m4
