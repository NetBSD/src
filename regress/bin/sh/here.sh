#!/bin/sh

rval=0
exec >&2

nl='
'
OIFS="$IFS"

check()
{
	SVIFS="$IFS"
	result="$(eval $1)"
	# Remove newlines
	IFS="$nl"
	result="$(echo $result)"
	IFS="$OIFS"
	if [ "$2" != "$result" ]
	then
		echo "command: $1"
		echo "    expected [$2], found [$result]"
		rval=1
	fi
	IFS="$SVIFS"
}

y=x

IFS=
check 'x=`cat <<EOF'$nl'text'${nl}EOF$nl'`; echo $x' 'text'
check 'x=`cat <<\EOF'$nl'text'${nl}EOF$nl'`; echo $x' 'text'

check 'x=`cat <<EOF'$nl'te${y}t'${nl}EOF$nl'`; echo $x' 'text'
check 'x=`cat <<\EOF'$nl'te${y}t'${nl}EOF$nl'`; echo $x' 'te${y}t'
check 'x=`cat <<"EOF"'$nl'te${y}t'${nl}EOF$nl'`; echo $x' 'te${y}t'
check 'x=`cat <<'"'EOF'"$nl'te${y}t'${nl}EOF$nl'`; echo $x' 'te${y}t'

check 'x=`cat <<EOF'$nl'te'"'"'xt'${nl}EOF$nl'`; echo $x' 'te'"'"'xt'
check 'x=`cat <<\EOF'$nl'te'"'"'xt'${nl}EOF$nl'`; echo $x' 'te'"'"'xt'
check 'x=`cat <<"EOF"'$nl'te'"'"'xt'${nl}EOF$nl'`; echo $x' 'te'"'"'xt'

check 'x=`cat <<EOF'$nl'te'"'"'${y}t'${nl}EOF$nl'`; echo $x' 'te'"'"'xt'
check 'x=`cat <<EOF'$nl'te'"''"'${y}t'${nl}EOF$nl'`; echo $x' 'te'"''"'xt'

exit $rval
