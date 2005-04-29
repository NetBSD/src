#!/bin/sh
# Creates Makefile.msvc.
domain=$1
catalogs=$2

cat <<\EOF
# -*- Makefile -*- for po subdirectory on VMS using the MMS utility

#### Start of system configuration section. ####

# Directories used by "make install":
prefix = SYS$DATA:[
datadir = $(prefix).share
localedir = $(datadir).locale

# Programs used by "make":
RM = delete

# Programs used by "make install":
INSTALL = copy
INSTALL_PROGRAM = copy
INSTALL_DATA = copy

#### End of system configuration section. ####

all :
	write sys$output "Nothing to be done for 'all'."

install : all
	create /directory $(prefix)]
	create /directory $(datadir)]
	create /directory $(localedir)]
EOF
for cat in $catalogs; do
  cat=`basename $cat`
  lang=`echo $cat | sed -e 's/\.gmo$//'`
cat <<EOF
	create /directory \$(localedir).${lang}]
	create /directory \$(localedir).${lang}.LC_MESSAGES]
	\$(INSTALL_DATA) ${lang}.gmo \$(localedir).${lang}.LC_MESSAGES]${domain}.mo
EOF
done
cat <<\EOF

installdirs :
	create /directory $(prefix)]
	create /directory $(datadir)]
	create /directory $(localedir)]
EOF
for cat in $catalogs; do
  cat=`basename $cat`
  lang=`echo $cat | sed -e 's/\.gmo$//'`
cat <<EOF
	create /directory \$(localedir).${lang}]
	create /directory \$(localedir).${lang}.LC_MESSAGES]
EOF
done
cat <<\EOF

uninstall :
EOF
for cat in $catalogs; do
  cat=`basename $cat`
  lang=`echo $cat | sed -e 's/\.gmo$//'`
cat <<EOF
	\$(RM) \$(localedir).${lang}.LC_MESSAGES]${domain}.mo;
EOF
done
cat <<\EOF

check : all
	write sys$output "Nothing else to be done for 'check'."

mostlyclean : clean
	write sys$output "Nothing else to be done for 'mostlyclean'."

clean :
	write sys$output "Nothing to be done for 'clean'."

distclean : clean
	write sys$output "Nothing else to be done for 'distclean'."

maintainer-clean : distclean
	write sys$output "Nothing else to be done for 'maintainer-clean'."
EOF
