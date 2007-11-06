# $NetBSD: domains.sed,v 1.2.22.1 2007/11/06 23:13:20 matt Exp $
s/<[^>]*>//g
/&nbsp;&nbsp/ {
	s/&nbsp;/ /g
	s/&#[0-9]*;/ /g
	s/  */ /g
	s/^ *\.//
	s/$//
	p
}
