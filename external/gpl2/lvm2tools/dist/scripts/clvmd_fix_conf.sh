#!/bin/sh
#
# Edit an lvm.conf file to enable cluster locking.
#
# $1 is the directory where the locking library is installed.
# $2 (optional) is the config file
# $3 (optional) is the locking library name
#
#
PREFIX=$1
LVMCONF=$2
LIB=$3

if [ -z "$PREFIX" ]
then
  echo "usage: $0 <prefix> [<config file>] [<library>]"
  echo ""
  echo "<prefix>|UNDO  location of the cluster locking shared library. (no default)"
  echo "               UNDO will reset the locking back to local"
  echo "<config file>  name of the LVM config file (default: /etc/lvm/lvm.conf)"
  echo "<library>      name of the shared library (default: liblvm2clusterlock.so)"
  echo ""
  exit 0
fi

[ -z "$LVMCONF" ] && LVMCONF="/etc/lvm/lvm.conf"
[ -z "$LIB" ] && LIB="liblvm2clusterlock.so"

if [ "$PREFIX" = "UNDO" ]
then
  locking_type="1"
else
  locking_type="2"

  if [ "${PREFIX:0:1}" != "/" ]
  then
    echo "Prefix must be an absolute path name (starting with a /)"
    exit 12
  fi

  if [ ! -f "$PREFIX/$LIB" ]
  then
    echo "$PREFIX/$LIB does not exist, did you do a \"make install\" ?"
    exit 11
  fi
fi

if [ ! -f "$LVMCONF" ]
then
  echo "$LVMCONF does not exist"
  exit 10
fi


SCRIPTFILE=`mktemp -t lvmscript.XXXXXXXXXX`
TMPFILE=`mktemp -t lvmtmp.XXXXXXXXXX`


# Flags so we know which parts of the file we can replace and which need
# adding. These are return codes from grep, so zero means it IS present!
have_type=1
have_dir=1
have_library=1
have_global=1

grep -q '^[[:blank:]]*locking_type[[:blank:]]*=' $LVMCONF
have_type=$?

grep -q '^[[:blank:]]*library_dir[[:blank:]]*=' $LVMCONF
have_dir=$?

grep -q '^[[:blank:]]*locking_library[[:blank:]]*=' $LVMCONF
have_library=$?

# Those options are in section "global {" so we must have one if any are present.
if [ "$have_type" = "0" -o "$have_dir" = "0" -o "$have_library" = "0" ]
then

    # See if we can find it...
    grep -q '^[[:blank:]]*global[[:blank:]]*{' $LVMCONF
    have_global=$?
    
    if [ "$have_global" = "1" ] 
	then
	echo "global keys but no 'global {' found, can't edit file"
	exit 12
    fi
fi

# So if we don't have "global {" we need to create one and 
# populate it

if [ "$have_global" = "1" ]
then
    cat $LVMCONF - <<EOF > $TMPFILE
global {
    # Enable locking for cluster LVM
    locking_type = $locking_type
    library_dir = "$PREFIX"
    locking_library = "$LIB"
}
EOF
    if [ $? != 0 ]
    then
	echo "failed to create temporary config file, $LVMCONF not updated"
	exit 1
    fi
else
    #
    # We have a "global {" section, so add or replace the
    # locking entries as appropriate
    #

    if [ "$have_type" = "0" ] 
    then
	SEDCMD=" s/^[[:blank:]]*locking_type[[:blank:]]*=.*/\ \ \ \ locking_type = $locking_type/g"
    else
	SEDCMD=" /global[[:blank:]]*{/a\ \ \ \ locking_type = 2"
    fi
    
    if [ "$have_dir" = "0" ] 
    then
	SEDCMD="${SEDCMD}\ns'^[[:blank:]]*library_dir[[:blank:]]*=.*'\ \ \ \ library_dir = \"$PREFIX\"'g"
    else
	SEDCMD="${SEDCMD}\n/global[[:blank:]]*{/a\ \ \ \ library_dir = \"$PREFIX\""
    fi

    if [ "$have_library" = "0" ] 
    then
	SEDCMD="${SEDCMD}\ns/^[[:blank:]]*locking_library[[:blank:]]*=.*/\ \ \ \ locking_library = \"$LIB\"/g"
    else
	SEDCMD="${SEDCMD}\n/global[[:blank:]]*{/a\ \ \ \ locking_library = \"$LIB\""
    fi

    echo -e $SEDCMD > $SCRIPTFILE
    sed  <$LVMCONF >$TMPFILE -f $SCRIPTFILE
    if [ $? != 0 ]
    then
	echo "sed failed, $LVMCONF not updated"
	exit 1
    fi
fi

# Now we have a suitably editted config file in a temp place,
# backup the original and copy our new one into place.

cp $LVMCONF $LVMCONF.nocluster
if [ $? != 0 ]
    then
    echo "failed to backup old config file, $LVMCONF not updated"
    exit 2
fi

cp $TMPFILE $LVMCONF
if [ $? != 0 ]
    then
    echo "failed to copy new config file into place, check $LVMCONF is still OK"
    exit 3
fi

