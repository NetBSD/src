# $NetBSD: nwid.sed,v 1.1 2001/07/12 19:08:05 garbled Exp $
/"/b quote
s/.*nwid \([0-9a-zA-Z_]*\).*/\1/
/./b end
:quote
s/.*nwid "\([^"]*\)".*/\1/
:end
