# $NetBSD: domains.sed,v 1.2 2003/03/02 21:32:33 jhawk Exp $
:top
#				Strip ^Ms
s///g			
# 				Join all lines with unterminated HTML tags
/<[^>]*$/{
	N
	b top
}
#				Replace all <BR> with EOL marker ($)
s/<BR>/$/g			
# 				Join all data lines (containing ">.") not ending in $
/>\..*[^$]$/{
	N
	s/\n//g
	b top
}
s/<[^>]*>//g
#				Remove all HTML tags
s/\$$//	
#				Remove EOL markers
s/&nbsp;/ /g
#				Remove HTML character encodings
s/&#150;//g
s/[ 	][ 	]*/ /g
#	n			Compress spaces/tabs
s/^ //
#				Output metadata to file "top"
/updated/{
  s/.*updated/# Latest change:/
  s/ *$//
  w top
}
#				Delete all non-data lines
/^\./!d
#				Remove leading '.'
s/^\.//
