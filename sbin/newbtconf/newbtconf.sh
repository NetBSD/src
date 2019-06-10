#!/bin/sh
#
# Setup a new config directory
#
if [ $# -lt 1 ] ; then
	echo "usage: $0 <newconfig> [<baseconfig>]"
	echo "usage: $0 init"
	echo "usage: $0 revert"
	exit 1;
fi
dir=$1

FILES="defaultdomain fstab ifconfig.* inetd.conf mrouted.conf \
	mygate myname netstart nsswitch.conf ntp.conf \
	rc.conf rc.conf.d resolv.conf"

if [ $dir = init ] ; then
	if [ -d /etc/etc.network ] || [ -e /etc/etc.current ] ; then
		echo "Error: multi-configuration already initialized"
		exit 1
	fi
	dir=etc.network
	cd /etc
	mkdir -m 755 $dir
	ln -s $dir etc.current
	ln -s $dir etc.default
	for i in ${FILES}; do
		if [ -f $i ] || [ -d $i ] ; then
			mv $i $dir
			ln -s etc.current/$i .
		fi
	done
	echo "/etc/$dir has now been created and populated."
	exit 0
fi

if [ $dir = revert ] ; then
	if [ !  -d /etc/etc.current ] ; then
		echo "Error: multi-configuration not initialized"
		exit 1
	fi
	cd /etc
	for i in ${FILES}; do
		if [ -f $i ] || [ -d $i ] ; then
			stat="`ls -ld $i`"
			case x"$stat" in
				xl*) :;;
				x*)
				echo "$i: not a symlink, skipping"
				continue ;;	
			esac
			linkto="${stat##*-> }"
			case x"$linkto" in
				xetc.current/*) :;;
				x*)
				echo "$i: does not symlink to etc.current, skipping"
				continue ;;
			esac
			if [ -f $i ] ; then
				rm $i
				cp -p $linkto $i
			else
				rm $i
				( cd etc.current && pax -rw -pe $i /etc )
			fi
		fi
	done
	rm etc.current
	rm etc.default
	exit 0
fi

if [ "`expr $dir : 'etc\.\(.*\)'`" != $dir ] ; then
	dir=etc.$dir
fi
if [ -e /etc/$dir ] ; then
	echo "Error: $dir already exists"
	exit 1;
fi
newname=`expr $dir : 'etc.\(.*\)'`
if [ $# -lt 2 ] ; then
	orig=etc.current
	echo "Using current config as base for $newname"
else
	orig=$2
fi

if [ -z "`expr $orig : 'etc.\(.*\)'`" ] ; then
	orig=etc.$orig
fi

if [ ! -d /etc/$orig ] ; then
	echo "Original directory /etc/$orig does not exist."
	exit 1;
fi
mkdir -m 755 /etc/$dir
cd /etc/$orig 
pax -rw -pe . /etc/$dir
echo "/etc/$dir has now been created and populated."
exit 0
