XCOMM! /bin/sh
XCOMM $Xorg: server.cpp,v 1.3 2000/08/17 19:54:01 cpqbld Exp $
XCOMM

XCOMM Copyright (c) 1993 Quarterdeck Office Systems
XCOMM
XCOMM Permission to use, copy, modify, distribute, and sell this software
XCOMM and software and its documentation for any purpose is hereby granted
XCOMM without fee, provided that the above copyright notice appear in all
XCOMM copies and that both that copyright notice and this permission
XCOMM notice appear in supporting documentation, and that the name
XCOMM Quarterdeck Office Systems, Inc. not be used in advertising or
XCOMM publicity pertaining to distribution of this software without
XCOMM specific, written prior permission.
XCOMM
XCOMM THIS SOFTWARE IS PROVIDED "AS-IS".  QUARTERDECK OFFICE SYSTEMS,
XCOMM INC., DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
XCOMM INCLUDING WITHOUT LIMITATION ALL IMPLIED WARRANTIES OF
XCOMM MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, OR
XCOMM NONINFRINGEMENT.  IN NO EVENT SHALL QUARTERDECK OFFICE SYSTEMS,
XCOMM INC., BE LIABLE FOR ANY DAMAGES WHATSOEVER, INCLUDING SPECIAL,
XCOMM INCIDENTAL OR CONSEQUENTIAL DAMAGES, INCLUDING LOSS OF USE, DATA, OR
XCOMM PROFITS, EVEN IF ADVISED OF THE POSSIBILITY THEREOF, AND REGARDLESS
XCOMM OF WHETHER IN AN ACTION IN CONTRACT, TORT OR NEGLIGENCE, ARISING OUT
XCOMM OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
XCOMM
XCOMM $XFree86: xc/programs/rstart/server.cpp,v 3.2 2001/01/17 23:45:03 dawes Exp $

exec BINDIR/SERVERNAME.real -c LIBDIR/config
