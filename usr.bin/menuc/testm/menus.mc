/*	$NetBSD: menus.mc,v 1.4 1998/06/25 09:58:58 phil Exp $	*/

/*
 * Copyright 1997 Piermont Information Systems Inc.
 * All rights reserved.
 *
 * Written by Philip A. Nelson for Piermont Information Systems Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software develooped for the NetBSD Project by
 *      Piermont Information Systems Inc.
 * 4. The name of Piermont Information Systems Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY PIERMONT INFORMATION SYSTEMS INC. ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL PIERMONT INFORMATION SYSTEMS INC. BE 
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF 
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

{

/* Initial code for definitions and includes. */

}

default x=20, y=10;

error action { fprintf (stderr, "Testm: Could not initialize curses.\n");
	       exit(1); };

menu root, title "  Main Menu of Test System";
	option  "Do nothing option", 
		action  { }
	;
	option  "Try a sub menu",
		sub menu  submenu
	;
	option  "A scrollable menu",
		sub menu  scrollit
	;
	option  "Run a shell...",
		action (endwin) { system ("/bin/sh"); }
	;
	exit action (endwin)  { printf ("Thanks for playing\n"); };
	help {
                    Main Menu Help Screen

This is help text for the main menu of the menu test system.  This
text should appear verbatim when asked for by use of the ? key by
the user.  This should allow scrolling, if needed.  If the first
character in the help is the newline (as the case for this help),
then that newline is not included in the help text.

Now this tests lines for scrolling:
10
11
12
13
14
15
16
17
18
19
20
21
22
23
24
25
26
27
28
29
30
31
32
33
34
35
36
37
38
39
40
41
42
43
44
45
46
47
48
49
50
51
52
53
54
55
56
57
58
59
60
61
62
63
64
65
66
67
68
69
70
71
72
73
74
75
76
77
78
79
80
};

menu submenu, title "  submenu test";
	option  "upper right", sub menu  upperright;
	option  "lower right", sub menu  lowerleft;
	option  "middle, no title", sub menu middle;

menu upperright, title "upper right", y=2, x=60, no exit;
	option  "Just Exit!", exit;

menu lowerleft, title "lower left", y=20, x=2, no exit;
	option  "Just Exit!", exit;

menu middle, no box;
	option "Just Exit!", exit;

menu scrollit, scrollable, h=4, title "  Scrollable Menu";
	option "option 1", action {};
	option "option 2", action {};
	option "option 3", action {};
	option "option 4", action {};
	option "option 5", action {};
	option "option 6", action {};
