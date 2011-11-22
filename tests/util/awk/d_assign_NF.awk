# $NetBSD: d_assign_NF.awk,v 1.1 2011/11/22 20:22:10 cheusov Exp $

{
	NF = 2
	print "$0=`" $0 "`"
	print "$3=`" $3 "`"
	print "$4=`" $4 "`"
	NF = 3
	print "$0=`" $0 "`"
	print "$3=`" $3 "`"
	print "$4=`" $4 "`"
	NF = 4
	print "$0=`" $0 "`"
	print "$3=`" $3 "`"
	print "$4=`" $4 "`"
}