rm -f $SCRIPTFILE $TMPFILE

#!/bin/sh
#
# Edit an lvm.conf file to enable cluster locking.
#
# $1 is the directory where the locking library is installed.
# $2 (optional) is the config file
# $3 (optional) is the locking library name
#
#
PREFIX=$1
LVMCONF=$2
LIB=$3

if [ -z "$PREFIX" ]
then
  echo "usage: $0 <prefix> [<config file>] [<library>]"
  echo ""
  echo "<prefix>|UNDO  location of the cluster locking shared library. (no default)"
  echo "               UNDO will reset the locking back to local"
  echo "<config file>  name of the LVM config file (default: /etc/lvm/lvm.conf)"
  echo "<library>      name of the shared library (default: liblvm2clusterlock.so)"
  echo ""
  exit 0
fi

[ -z "$LVMCONF" ] && LVMCONF="/etc/lvm/lvm.conf"
[ -z "$LIB" ] && LIB="liblvm2clusterlock.so"

if [ "$PREFIX" = "UNDO" ]
then
  locking_type="1"
else
  locking_type="2"

  if [ "${PREFIX:0:1}" != "/" ]
  then
    echo "Prefix must be an absolute path name (starting with a /)"
    exit 12
  fi

  if [ ! -f "$PREFIX/$LIB" ]
  then
    echo "$PREFIX/$LIB does not exist, did you do a \"make install\" ?"
    exit 11
  fi
fi

if [ ! -f "$LVMCONF" ]
then
  echo "$LVMCONF does not exist"
  exit 10
fi


SCRIPTFILE=`mktemp -t lvmscript.XXXXXXXXXX`
TMPFILE=`mktemp -t lvmtmp.XXXXXXXXXX`


# Flags so we know which parts of the file we can replace and which need
# adding. These are return codes from grep, so zero means it IS present!
have_type=1
have_dir=1
have_library=1
have_global=1

grep -q '^[[:blank:]]*locking_type[[:blank:]]*=' $LVMCONF
have_type=$?

grep -q '^[[:blank:]]*library_dir[[:blank:]]*=' $LVMCONF
have_dir=$?

grep -q '^[[:blank:]]*locking_library[[:blank:]]*=' $LVMCONF
have_library=$?

# Those options are in section "global {" so we must have one if any are present.
if [ "$have_type" = "0" -o "$have_dir" = "0" -o "$have_library" = "0" ]
then

    # See if we can find it...
    grep -q '^[[:blank:]]*global[[:blank:]]*{' $LVMCONF
    have_global=$?
    
    if [ "$have_global" = "1" ] 
	then
	echo "global keys but no 'global {' found, can't edit file"
	exit 12
    fi
fi

# So if we don't have "global {" we need to create one and 
# populate it

if [ "$have_global" = "1" ]
then
    cat $LVMCONF - <<EOF > $TMPFILE
global {
    # Enable locking for cluster LVM
    locking_type = $locking_type
    library_dir = "$PREFIX"
    locking_library = "$LIB"
}
EOF
    if [ $? != 0 ]
    then
	echo "failed to create temporary config file, $LVMCONF not updated"
	exit 1
    fi
else
    #
    # We have a "global {" section, so add or replace the
    # locking entries as appropriate
    #

    if [ "$have_type" = "0" ] 
    then
	SEDCMD=" s/^[[:blank:]]*locking_type[[:blank:]]*=.*/\ \ \ \ locking_type = $locking_type/g"
    else
	SEDCMD=" /global[[:blank:]]*{/a\ \ \ \ locking_type = 2"
    fi
    
    if [ "$have_dir" = "0" ] 
    then
	SEDCMD="${SEDCMD}\ns'^[[:blank:]]*library_dir[[:blank:]]*=.*'\ \ \ \ library_dir = \"$PREFIX\"'g"
    else
	SEDCMD="${SEDCMD}\n/global[[:blank:]]*{/a\ \ \ \ library_dir = \"$PREFIX\""
    fi

    if [ "$have_library" = "0" ] 
    then
	SEDCMD="${SEDCMD}\ns/^[[:blank:]]*locking_library[[:blank:]]*=.*/\ \ \ \ locking_library = \"$LIB\"/g"
    else
	SEDCMD="${SEDCMD}\n/global[[:blank:]]*{/a\ \ \ \ locking_library = \"$LIB\""
    fi

    echo -e $SEDCMD > $SCRIPTFILE
    sed  <$LVMCONF >$TMPFILE -f $SCRIPTFILE
    if [ $? != 0 ]
    then
	echo "sed failed, $LVMCONF not updated"
	exit 1
    fi
fi

# Now we have a suitably editted config file in a temp place,
# backup the original and copy our new one into place.

cp $LVMCONF $LVMCONF.nocluster
if [ $? != 0 ]
    then
    echo "failed to backup old config file, $LVMCONF not updated"
    exit 2
fi

cp $TMPFILE $LVMCONF
if [ $? != 0 ]
    then
    echo "failed to copy new config file into place, check $LVMCONF is still OK"
    exit 3
fi

rm -f $SCRIPTFILE $TMPFILE

