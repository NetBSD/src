# $Id: test.awk,v 1.1.2.2 2007/11/06 23:12:26 matt Exp $

END {
	print toupper($0);
}
