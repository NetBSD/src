#!/bin/sh -
# This script generates ed test scripts (.ed) from .t files

PATH="/bin:/usr/bin:/usr/local/bin/:."
[ X"$ED" = X ] && ED="../ed"

for i in *.t; do
#	base=${i%.*}
	base=`echo $i | sed 's/\..*//'`
	(
	echo "#!/bin/sh -" 
	echo "$ED - <<\EOT" 
	echo "r \\$base.d" 
	cat $i 
	echo "w \\$base.o" 
	echo EOT
	) >$base.ed
	chmod +x $base.ed
# The following is pretty ugly and not appropriate use of ed
# but the point is that it can be done...
#	base=`$ED - <<-EOF
#	r !echo "$i"
#	s/\..*
#	EOF`
#	$ED - <<-EOF
#	a
#	#!/bin/sh -
#	$ED - <<\EOT
#	r \\$base.d
#	w \\$base.o
#	EOT
#	.
#	-2r \\$i
#	w \\$base.ed
#	!chmod +x \\$base.ed
#	EOF
done

for i in *.err; do
#	base=${i%.*}
	base=`echo $i | sed 's/\..*//'`
	(
	echo "#!/bin/sh -" 
	echo "$ED - <<\EOT" 
	echo "r \\$base.err" 
	cat $i 
	echo "w \\$base.o" 
	echo EOT
	) >$base-err.ed
	chmod +x $base-err.ed
#	base=`$ED - <<-EOF
#	r !echo "$i"
#	s/\..*
#	EOF`
#	$ED - <<-EOF
#	a
#	#!/bin/sh -
#	$ED - <<\EOT
#	H
#	r \\$base.err
#	w \\$base.o
#	EOT
#	.
#	-2r \\$i
#	w \\${base}-err.ed
#	!chmod +x ${base}-err.ed
#	EOF
done
