%{
/*	$NetBSD: fgen.l,v 1.27.30.1 2009/05/13 19:19:49 jym Exp $	*/
/* FLEX input for FORTH input file scanner */
/*  
 * Copyright (c) 1998 Eduardo Horvath.
 * All rights reserved.
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
 *      This product includes software developed by Eduardo Horvath.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
	Specifications are as follows:

	The function "yylex()" always returns a pointer to a structure:

	    struct tok {
		int type;
		char *text;
	    }
	    #define TOKEN struct tok
*/
#include <sys/cdefs.h>
#ifdef HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#if defined(__RCSID) && !defined(lint)
__RCSID("$NetBSD: fgen.l,v 1.27.30.1 2009/05/13 19:19:49 jym Exp $");
#endif

%}

%option yylineno

decimal	[0-9.]
hex	[0-9A-Fa-f.]
octal	[0-7.]
white	[ \t\n\r\f]
tail	{white}

%{
#include <sys/types.h>
#include <arpa/inet.h>

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "fgen.h"
TOKEN ltoken;

/*
 * Global variables that control the parse state.
 */

struct fcode *dictionary = NULL;
struct macro *aliases = NULL;
int outf = 1; /* stdout */
int state = 0;
int nextfcode = 0x800; 
int numbase = TOK_HEX;
long outpos;
char *outbuf = NULL;
char *outfile, *infile;
#define BUFCLICK	(1024*1024)
size_t outbufsiz = 0;
char *myname = NULL;
int offsetsize = 8;
int defining = 0;
int tokenizer = 0;

#define PSTKSIZ		1024
Cell parse_stack[PSTKSIZ];
int parse_stack_ptr = 0;

void	token_err(int, const char *, const char *, const char *, ...)
	__attribute__((__format__(__printf__, 4, 5)));
YY_DECL;

int debug = 0;
#define ASSERT if (debug) assert
#define STATE(y, x)	do { if (debug) printf( "%ld State %s: token `%s'\n", outpos, x, y); } while (0)

#define YY_NO_UNPUT
%}

%%

0		{ ltoken.type = TOK_OTHER; ltoken.text = yytext; return &ltoken; }

1		{ ltoken.type = TOK_OTHER; ltoken.text = yytext; return &ltoken; }

2		{ ltoken.type = TOK_OTHER; ltoken.text = yytext; return &ltoken; }

3		{ ltoken.type = TOK_OTHER; ltoken.text = yytext; return &ltoken; }

-1		{ ltoken.type = TOK_OTHER; ltoken.text = yytext; return &ltoken; }

\.		{ ltoken.type = TOK_OTHER; ltoken.text = yytext; return &ltoken; }

{white}*		/* whitespace -- keep looping */ ;

\\[^\n]*\n		/* end of line comment -- keep looping */ { STATE(yytext, "EOL comment"); }

-?{hex}+		{ ltoken.type = TOK_NUMBER; ltoken.text = yytext;
					return &ltoken; }

\'.\'		{ ltoken.type = TOK_C_LIT; ltoken.text = yytext; return &ltoken; }

\"{white}*(\\\"|[^"])*\"	{ ltoken.type = TOK_STRING_LIT; ltoken.text = yytext; 
				return &ltoken; } /* String started by `"' or `."' */

\.\({white}*(\\\"|[^)])*\)	{ ltoken.type = TOK_PSTRING; ltoken.text = yytext; 
				return &ltoken; } /* String of type `.(.....)' */

\.\"{white}*(\\\"|[^"])*\"	{ ltoken.type = TOK_PSTRING; ltoken.text = yytext; 
				return &ltoken; }

"("		{ ltoken.type = TOK_COMMENT; ltoken.text = yytext;
				return &ltoken; }

")"		{ ltoken.type = TOK_ENDCOMMENT; ltoken.text = yytext;
				return &ltoken; }

":"		{ ltoken.type = TOK_COLON; ltoken.text = yytext;
				return &ltoken; }

";"		{ ltoken.type = TOK_SEMICOLON; ltoken.text = yytext;
				return &ltoken; }

\'		{ ltoken.type = TOK_TOKENIZE; ltoken.text = yytext;
				return &ltoken; }

[aA][gG][aA][iI][nN]	{ ltoken.type = TOK_AGAIN; ltoken.text = yytext;
				return &ltoken; }

[aA][lL][iI][aA][sS]	{ ltoken.type = TOK_ALIAS; ltoken.text = yytext;
				return &ltoken; }

\[\'\]			{ ltoken.type = TOK_GETTOKEN; ltoken.text = yytext;
				return &ltoken; }

[aA][sS][cC][iI][iI]	{ ltoken.type = TOK_ASCII; ltoken.text = yytext;
				return &ltoken; }

[bB][eE][gG][iI][nN]	{ ltoken.type = TOK_BEGIN; ltoken.text = yytext;
				return &ltoken; }

[bB][uU][fF][fF][eE][rR]:	{ ltoken.type = TOK_BUFFER; ltoken.text = yytext;
				return &ltoken; }

[cC][aA][sS][eE]	{ ltoken.type = TOK_CASE; ltoken.text = yytext;
				return &ltoken; }

[cC][oO][nN][sS][tT][aA][nN][tT]	{ ltoken.type = TOK_CONSTANT; ltoken.text = yytext;
				return &ltoken; }

[cC][oO][nN][tT][rR][oO][lL]	{ ltoken.type = TOK_CONTROL; ltoken.text = yytext;
				return &ltoken; }

[cC][rR][eE][aA][tT][eE]	{ ltoken.type = TOK_CREATE; ltoken.text = yytext;
				return &ltoken; }

[dD]#		{ ltoken.type = TOK_DECIMAL; ltoken.text = yytext;
				return &ltoken; }

[dD][eE][cC][iI][mM][aA][lL]	{ ltoken.type = TOK_DECIMAL; ltoken.text = yytext;
				return &ltoken; }

[dD][eE][fF][eE][rR]	{ ltoken.type = TOK_DEFER; ltoken.text = yytext;
				return &ltoken; }

\??[dD][oO]	{ ltoken.type = TOK_DO; ltoken.text = yytext;
				return &ltoken; }

[eE][lL][sS][eE]	{ ltoken.type = TOK_ELSE; ltoken.text = yytext;
				return &ltoken; }

[eE][nN][dD][cC][aA][sS][eE]	{ ltoken.type = TOK_ENDCASE; ltoken.text = yytext;
				return &ltoken; }

[eE][nN][dD][oO][fF]	{ ltoken.type = TOK_ENDOF; ltoken.text = yytext;
				return &ltoken; }

[eE][xX][tT][eE][rR][nN][aA][lL]	{ ltoken.type = TOK_EXTERNAL; ltoken.text = yytext;
				return &ltoken; }

[fF][iI][eE][lL][dD]	{ ltoken.type = TOK_FIELD; ltoken.text = yytext;
				return &ltoken; }

[hH]#		{ ltoken.type = TOK_HEX; ltoken.text = yytext;
				return &ltoken; }

[hH][eE][aA][dD][eE][rR][lL][eE][sS][sS]	{ ltoken.type = TOK_HEADERLESS; ltoken.text = yytext;
				return &ltoken; }

[hH][eE][aA][dD][eE][rR][sS]	{ ltoken.type = TOK_HEADERS; ltoken.text = yytext;
				return &ltoken; }

[hH][eE][xX]	{ ltoken.type = TOK_HEX; ltoken.text = yytext;
				return &ltoken; }

[iI][fF]		{ ltoken.type = TOK_IF; ltoken.text = yytext;
				return &ltoken; }

\??[lL][eE][aA][vV][eE]	{ ltoken.type = TOK_LEAVE; ltoken.text = yytext;
				return &ltoken; }

\+?[lL][oO][oO][pP]	{ ltoken.type = TOK_LOOP; ltoken.text = yytext;
				return &ltoken; }

[oO]#		{ ltoken.type = TOK_OCTAL; ltoken.text = yytext;
				return &ltoken; }

[oO][cC][tT][aA][lL]	{ ltoken.type = TOK_OCTAL; ltoken.text = yytext;
				return &ltoken; }

[oO][fF]		{ ltoken.type = TOK_OF; ltoken.text = yytext;
				return &ltoken; }

[rR][eE][pP][eE][aA][tT]	{ ltoken.type = TOK_REPEAT; ltoken.text = yytext;
				return &ltoken; }

[tT][hH][eE][nN]	{ ltoken.type = TOK_THEN; ltoken.text = yytext;
				return &ltoken; }

[tT][oO]		{ ltoken.type = TOK_TO; ltoken.text = yytext;
				return &ltoken; }

[uU][nN][tT][iI][lL]	{ ltoken.type = TOK_UNTIL; ltoken.text = yytext;
				return &ltoken; }

[vV][aA][lL][uU][eE]	{ ltoken.type = TOK_VALUE; ltoken.text = yytext;
				return &ltoken; }

[vV][aA][rR][iI][aA][bB][lL][eE]	{ ltoken.type = TOK_VARIABLE; ltoken.text = yytext;
				return &ltoken; }

[wW][hH][iI][lL][eE]	{ ltoken.type = TOK_WHILE; ltoken.text = yytext;
				return &ltoken; }

offset16		{ ltoken.type = TOK_OFFSET16; ltoken.text = yytext;
				return &ltoken; }

tokenizer\[	{ ltoken.type = TOK_BEGTOK; ltoken.text = yytext;
				return &ltoken; }

emit-byte		{ ltoken.type = TOK_EMIT_BYTE; ltoken.text = yytext;
				return &ltoken; }

\]tokenizer	{ ltoken.type = TOK_ENDTOK; ltoken.text = yytext;
				return &ltoken; }

fload		{ ltoken.type = TOK_FLOAD; ltoken.text = yytext;
				return &ltoken; }


[^ \n\t\r\f]+	{ ltoken.type = TOK_OTHER; ltoken.text = yytext;
				return &ltoken; }

<<EOF>>			{ return NULL; }
%%

/* Function definitions */
void push(Cell);
Cell pop(void);
int depth(void);
int fadd(struct fcode *, struct fcode *);
struct fcode *flookup(struct fcode *, const char *);
int aadd(struct macro *, struct macro *);
struct macro *alookup(struct macro *, const char *);
void initdic(void);
void usage(const char *);
void tokenize(YY_BUFFER_STATE);
int emit(const char *);
int spit(long);
void sspit(const char *);
int apply_macros(YY_BUFFER_STATE, const char *);
int main(int argc, char *argv[]);
Cell cvt(const char *, char **, int base);

/*
 * Standard FCode names and numbers.  Includes standard
 * tokenizer aliases.
 */
struct fcode fcodes[] = {
		{ "end0",			0x0000, 0, NULL, NULL },
		{ "b(lit)",			0x0010, 0, NULL, NULL },
		{ "b(')",			0x0011, 0, NULL, NULL },
		{ "b(\")",			0x0012, 0, NULL, NULL },
		{ "bbranch",			0x0013, 0, NULL, NULL },
		{ "b?branch",			0x0014, 0, NULL, NULL },
		{ "b(loop)",			0x0015, 0, NULL, NULL },
		{ "b(+loop)",			0x0016, 0, NULL, NULL },
		{ "b(do)",			0x0017, 0, NULL, NULL },
		{ "b(?do)",			0x0018, 0, NULL, NULL },
		{ "i",				0x0019, 0, NULL, NULL },
		{ "j",				0x001a, 0, NULL, NULL },
		{ "b(leave)",			0x001b, 0, NULL, NULL },
		{ "b(of)",			0x001c, 0, NULL, NULL },
		{ "execute",			0x001d, 0, NULL, NULL },
		{ "+",				0x001e, 0, NULL, NULL },
		{ "-",				0x001f, 0, NULL, NULL },
		{ "*",				0x0020, 0, NULL, NULL },
		{ "/",				0x0021, 0, NULL, NULL },
		{ "mod",			0x0022, 0, NULL, NULL },
		{ "and",			0x0023, 0, NULL, NULL },
		{ "or",				0x0024, 0, NULL, NULL },
		{ "xor",			0x0025, 0, NULL, NULL },
		{ "invert",			0x0026, 0, NULL, NULL },
		{ "lshift",			0x0027, 0, NULL, NULL },
		{ "rshift",			0x0028, 0, NULL, NULL },
		{ ">>a",			0x0029, 0, NULL, NULL },
		{ "/mod",			0x002a, 0, NULL, NULL },
		{ "u/mod",			0x002b, 0, NULL, NULL },
		{ "negate",			0x002c, 0, NULL, NULL },
		{ "abs",			0x002d, 0, NULL, NULL },
		{ "min",			0x002e, 0, NULL, NULL },
		{ "max",			0x002f, 0, NULL, NULL },
		{ ">r",				0x0030, 0, NULL, NULL },
		{ "r>",				0x0031, 0, NULL, NULL },
		{ "r@",				0x0032, 0, NULL, NULL },
		{ "exit",			0x0033, 0, NULL, NULL },
		{ "0=",				0x0034, 0, NULL, NULL },
		{ "0<>",			0x0035, 0, NULL, NULL },
		{ "0<",				0x0036, 0, NULL, NULL },
		{ "0<=",			0x0037, 0, NULL, NULL },
		{ "0>",				0x0038, 0, NULL, NULL },
		{ "0>=",			0x0039, 0, NULL, NULL },
		{ "<",				0x003a, 0, NULL, NULL },
		{ ">",				0x003b, 0, NULL, NULL },
		{ "=",				0x003c, 0, NULL, NULL },
		{ "<>",				0x003d, 0, NULL, NULL },
		{ "u>",				0x003e, 0, NULL, NULL },
		{ "u<=",			0x003f, 0, NULL, NULL },
		{ "u<",				0x0040, 0, NULL, NULL },
		{ "u>=",			0x0041, 0, NULL, NULL },
		{ ">=",				0x0042, 0, NULL, NULL },
		{ "<=",				0x0043, 0, NULL, NULL },
		{ "between",			0x0044, 0, NULL, NULL },
		{ "within",			0x0045, 0, NULL, NULL },
		{ "drop",			0x0046, 0, NULL, NULL },
		{ "dup",			0x0047, 0, NULL, NULL },
		{ "over",			0x0048, 0, NULL, NULL },
		{ "swap",			0x0049, 0, NULL, NULL },
		{ "rot",			0x004a, 0, NULL, NULL },
		{ "-rot",			0x004b, 0, NULL, NULL },
		{ "tuck",			0x004c, 0, NULL, NULL },
		{ "nip",			0x004d, 0, NULL, NULL },
		{ "pick",			0x004e, 0, NULL, NULL },
		{ "roll",			0x004f, 0, NULL, NULL },
		{ "?dup",			0x0050, 0, NULL, NULL },
		{ "depth",			0x0051, 0, NULL, NULL },
		{ "2drop",			0x0052, 0, NULL, NULL },
		{ "2dup",			0x0053, 0, NULL, NULL },
		{ "2over",			0x0054, 0, NULL, NULL },
		{ "2swap",			0x0055, 0, NULL, NULL },
		{ "2rot",			0x0056, 0, NULL, NULL },
		{ "2/",				0x0057, 0, NULL, NULL },
		{ "u2/",			0x0058, 0, NULL, NULL },
		{ "2*",				0x0059, 0, NULL, NULL },
		{ "/c",				0x005a, 0, NULL, NULL },
		{ "/w",				0x005b, 0, NULL, NULL },
		{ "/l",				0x005c, 0, NULL, NULL },
		{ "/n",				0x005d, 0, NULL, NULL },
		{ "ca+",			0x005e, 0, NULL, NULL },
		{ "wa+",			0x005f, 0, NULL, NULL },
		{ "la+",			0x0060, 0, NULL, NULL },
		{ "na+",			0x0061, 0, NULL, NULL },
		{ "char+",			0x0062, 0, NULL, NULL },
		{ "wa1+",			0x0063, 0, NULL, NULL },
		{ "la1+",			0x0064, 0, NULL, NULL },
		{ "cell+",			0x0065, 0, NULL, NULL },
		{ "chars",			0x0066, 0, NULL, NULL },
		{ "/w*",			0x0067, 0, NULL, NULL },
		{ "/l*",			0x0068, 0, NULL, NULL },
		{ "cells",			0x0069, 0, NULL, NULL },
		{ "on",				0x006a, 0, NULL, NULL },
		{ "off",			0x006b, 0, NULL, NULL },
		{ "+!",				0x006c, 0, NULL, NULL },
		{ "@",				0x006d, 0, NULL, NULL },
		{ "l@",				0x006e, 0, NULL, NULL },
		{ "w@",				0x006f, 0, NULL, NULL },
		{ "<w@",			0x0070, 0, NULL, NULL },
		{ "c@",				0x0071, 0, NULL, NULL },
		{ "!",				0x0072, 0, NULL, NULL },
		{ "l!",				0x0073, 0, NULL, NULL },
		{ "w!",				0x0074, 0, NULL, NULL },
		{ "c!",				0x0075, 0, NULL, NULL },
		{ "2@",				0x0076, 0, NULL, NULL },
		{ "2!",				0x0077, 0, NULL, NULL },
		{ "move",			0x0078, 0, NULL, NULL },
		{ "fill",			0x0079, 0, NULL, NULL },
		{ "comp",			0x007a, 0, NULL, NULL },
		{ "noop",			0x007b, 0, NULL, NULL },
		{ "lwsplit",			0x007c, 0, NULL, NULL },
		{ "wjoin",			0x007d, 0, NULL, NULL },
		{ "lbsplit",			0x007e, 0, NULL, NULL },
		{ "bljoin",			0x007f, 0, NULL, NULL },
		{ "wbflip",			0x0080, 0, NULL, NULL },
		{ "upc",			0x0081, 0, NULL, NULL },
		{ "lcc",			0x0082, 0, NULL, NULL },
		{ "pack",			0x0083, 0, NULL, NULL },
		{ "count",			0x0084, 0, NULL, NULL },
		{ "body>",			0x0085, 0, NULL, NULL },
		{ ">body",			0x0086, 0, NULL, NULL },
		{ "fcode-revision",		0x0087, 0, NULL, NULL },
		{ "span",			0x0088, 0, NULL, NULL },
		{ "unloop",			0x0089, 0, NULL, NULL },
		{ "expect",			0x008a, 0, NULL, NULL },
		{ "alloc-mem",			0x008b, 0, NULL, NULL },
		{ "free-mem",			0x008c, 0, NULL, NULL },
		{ "key?",			0x008d, 0, NULL, NULL },
		{ "key",			0x008e, 0, NULL, NULL },
		{ "emit",			0x008f, 0, NULL, NULL },
		{ "type",			0x0090, 0, NULL, NULL },
		{ "(cr",			0x0091, 0, NULL, NULL },
		{ "cr",				0x0092, 0, NULL, NULL },
		{ "#out",			0x0093, 0, NULL, NULL },
		{ "#line",			0x0094, 0, NULL, NULL },
		{ "hold",			0x0095, 0, NULL, NULL },
		{ "<#",				0x0096, 0, NULL, NULL },
		{ "u#>",			0x0097, 0, NULL, NULL },
		{ "sign",			0x0098, 0, NULL, NULL },
		{ "u#",				0x0099, 0, NULL, NULL },
		{ "u#s",			0x009a, 0, NULL, NULL },
		{ "u.",				0x009b, 0, NULL, NULL },
		{ "u.r",			0x009c, 0, NULL, NULL },
		{ ".",				0x009d, 0, NULL, NULL },
		{ ".r",				0x009e, 0, NULL, NULL },
		{ ".s",				0x009f, 0, NULL, NULL },
		{ "base",			0x00a0, 0, NULL, NULL },
		{ "convert",			0x00a1, 0, NULL, NULL },
		{ "$number",			0x00a2, 0, NULL, NULL },
		{ "digit",			0x00a3, 0, NULL, NULL },
		{ "-1",				0x00a4, 0, NULL, NULL },
		{ "true",			0x00a4, 0, NULL, NULL },
		{ "0",				0x00a5, 0, NULL, NULL },
		{ "1",				0x00a6, 0, NULL, NULL },
		{ "2",				0x00a7, 0, NULL, NULL },
		{ "3",				0x00a8, 0, NULL, NULL },
		{ "bl",				0x00a9, 0, NULL, NULL },
		{ "bs",				0x00aa, 0, NULL, NULL },
		{ "bell",			0x00ab, 0, NULL, NULL },
		{ "bounds",			0x00ac, 0, NULL, NULL },
		{ "here",			0x00ad, 0, NULL, NULL },
		{ "aligned",			0x00ae, 0, NULL, NULL },
		{ "wbsplit",			0x00af, 0, NULL, NULL },
		{ "bwjoin",			0x00b0, 0, NULL, NULL },
		{ "b(<mark)",			0x00b1, 0, NULL, NULL },
		{ "b(>resolve)",		0x00b2, 0, NULL, NULL },
		{ "set-token-table",		0x00b3, 0, NULL, NULL },
		{ "set-table",			0x00b4, 0, NULL, NULL },
		{ "new-token",			0x00b5, 0, NULL, NULL },
		{ "named-token",		0x00b6, 0, NULL, NULL },
		{ "b(:)",			0x00b7, 0, NULL, NULL },
		{ "b(value)",			0x00b8, 0, NULL, NULL },
		{ "b(variable)",		0x00b9, 0, NULL, NULL },
		{ "b(constant)",		0x00ba, 0, NULL, NULL },
		{ "b(create)",			0x00bb, 0, NULL, NULL },
		{ "b(defer)",			0x00bc, 0, NULL, NULL },
		{ "b(buffer:)",			0x00bd, 0, NULL, NULL },
		{ "b(field)",			0x00be, 0, NULL, NULL },
		{ "b(code)",			0x00bf, 0, NULL, NULL },
		{ "instance",			0x00c0, 0, NULL, NULL },
		{ "b(;)",			0x00c2, 0, NULL, NULL },
		{ "b(to)",			0x00c3, 0, NULL, NULL },
		{ "b(case)",			0x00c4, 0, NULL, NULL },
		{ "b(endcase)",			0x00c5, 0, NULL, NULL },
		{ "b(endof)",			0x00c6, 0, NULL, NULL },
		{ "#",				0x00c7, 0, NULL, NULL },
		{ "#s",				0x00c8, 0, NULL, NULL },
		{ "#>",				0x00c9, 0, NULL, NULL },
		{ "external-token",		0x00ca, 0, NULL, NULL },
		{ "$find",			0x00cb, 0, NULL, NULL },
		{ "offset16",			0x00cc, 0, NULL, NULL },
		{ "evaluate",			0x00cd, 0, NULL, NULL },
		{ "c,",				0x00d0, 0, NULL, NULL },
		{ "w,",				0x00d1, 0, NULL, NULL },
		{ "l,",				0x00d2, 0, NULL, NULL },
		{ "'",				0x00d3, 0, NULL, NULL },
		{ "um*",			0x00d4, 0, NULL, NULL },
		{ "um/mod",			0x00d5, 0, NULL, NULL },
		{ "d+",				0x00d8, 0, NULL, NULL },
		{ "d-",				0x00d9, 0, NULL, NULL },
		{ "get-token",			0x00da, 0, NULL, NULL },
		{ "set-token",			0x00db, 0, NULL, NULL },
		{ "state",			0x00dc, 0, NULL, NULL },
		{ "compile,",			0x00dd, 0, NULL, NULL },
		{ "behavior",			0x00de, 0, NULL, NULL },
		{ "start0",			0x00f0, 0, NULL, NULL },
		{ "start1",			0x00f1, 0, NULL, NULL },
		{ "start2",			0x00f2, 0, NULL, NULL },
		{ "start4",			0x00f3, 0, NULL, NULL },
		{ "ferror",			0x00fc, 0, NULL, NULL },
		{ "version1",			0x00fd, 0, NULL, NULL },
		{ "4-byte-id",			0x00fe, 0, NULL, NULL },
		{ "end1",			0x00ff, 0, NULL, NULL },
		{ "dma-alloc",			0x0101, 0, NULL, NULL },
		{ "my-address",			0x0102, 0, NULL, NULL },
		{ "my-space",			0x0103, 0, NULL, NULL },
		{ "memmap",			0x0104, 0, NULL, NULL },
		{ "free-virtual",		0x0105, 0, NULL, NULL },
		{ ">physical",			0x0106, 0, NULL, NULL },
		{ "my-params",			0x010f, 0, NULL, NULL },
		{ "property",			0x0110, 0, NULL, NULL },
		{ "encode-int",			0x0111, 0, NULL, NULL },
		{ "encode+",			0x0112, 0, NULL, NULL },
		{ "encode-phys",		0x0113, 0, NULL, NULL },
		{ "encode-string",		0x0114, 0, NULL, NULL },
		{ "encode-bytes",		0x0115, 0, NULL, NULL },
		{ "reg",			0x0116, 0, NULL, NULL },
		{ "intr",			0x0117, 0, NULL, NULL },
		{ "driver",			0x0118, 0, NULL, NULL },
		{ "model",			0x0119, 0, NULL, NULL },
		{ "device-type",		0x011a, 0, NULL, NULL },
		{ "parse-2int",			0x011b, 0, NULL, NULL },
		{ "is-install",			0x011c, 0, NULL, NULL },
		{ "is-remove",			0x011d, 0, NULL, NULL },
		{ "is-selftest",		0x011e, 0, NULL, NULL },
		{ "new-device",			0x011f, 0, NULL, NULL },
		{ "diagnostic-mode?",		0x0120, 0, NULL, NULL },
		{ "display-status",		0x0121, 0, NULL, NULL },
		{ "memory-test-suite",		0x0122, 0, NULL, NULL },
		{ "group-code",			0x0123, 0, NULL, NULL },
		{ "mask",			0x0124, 0, NULL, NULL },
		{ "get-msecs",			0x0125, 0, NULL, NULL },
		{ "ms",				0x0126, 0, NULL, NULL },
		{ "find-device",		0x0127, 0, NULL, NULL },
		{ "decode-phys",		0x0128, 0, NULL, NULL },
		{ "map-low",			0x0130, 0, NULL, NULL },
		{ "sbus-intr>cpu",		0x0131, 0, NULL, NULL },
		{ "#lines",			0x0150, 0, NULL, NULL },
		{ "#columns",			0x0151, 0, NULL, NULL },
		{ "line#",			0x0152, 0, NULL, NULL },
		{ "column#",			0x0153, 0, NULL, NULL },
		{ "inverse?",			0x0154, 0, NULL, NULL },
		{ "inverse-screen?",		0x0155, 0, NULL, NULL },
		{ "frame-buffer-busy?",		0x0156, 0, NULL, NULL },
		{ "draw-character",		0x0157, 0, NULL, NULL },
		{ "reset-screen",		0x0158, 0, NULL, NULL },
		{ "toggle-cursor",		0x0159, 0, NULL, NULL },
		{ "erase-screen",		0x015a, 0, NULL, NULL },
		{ "blink-screen",		0x015b, 0, NULL, NULL },
		{ "invert-screen",		0x015c, 0, NULL, NULL },
		{ "insert-characters",		0x015d, 0, NULL, NULL },
		{ "delete-characters",		0x015e, 0, NULL, NULL },
		{ "insert-lines",		0x015f, 0, NULL, NULL },
		{ "delete-lines",		0x0160, 0, NULL, NULL },
		{ "draw-logo",			0x0161, 0, NULL, NULL },
		{ "frame-buffer-addr",		0x0162, 0, NULL, NULL },
		{ "screen-height",		0x0163, 0, NULL, NULL },
		{ "screen-width",		0x0164, 0, NULL, NULL },
		{ "window-top",			0x0165, 0, NULL, NULL },
		{ "window-left",		0x0166, 0, NULL, NULL },
		{ "default-font",		0x016a, 0, NULL, NULL },
		{ "set-font",			0x016b, 0, NULL, NULL },
		{ "char-height",		0x016c, 0, NULL, NULL },
		{ "char-width",			0x016d, 0, NULL, NULL },
		{ ">font",			0x016e, 0, NULL, NULL },
		{ "fontbytes",			0x016f, 0, NULL, NULL },
		{ "fb8-draw-character",		0x0180, 0, NULL, NULL },
		{ "fb8-reset-screen",		0x0181, 0, NULL, NULL },
		{ "fb8-toggle-cursor",		0x0182, 0, NULL, NULL },
		{ "fb8-erase-screen",		0x0183, 0, NULL, NULL },
		{ "fb8-blink-screen",		0x0184, 0, NULL, NULL },
		{ "fb8-invert-screen",		0x0185, 0, NULL, NULL },
		{ "fb8-insert-characters",	0x0186, 0, NULL, NULL },
		{ "fb8-delete-characters",	0x0187, 0, NULL, NULL },
		{ "fb8-inisert-lines",		0x0188, 0, NULL, NULL },
		{ "fb8-delete-lines",		0x0189, 0, NULL, NULL },
		{ "fb8-draw-logo",		0x018a, 0, NULL, NULL },
		{ "fb8-install",		0x018b, 0, NULL, NULL },
		{ "return-buffer",		0x01a0, 0, NULL, NULL },
		{ "xmit-packet",		0x01a1, 0, NULL, NULL },
		{ "poll-packet",		0x01a2, 0, NULL, NULL },
		{ "mac-address",		0x01a4, 0, NULL, NULL },
		{ "device-name",		0x0201, 0, NULL, NULL },
		{ "my-args",			0x0202, 0, NULL, NULL },
		{ "my-self",			0x0203, 0, NULL, NULL },
		{ "find-package",		0x0204, 0, NULL, NULL },
		{ "open-package",		0x0205, 0, NULL, NULL },
		{ "close-package",		0x0206, 0, NULL, NULL },
		{ "find-method",		0x0207, 0, NULL, NULL },
		{ "call-package",		0x0208, 0, NULL, NULL },
		{ "$call-parent",		0x0209, 0, NULL, NULL },
		{ "my-parent",			0x020a, 0, NULL, NULL },
		{ "ihandle>phandle",		0x020b, 0, NULL, NULL },
		{ "my-unit",			0x020d, 0, NULL, NULL },
		{ "$call-method",		0x020e, 0, NULL, NULL },
		{ "$open-package",		0x020f, 0, NULL, NULL },
		{ "processor-type",		0x0210, 0, NULL, NULL },
		{ "firmware-version",		0x0211, 0, NULL, NULL },
		{ "fcode-version",		0x0212, 0, NULL, NULL },
		{ "alarm",			0x0213, 0, NULL, NULL },
		{ "(is-user-word)",		0x0214, 0, NULL, NULL },
		{ "suspend-fcode",		0x0215, 0, NULL, NULL },
		{ "abort",			0x0216, 0, NULL, NULL },
		{ "catch",			0x0217, 0, NULL, NULL },
		{ "throw",			0x0218, 0, NULL, NULL },
		{ "user-abort",			0x0219, 0, NULL, NULL },
		{ "get-my-property",		0x021a, 0, NULL, NULL },
		{ "decode-int",			0x021b, 0, NULL, NULL },
		{ "decode-string",		0x021c, 0, NULL, NULL },
		{ "get-inherited-property",	0x021d, 0, NULL, NULL },
		{ "delete-property",		0x021e, 0, NULL, NULL },
		{ "get-package-property",	0x021f, 0, NULL, NULL },
		{ "cpeek",			0x0220, 0, NULL, NULL },
		{ "wpeek",			0x0221, 0, NULL, NULL },
		{ "lpeek",			0x0222, 0, NULL, NULL },
		{ "cpoke",			0x0223, 0, NULL, NULL },
		{ "wpoke",			0x0224, 0, NULL, NULL },
		{ "lpoke",			0x0225, 0, NULL, NULL },
		{ "lwflip",			0x0226, 0, NULL, NULL },
		{ "lbflip",			0x0227, 0, NULL, NULL },
		{ "lbflips",			0x0228, 0, NULL, NULL },
		{ "adr-mask",			0x0229, 0, NULL, NULL },
		{ "rb@",			0x0230, 0, NULL, NULL },
		{ "rb!",			0x0231, 0, NULL, NULL },
		{ "rw@",			0x0232, 0, NULL, NULL },
		{ "rw!",			0x0233, 0, NULL, NULL },
		{ "rl@",			0x0234, 0, NULL, NULL },
		{ "rl!",			0x0235, 0, NULL, NULL },
		{ "wbflips",			0x0236, 0, NULL, NULL },
		{ "lwflips",			0x0237, 0, NULL, NULL },
		{ "probe",			0x0238, 0, NULL, NULL },
		{ "probe-virtual",		0x0239, 0, NULL, NULL },
		{ "child",			0x023b, 0, NULL, NULL },
		{ "peer",			0x023c, 0, NULL, NULL },
		{ "next-property",		0x023d, 0, NULL, NULL },
		{ "byte-load",			0x023e, 0, NULL, NULL },
		{ "set-args",			0x023f, 0, NULL, NULL },
		{ "left-parse-string",		0x0240, 0, NULL, NULL },
			/* 64-bit FCode extensions */
		{ "bxjoin",			0x0241, 0, NULL, NULL },
		{ "<l@",			0x0242, 0, NULL, NULL },
		{ "lxjoin",			0x0243, 0, NULL, NULL },
		{ "rx@",			0x022e, 0, NULL, NULL },
		{ "rx!",			0x022f, 0, NULL, NULL },
		{ "wxjoin",			0x0244, 0, NULL, NULL },
		{ "x,",				0x0245, 0, NULL, NULL },
		{ "x@",				0x0246, 0, NULL, NULL },
		{ "x!",				0x0247, 0, NULL, NULL },
		{ "/x",				0x0248, 0, NULL, NULL },
		{ "/x*",			0x0249, 0, NULL, NULL },
		{ "xa+",			0x024a, 0, NULL, NULL },
		{ "xa1+",			0x024b, 0, NULL, NULL },
		{ "xbflip",			0x024c, 0, NULL, NULL },
		{ "xbflips",			0x024d, 0, NULL, NULL },
		{ "xbsplit",			0x024e, 0, NULL, NULL },
		{ "xlflip",			0x024f, 0, NULL, NULL },
		{ "xlflips",			0x0250, 0, NULL, NULL },
		{ "xlsplit",			0x0251, 0, NULL, NULL },
		{ "xwflip",			0x0252, 0, NULL, NULL },
		{ "xwflips",			0x0253, 0, NULL, NULL },
		{ "xwsplit",			0x0254, 0, NULL, NULL },
		{ NULL,				0, 0, NULL, NULL }
};

/*
 * Default macros -- can be overridden by colon definitions.
 */
struct macro macros[] = {
	{ "eval",	"evaluate", 0, NULL, NULL }, /* Build a more balanced tree */
	{ "(.)",	"dup abs <# u#s swap sign u#>", 0, NULL, NULL },
	{ "<<",		"lshift", 0, NULL, NULL },
	{ ">>",		"rshift", 0, NULL, NULL },
	{ "?",		"@ .", 0, NULL, NULL },
	{ "1+",		"1 +", 0, NULL, NULL },
	{ "1-",		"1 -", 0, NULL, NULL },
	{ "2+",		"2 +", 0, NULL, NULL },
	{ "2-",		"2 -", 0, NULL, NULL },
	{ "abort\"",	"-2 throw", 0, NULL, NULL },
	{ "accept",	"span @ -rot expect span @ swap span !", 0, NULL, NULL },
	{ "allot",	"0 max 0 ?do 0 c, loop", 0, NULL, NULL },
	{ "blank",	"bl fill", 0, NULL, NULL },
	{ "/c*",	"chars", 0, NULL, NULL },
	{ "ca1+",	"char+", 0, NULL, NULL },
	{ "carret",	"b(lit) 00 00 00 0x0d", 0, NULL, NULL },
	{ ".d",		"base @ swap 0x0a base ! . base !", 0, NULL, NULL },
	{ "decode-bytes", ">r over r@ + swap r@ - rot r>", 0, NULL, NULL },
	{ "3drop",	"drop 2drop", 0, NULL, NULL },
	{ "3dup",	"2 pick 2 pick 2 pick", 0, NULL, NULL },
	{ "erase",	"0 fill", 0, NULL, NULL },
	{ "false",	"0", 0, NULL, NULL },
	{ ".h",		"base @ swap 0x10 base ! . base !", 0, NULL, NULL },
	{ "linefeed",	"b(lit) 00 00 00 0x0a", 0, NULL, NULL },
	{ "/n*",	"cells", 0, NULL, NULL },
	{ "na1+",	"cell+", 0, NULL, NULL },
	{ "not",	"invert", 0, NULL, NULL },
	{ "s.",		"(.) type space", 0, NULL, NULL },
	{ "space",	"bl emit", 0, NULL, NULL },
	{ "spaces",	"0 max 0 ?do space loop", 0, NULL, NULL },
	{ "struct",	"0", 0, NULL, NULL },
	{ "true",	"-1", 0, NULL, NULL },
	{ "(u,)",	"<# u#s u#>", 0, NULL, NULL },
	{ NULL, NULL, 0, NULL, NULL }
};

/*
 * Utility functions.
 */

/*
 * ASCII -> long int converter, eats `.'s
 */
#define strtol(x, y, z)		cvt(x, y, z)
Cell
cvt(const char *s, char **e, int base)
{
	Cell v = 0;
	int c, n = 0;

	c = *s;
	if (c == '-') { n = 1; s++; }

	for (c = *s; (c = *s); s++) {

		/* Ignore `.' */
		if (c == '.') 
			continue;
		if (c >= '0' && c <= '9')
			c -= '0';
		else if (c >= 'a' && c <= 'f')
			c += 10 - 'a';
		else if (c >= 'A' && c <= 'F')
			c += 10 - 'A';
		if (c >= base)
			break;
		v *= base;
		v += c;
	}
	if (e)
		*e = __UNCONST(s);
	if (n)
		return (-v);
	return (v);
}

/*
 * Parser stack control functions.
 */

void
push(Cell val)
{
	parse_stack[parse_stack_ptr++] = val;
	if (parse_stack_ptr >= PSTKSIZ) {
		(void)printf( "Parse stack overflow\n");
		exit(1);
	}
}

Cell
pop(void)
{
	ASSERT(parse_stack_ptr);
	return parse_stack[--parse_stack_ptr];
}

int
depth(void)
{
	return (parse_stack_ptr);
}

/*
 * Insert fcode into dictionary.
 */
int
fadd(struct fcode *dict, struct fcode *new)
{
	int res = strcmp(dict->name, new->name);

#ifdef DEBUG
	new->type = FCODE;
	ASSERT(dict->type == FCODE);
#endif
	/* Don't allow duplicate entries. */
	if (!res) return (0);
	if (res < 0) {
		if (dict->l)
			return fadd(dict->l, new);
		else {
#ifdef DEBUG
			if (debug > 1)
				(void)printf( "fadd: new FCode `%s' is %lx\n", 
					      new->name, new->num);
#endif
			new->l = new->r = NULL;
			dict->l = new;
		}
	} else {
		if (dict->r)
			return fadd(dict->r, new);
		else {
#ifdef DEBUG
			if (debug > 1)
				(void)printf( "fadd: new FCode `%s' is %lx\n", 
					      new->name, new->num);
#endif
			new->l = new->r = NULL;
			dict->r = new;
		}
	}
	return (1);
}

/*
 * Look for a code in the dictionary.
 */
struct fcode *
flookup(struct fcode *dict, const char *str)
{
	int res;
	if (!dict) return (dict);

	res = strcmp(dict->name, str);
#ifdef DEBUG
	ASSERT(dict->type == FCODE);
	if (debug > 2)
		(void)printf( "flookup: `%s' and `%s' %s match\n", 
			      str, dict->name, res?"don't":"do");
#endif
	if (!res) return (dict);
	if (res < 0)
		return (flookup(dict->l, str));
	else 
		return (flookup(dict->r, str));

}

/*
 * Insert alias into macros.
 */
int
aadd(struct macro *dict, struct macro *new)
{
	int res = strcmp(dict->name, new->name);

#ifdef DEBUG
	new->type = MACRO;
	ASSERT(dict->type == MACRO);
#endif
	/* Don't allow duplicate entries. */
	if (!res) return (0);
	if (res < 0) {
		if (dict->l)
			return aadd(dict->l, new);
		else {
			new->l = new->r = NULL;
			dict->l = new;
#ifdef DEBUG
			if (debug > 1)
				(void)printf( "aadd: new alias `%s' to `%s'\n", 
					      new->name, new->equiv);
#endif
		}
	} else {
		if (dict->r)
			return aadd(dict->r, new);
		else {
			new->l = new->r = NULL;
			dict->r = new;
#ifdef DEBUG
			if (debug > 1)
				(void)printf( "aadd: new alias `%s' to `%s'\n", 
					      new->name, new->equiv);
#endif
		}
	}
	return (1);
}

/*
 * Look for a macro in the aliases.
 */
struct macro *
alookup(struct macro *dict, const char *str)
{
	int res;
	if (!dict) return (dict);

#ifdef DEBUG
	ASSERT(dict->type == MACRO);
#endif
	res = strcmp(dict->name, str);
	if (!res) return (dict);
	if (res < 0)
		return (alookup(dict->l, str));
	else 
		return (alookup(dict->r, str));

}

/*
 * Bootstrap the dictionary and then install
 * all the standard FCodes.
 */
void
initdic(void)
{
	struct fcode *code = fcodes;
	struct macro *alias = macros;

	ASSERT(dictionary == NULL);
	code->l = code->r = NULL;
	dictionary = code;
#ifdef DEBUG
	code->type = FCODE;
#endif

	while ((++code)->name) {
		if(!fadd(dictionary, code)) {
			printf("init: duplicate dictionary entry %s\n", 
			       code->name);
			abort();
		}
	}

	ASSERT(aliases == NULL);
	aliases = alias;
	alias->l = alias->r = NULL;
#ifdef DEBUG
	alias->type = MACRO;
#endif
	while ((++alias)->name) {
		if(!aadd(aliases, alias)) {
			printf("init: duplicate macro entry %s\n", 
			       alias->name);
			abort();
		}
	}

}

int
apply_macros(YY_BUFFER_STATE yinput, const char *str)
{
	struct macro *xform = alookup(aliases, str);
	
	if (xform) {
		YY_BUFFER_STATE newbuf;

		newbuf = yy_scan_string(xform->equiv);
		yy_switch_to_buffer(newbuf);
		tokenize(newbuf);
		yy_switch_to_buffer(yinput);
		yy_delete_buffer(newbuf);
	}
	return (xform != NULL);
}

void
usage(const char *me)
{
	(void)fprintf(stderr, "%s: [-d level] [-o outfile] infile\n", me);
	exit(1);
}

int
main(int argc, char *argv[])
{
	int bflag, ch;
	FILE *inf;
	struct fcode_header *fheader;
	YY_BUFFER_STATE inbuf;
	const char *hdrtype = "version1";
	int i;

	outf = 1; /* stdout */
	myname = argv[0];

	bflag = 0;
	while ((ch = getopt(argc, argv, "d:o:")) != -1)
		switch(ch) {
		case 'd':
			debug = atol(optarg);
			break;
		case 'o':
			outfile = optarg;
			break;
		default:
			usage(myname);
		}
	argc -= optind;
	argv += optind;
	
	if (argc != 1)
		usage(myname);
	
	infile = argv[0];

	/*
	 * Initialization stuff.
	 */
	initdic();
	outbufsiz = BUFCLICK;
	outbuf = malloc(outbufsiz);
	fheader = (struct fcode_header *)outbuf;
	outpos = 0;
	emit(hdrtype);
	outpos = sizeof(*fheader);

	/* 
	 * Do it.
	 */
	if ((inf = fopen(infile, "r")) == NULL)
		(void)err(1, "can not open %s for reading", infile);

	inbuf = yy_create_buffer( inf, YY_BUF_SIZE );
	yy_switch_to_buffer(inbuf);
	tokenize(inbuf);
	yy_delete_buffer(inbuf);
	fclose(inf);
	emit("end0");

	/* Now calculate length and checksum and stick them in the header */
	fheader->format = 0x08;
	fheader->length = htonl(outpos);
	fheader->checksum = 0;
	for (i = sizeof(*fheader); i<outpos; i++)
		fheader->checksum += outbuf[i];
	fheader->checksum = htons(fheader->checksum);

	if ((outf = open(outfile, O_WRONLY|O_CREAT|O_TRUNC, 0666)) == -1)
		err(1, "can out open %s for writing", outfile);

	if (write(outf, outbuf, outpos) != outpos) {
		close(outf);
		unlink(outfile);
		err(1, "write error");
	}
	close(outf);
	return (0);
};

/*
 * Tokenize one file.  This is a separate function so it can
 * be called recursively to parse mutiple levels of include files.
 */

void
tokenize(YY_BUFFER_STATE yinput)
{
	FILE *inf;
	YY_BUFFER_STATE inbuf;
	TOKEN *token;
	const char *last_token = "";
	struct fcode *fcode;
	int pos, off;

	while ((token = yylex()) != NULL) {
		switch (token->type) {
		case TOK_NUMBER:
			STATE(token->text, "TOK_NUMBER");
		{ 
			char *end;
			Cell value;
			
			if (tokenizer) {
				push(strtol(token->text, &end, 16));
				break;
			}
			value = strtol(token->text, &end, numbase);
			if (*end != 0)
				token_err(yylineno, infile, yytext,
				    "illegal number conversion");

			/* 
			 * If this is a 64-bit value we need to store two literals
			 * and issue a `lxjoin' to combine them.  But that's a future
			 * project.
			 */
			emit("b(lit)");
			spit((value>>24)&0x0ff);
			spit((value>>16)&0x0ff);
			spit((value>>8)&0x0ff);
			spit(value&0x0ff);
			if ((value>>32) != value && (value>>32) != 0 && 
				(value>>32) != -1) {
				emit("b(lit)");
				spit((value>>56)&0x0ff);
				spit((value>>48)&0x0ff);
				spit((value>>40)&0x0ff);
				spit((value>>32)&0x0ff);
				emit("lxjoin");
			}
		}
		break;
		case TOK_C_LIT:
			STATE(token->text, "TOK_C_LIT");
			emit("b(lit)");
			spit(0);
			spit(0);
			spit(0);
			spit(token->text[1]);
		break;
		case TOK_STRING_LIT: 
			STATE(token->text, "TOK_STRING_LIT:");
		{
			int len;
			char *p = token->text;
			
			++p;			/* Skip the quote */
			len = strlen(++p);	/* Skip the 1st space */

#define ERR_TOOLONG	\
	token_err(yylineno, infile, yytext, "string length %d too long", len)

			if (len > 255)
				ERR_TOOLONG;

			if (p[len-1] == ')' ||
			    p[len-1] == '"') {
				p[len-1] = 0;
			}
			emit("b(\")");
			sspit(p);
		}
		break;
		case TOK_PSTRING: 
			STATE(token->text, "TOK_PSTRING:");
		{
			int len;
			char *p = token->text;

			if (*p++ == '.') p++; /* Skip over delimiter */
			p++; /* Skip over space/tab */

			len = strlen(p);
			if (len > 255)
				ERR_TOOLONG;

			if (p[len-1] == ')' ||
			    p[len-1] == '"') {
				p[len-1] = 0;
			}
			emit("b(\")");
			sspit(p);
			emit("type");
		}
		break;
		case TOK_TOKENIZE:
			STATE(token->text, "TOK_TOKENIZE");
			/* The next pass should tokenize the FCODE number */
			emit("b(')");
			break;
		case TOK_COMMENT: 
			STATE(token->text, "TOK_COMMENT:");
			while (((token = yylex()) != NULL) && token->type != TOK_ENDCOMMENT)
				;
			break;
		case TOK_ENDCOMMENT:
			STATE(token->text, "TOK_ENDCOMMENT");
			token_err(yylineno, infile, NULL,
			    "ENDCOMMENT encountered outside comment");
			break;
		case TOK_COLON: 
			STATE(token->text, "TOK_COLON:");

			token = yylex();
			if (token == NULL)
				token_err(yylineno, infile, yytext,
				    "EOF in colon definition");
			
			/* Add new code to dictionary */
			fcode = malloc(sizeof(*fcode));
			fcode->num = nextfcode++;
			fcode->name = strdup(token->text);
			if (!fadd(dictionary, fcode))
				token_err(yylineno, infile, NULL,
				    "Duplicate definition: `%s'\n", fcode->name);
#ifdef DEBUG
			if (debug)
				(void)printf("Adding %s to dictionary\n", token->text);		
#endif
			if (state == 0)
				emit("new-token");
			else {
				if (state == TOK_EXTERNAL)
					emit("external-token");
				else
				/* Here we have a choice of new-token or named-token */
					emit("named-token");
				sspit(token->text);
			}
			spit(fcode->num);
			emit("b(:)");
			last_token = fcode->name;
			defining = 1;
 			break;
		case TOK_SEMICOLON: 
			STATE(token->text, "TOK_SEMICOLON:");
			emit("b(;)");
			defining = 0;
			if (depth()) {
				token_err(yylineno, infile, NULL,
				    "Warning: stack depth %d at end of %s\n",
				    depth(), last_token);
			}
			last_token = "";
			break;

			/* These are special */
		case TOK_AGAIN:
			STATE(token->text, "TOK_AGAIN");
			emit("bbranch");
			pos = pop();
			pos -= outpos;
			if (offsetsize == 16) {
				spit((pos>>8)&0xff);
			}
			spit(pos&0xff);
			break;
		case TOK_ALIAS:
			STATE(token->text, "TOK_ALIAS");
		{
			struct macro *alias;

			token = yylex();
			if (token == NULL) {
				(void)printf( "EOF in alias definition\n");
				return;
			}
			if (token->type != TOK_OTHER) {
				(void)printf( "ENDCOMMENT aliasing weird token type %d\n",
					      token->type);
			}
			alias = malloc(sizeof(*alias));
			alias->name = strdup(token->text);
			token = yylex();
			if (token == NULL) {
				free(__UNCONST(alias->name));
				free(alias);
				(void)printf( "EOF in alias definition\n");
				return;
			}			
			alias->equiv = strdup(token->text);
			if (!aadd(aliases, alias)) {
				(void)printf( "ERROR: Duplicate alias %s\n",
					      alias->name);
				exit(1);
			}
		}
		break;
		case TOK_GETTOKEN:
			STATE(token->text, "TOK_GETTOKEN");
			/* This is caused by ['] */
			emit("b(')");
			token = yylex();
			if (token == NULL) {
				(void)printf( "EOF in [']\n");
				return;
			}
			if ((fcode = flookup(dictionary, token->text)) == NULL) {
				(void)printf( "[']: %s not found\n", token->text);
				exit(1);
			}
			spit(fcode->num);
			break;
		case TOK_ASCII:
			STATE(token->text, "TOK_ASCII");
			token = yylex();
			if (token == NULL) {
				(void)printf( "EOF after \"ascii\"\n");
				exit(1);
			}
			emit("b(lit)");
			spit(0);
			spit(0);
			spit(0);
			spit(token->text[0]);
			break;
		case TOK_BEGIN:
			STATE(token->text, "TOK_BEGIN");
			emit("b(<mark)");
			push(outpos);
			break;
		case TOK_BUFFER:
			STATE(token->text, "TOK_BUFFER");

			token = yylex();
			if (token == NULL) {
				(void)printf( "EOF in colon definition\n");
				return;
			}
			
			/* Add new code to dictionary */
			fcode = malloc(sizeof(*fcode));
			fcode->num = nextfcode++;
			fcode->name = strdup(token->text);
			fadd(dictionary, fcode);
			
			if (state == 0)
				emit("new-token");
			else {
				if (state == TOK_EXTERNAL)
					emit("external-token");
				else
				/* Here we have a choice of new-token or named-token */
					emit("named-token");
				sspit(token->text);
			}
			spit(fcode->num);
			emit("b(buffer:)");
			break;
		case TOK_CASE:
			STATE(token->text, "TOK_CASE");
			emit("b(case)");
			push(0);
			break;
		case TOK_CONSTANT:
			STATE(token->text, "TOK_CONSTANT");

			token = yylex();
			if (token == NULL) {
				(void)printf( "EOF in constant definition\n");
				return;
			}
			
			/* Add new code to dictionary */
			fcode = malloc(sizeof(*fcode));
			fcode->num = nextfcode++;
			fcode->name = strdup(token->text);
			fadd(dictionary, fcode);
			
			if (state == 0)
				emit("new-token");
			else {
				if (state == TOK_EXTERNAL)
					emit("external-token");
				else
				/* Here we have a choice of new-token or named-token */
					emit("named-token");
				sspit(token->text);
			}
			spit(fcode->num);
			emit("b(constant)");
			break;
		case TOK_CONTROL:
			STATE(token->text, "TOK_CONTROL");
			token = yylex();
			if (token == NULL) {
				(void)printf( "EOF after \"ascii\"\n");
				exit(1);
			}
			emit("b(lit)");
			spit(0);
			spit(0);
			spit(0);
			spit(token->text[0]&0x1f);
			break;
		case TOK_CREATE:
			STATE(token->text, "TOK_CREATE");
			/* Don't know what this does or if it's right */
			token = yylex();
			if (token == NULL) {
				(void)printf( "EOF in create definition\n");
				return;
			}
			
			/* Add new code to dictionary */
			fcode = malloc(sizeof(*fcode));
			fcode->num = nextfcode++;
			fcode->name = strdup(token->text);
			fadd(dictionary, fcode);
			
			if (state == 0)
				emit("new-token");
			else {
				if (state == TOK_EXTERNAL)
					emit("external-token");
				else
				/* Here we have a choice of new-token or named-token */
					emit("named-token");
				sspit(token->text);
			}
			spit(fcode->num);
			emit("b(create)");
			break;
		case TOK_DECIMAL:
			STATE(token->text, "TOK_DECIMAL");
			if (token->text[1] != '#') {
				if (defining) {
					spit(10);
					emit("base");
					emit("!");
				} else
					numbase = TOK_DECIMAL;
			} else {
				char *end;
				Cell value;

				token = yylex();
				if (token == NULL) {
					(void)printf( "EOF after d#\n");
					return;
				}
				if (token->type == TOK_OTHER) {
					if (strcmp("-1", token->text) == 0) {
						emit(token->text);
						break;
					}
				}
				value = strtol(token->text, &end, 10);
				if (*end != 0)
					token_err(yylineno, infile, NULL,
					    "Illegal number conversion: %s", token->text);

				/* 
				 * If this is a 64-bit value we need to store two literals
				 * and issue a `lxjoin' to combine them.  But that's a future
				 * project.
				 */
				emit("b(lit)");
				spit((value>>24)&0x0ff);
				spit((value>>16)&0x0ff);
				spit((value>>8)&0x0ff);
				spit(value&0x0ff);
				if ((value>>32) != value && (value>>32) != 0) {
					emit("b(lit)");
					spit((value>>56)&0x0ff);
					spit((value>>48)&0x0ff);
					spit((value>>40)&0x0ff);
					spit((value>>32)&0x0ff);
					emit("lxjoin");
				}
			}
			break;
		case TOK_DEFER:
			STATE(token->text, "TOK_DEFER");
			/* Don't know what this does or if it's right */
			token = yylex();
			if (token == NULL) {
				(void)printf( "EOF in colon definition\n");
				return;
			}
			
			/* Add new code to dictionary */
			fcode = malloc(sizeof(*fcode));
			fcode->num = nextfcode++;
			fcode->name = strdup(token->text);
			fadd(dictionary, fcode);
			
			if (state == 0)
				emit("new-token");
			else {
				if (state == TOK_EXTERNAL)
					emit("external-token");
				else
				/* Here we have a choice of new-token or named-token */
					emit("named-token");
				sspit(token->text);
			}
			spit(fcode->num);
			emit("b(defer)");
			break;
		case TOK_DO:
			STATE(token->text, "TOK_DO");
			/*
			 * From the 1275 spec.  B is branch location, T is branch target.
			 *
			 *	b(do)  offset1 ... b(loop)  offset2 ...
			 *	b(do)  offset1 ... b(+loop) offset2 ...
			 *	b(?do) offset1 ... b(loop)  offset2 ...
			 *	b(?do) offset1 ... b(+loop) offset2 ...
			 *            ^                            ^
			 *           B1       ^            ^       T1
			 *                    T2           B2
			 *
			 * How we do this is we generate the b(do) or b(?do), spit out a
			 * zero offset while remembering b1 and t2.  Then we call tokenize()
			 * to generate the body.  When tokenize() finds a b(loop) or b(+loop),
			 * it generates the FCode and returns, with outpos at b2.  We then
			 * calculate the offsets, put them in the right slots and finishup.
			 */
			
			if (token->text[0] == '?')
				emit("b(?do)");
			else
				emit("b(do)");
			push(outpos);
			if (offsetsize == 16) {
				spit(0);
			}
			spit(0);	/* Place holder for later */
			push(outpos);
			break;
		case TOK_ELSE:
			STATE(token->text, "TOK_ELSE");
			/* Get where we need to patch */
			off = pop();
			emit("bbranch");
			/* Save where we are now. */
			push(outpos);
			if (offsetsize == 16) {
				spit(0);	/* Place holder for later */
			}
			spit(0);	/* Place holder for later */
			emit("b(>resolve)");
			/* Rewind and patch the if branch */
			pos = outpos;
			outpos = off;
			off = pos - off;
			if (offsetsize == 16) {
				spit(0);	/* Place holder for later */
			}
			spit(0);	/* Place holder for later */
			/* revert to the end */
			outpos = pos;
			break;
		case TOK_ENDCASE: 
			STATE(token->text, "TOK_ENDCASE:");
			pos = outpos; /* Remember where we need to branch to */

			/* Thread our way backwards and install proper offsets */
			off = pop();
			while (off) {
				int tmp;

				/* Move to this offset */
				outpos = off;
				/* Load next offset to process */
				tmp = outbuf[outpos];

				/* process this offset */
				off = pos - outpos;
				if (offsetsize == 16) {
					spit((off>>8)&0xff);
				}
				spit(off&0xff);
				off = tmp;
			}
			outpos = pos;
			emit("b(endcase)");
			break;
		case TOK_ENDOF:
			STATE(token->text, "TOK_ENDOF");
			off = pop();
			emit("b(endof)");
			/* 
			 * Save back pointer in the offset field so we can traverse
			 * the linked list and patch it in the endcase.
			 */
			pos = pop();	/* get position of prev link. */
			push(outpos);	/* save position of this link. */
			spit(pos);	/* save potision of prev link. */
			if (offsetsize == 16) {
				spit(0);
			}
			pos = outpos;
			/* Now point the offset from b(of) here. */
			outpos = off;
			off = outpos - off;
			if (offsetsize == 16) {
				spit((off>>8)&0xff);
			}
			spit(off&0xff);
			/* Restore position */
			outpos = pos;
			break;
		case TOK_EXTERNAL:
			STATE(token->text, "TOK_EXTERNAL");
			state = TOK_EXTERNAL;
			break;
		case TOK_FIELD:
			STATE(token->text, "TOK_FIELD");
	
			token = yylex();
			if (token == NULL) {
				(void)printf( "EOF in field definition\n");
				return;
			}
			
			/* Add new code to dictionary */
			fcode = malloc(sizeof(*fcode));
			fcode->num = nextfcode++;
			fcode->name = strdup(token->text);
			fadd(dictionary, fcode);
			
			if (state == 0)
				emit("new-token");
			else {
				if (state == TOK_EXTERNAL)
					emit("external-token");
				else
				/* Here we have a choice of new-token or named-token */
					emit("named-token");
				sspit(token->text);
			}
			spit(fcode->num);
			emit("b(field)");
			break;
		
		case TOK_HEX:
			STATE(token->text, "TOK_HEX");
			if (token->text[1] != '#') {
				if (defining) {
					spit(16);
					emit("base");
					emit("!");
				} else
					numbase = TOK_HEX;
			} else {
				char *end;
				Cell value;

				token = yylex();
				if (token == NULL) {
					(void)printf( "EOF after h#\n");
					return;
				}
				value = strtol(token->text, &end, 16);
				if (*end != 0) {
					(void)printf("Illegal number conversion:%s:%d: %s\n",
					    infile, yylineno, yytext);
					exit(1);
				}
				/* 
				 * If this is a 64-bit value we need to store two literals
				 * and issue a `lxjoin' to combine them.  But that's a future
				 * project.
				 */
				emit("b(lit)");
				spit((value>>24)&0x0ff);
				spit((value>>16)&0x0ff);
				spit((value>>8)&0x0ff);
				spit(value&0x0ff);
				if ((value>>32) != value && (value>>32) != 0) {
					emit("b(lit)");
					spit((value>>56)&0x0ff);
					spit((value>>48)&0x0ff);
					spit((value>>40)&0x0ff);
					spit((value>>32)&0x0ff);
					emit("lxjoin");
				}
			}
			break;
		case TOK_HEADERLESS:
			STATE(token->text, "TOK_HEADERLESS");
			state = 0;
			break;
		case TOK_HEADERS:
			STATE(token->text, "TOK_HEADERS");
			state = TOK_HEADERS;
			break;
		case TOK_OFFSET16:
			STATE(token->text, "TOK_OFFSET16");
			offsetsize = 16;
			emit("offset16");
			break;
		case TOK_IF:
			STATE(token->text, "TOK_IF");
			/*
			 * Similar to do but simpler since we only deal w/one branch.
			 */
			emit("b?branch");
			push(outpos);
			if (offsetsize == 16) {
				spit(0);	/* Place holder for later */
			}
			spit(0);	/* Place holder for later */
			break;
		case TOK_LEAVE:
			STATE(token->text, "TOK_LEAVE");
			emit("b(leave)");
			break;
		case TOK_LOOP:
			STATE(token->text, "TOK_LOOP");

			if (token->text[0] == '+')
				emit("b(+loop)");
			else
				emit("b(loop)");
			/* First do backwards branch of loop */
			pos = pop();
			off = pos - outpos;
			if (offsetsize == 16) {
				spit((off>>8)&0xff);
			}
			spit(off&0xff);
			/* Now do forward branch of do */
			pos = outpos;
			outpos = pop();
			off = pos - outpos;
			if (offsetsize == 16) {
				spit((off>>8)&0xff);
			}
			spit(off&0xff);
			/* Restore output position */
			outpos = pos;
			break;
		case TOK_OCTAL:
			STATE(token->text, "TOK_OCTAL");
			if (token->text[1] != '#') {
				if (defining) {
					spit(16);
					emit("base");
					emit("!");
				} else
					numbase = TOK_OCTAL;
			} else {
				char *end;
				Cell value;

				token = yylex();
				if (token == NULL) {
					(void)printf( "EOF after o#\n");
					return;
				}
				value = strtol(token->text, &end, 8);
				if (*end != 0) {
					(void)printf("Illegal number conversion:%s:%d: %s\n",
					    infile, yylineno, yytext);
					exit(1);
				}
				/* 
				 * If this is a 64-bit value we need to store two literals
				 * and issue a `lxjoin' to combine them.  But that's a future
				 * project.
				 */
				emit("b(lit)");
				spit((value>>24)&0x0ff);
				spit((value>>16)&0x0ff);
				spit((value>>8)&0x0ff);
				spit(value&0x0ff);
				if ((value>>32) != value && (value>>32) != 0) {
					emit("b(lit)");
					spit((value>>56)&0x0ff);
					spit((value>>48)&0x0ff);
					spit((value>>40)&0x0ff);
					spit((value>>32)&0x0ff);
					emit("lxjoin");
				}
			}
			break;
		case TOK_OF:
			STATE(token->text, "TOK_OF");
			/*
			 * Let's hope I get the semantics right.
			 *
			 * The `of' behaves almost the same as an
			 * `if'.  The difference is that `endof'
			 * takes a branch offset to the associated
			 * `endcase'.  Here we will generate a temporary
			 * offset of the `of' associated with the `endof'.
			 * Then in `endcase' we should be pointing just
			 * after the offset of the last `endof' so we 
			 * calculate the offset and thread our way backwards
			 * searching for the previous `b(case)' or `b(endof)'.
			 */
			emit("b(of)");
			push(outpos);
			if (offsetsize == 16) {
				spit(0);
			}
			spit(0);	/* Place holder for later */
			break;
		case TOK_REPEAT:
			STATE(token->text, "TOK_REPEAT");
			emit("bbranch");
			pos = pop();
			off = pop();
			/* First the offset for the branch back to the begin */
			off -= outpos;
			if (offsetsize == 16) {
				spit((off>>8)&0xff);
			}
			spit(off&0xff);
			emit("b(>resolve)");
			/* Now point the offset of the while here. */
			off = outpos;
			outpos = pos;
			pos = off - pos;
			if (offsetsize == 16) {
				spit((pos>>8)&0xff);
			}
			spit(pos&0xff);
			/* Return to the end of the output */
			outpos = off;
			break;
		case TOK_THEN:
			STATE(token->text, "TOK_THEN");
			emit("b(>resolve)");
			pos = outpos;
			outpos = pop();
			off = pos - outpos;
			if (offsetsize == 16) {
				spit((off>>8)&0xff);
			}
			spit(off&0xff);
			outpos = pos;
			break;
		case TOK_TO:
			STATE(token->text, "TOK_TO");
			/* The next pass should tokenize the FCODE number */
			emit("b(to)");
			break;
		case TOK_UNTIL:
			STATE(token->text, "TOK_UNTIL");
			emit("b?branch");
			pos = pop();
			pos -= outpos;
			if (offsetsize == 16) {
				spit((pos>>8)&0xff);
			}
			spit(pos&0xff);
			break;
		case TOK_VALUE:
			STATE(token->text, "TOK_VALUE");

			token = yylex();
			if (token == NULL) {
				(void)printf( "EOF in value definition\n");
				return;
			}
			
			/* Add new code to dictionary */
			fcode = malloc(sizeof(*fcode));
			fcode->num = nextfcode++;
			fcode->name = strdup(token->text);
			fadd(dictionary, fcode);
			
			if (state == 0)
				emit("new-token");
			else {
				if (state == TOK_EXTERNAL)
					emit("external-token");
				else
				/* Here we have a choice of new-token or named-token */
					emit("named-token");
				sspit(token->text);
			}
			spit(fcode->num);
			emit("b(value)");
			break;
		case TOK_VARIABLE:
			STATE(token->text, "TOK_VARIABLE");

			token = yylex();
			if (token == NULL) {
				(void)printf( "EOF in variable definition\n");
				return;
			}
			
			/* Add new code to dictionary */
			fcode = malloc(sizeof(*fcode));
			fcode->num = nextfcode++;
			fcode->name = strdup(token->text);
			fadd(dictionary, fcode);
			
			if (state == 0)
				emit("new-token");
			else {
				if (state == TOK_EXTERNAL)
					emit("external-token");
				else
				/* Here we have a choice of new-token or named-token */
					emit("named-token");
				sspit(token->text);
			}
			spit(fcode->num);
			emit("b(variable)");
			break;
		case TOK_WHILE:
			STATE(token->text, "TOK_WHILE");
			emit("b?branch");
			push(outpos);
			if (offsetsize == 16) {
				spit(0);
			}
			spit(0);
			break;

			/* Tokenizer directives */
		case TOK_BEGTOK:
			STATE(token->text, "TOK_BEGTOK");
			tokenizer = 1;
			break;
		case TOK_EMIT_BYTE:
			STATE(token->text, "TOK_EMIT_BYTE");
			spit(pop());
			break;
		case TOK_ENDTOK:
			STATE(token->text, "TOK_ENDTOK");
			tokenizer = 0;
			break;
		case TOK_FLOAD:
			STATE(token->text, "TOK_FLOAD");
			/* Parse a different file for a while */
			token = yylex();
			if ((inf = fopen(token->text, "r")) == NULL) {
				(void)printf("%s: Could not open %s: %s\n",
					      myname, token->text, strerror(errno));
				break;
			}
			inbuf = yy_create_buffer(inf, YY_BUF_SIZE);
			yy_switch_to_buffer(inbuf);
			{
				char *oldinfile = infile;

				infile = token->text;
				tokenize(inbuf);
				infile = oldinfile;
			}
			yy_switch_to_buffer(yinput);
			yy_delete_buffer(inbuf);
			fclose(inf);
			break;
		case TOK_OTHER:
			STATE(token->text, "TOK_OTHER");
			if (apply_macros(yinput, token->text))
				break;
			if (emit(token->text)) {
#if 0
				/*
				 * Call an external command 
				 *
				 * XXXXX assumes it will always find the command
				 */
				sspit(token->text);
				emit("$find");
				emit("drop");
				emit("execute");
#else
				(void)printf( "%s: undefined token `%s'\n",
					      myname, token->text);
				fflush(stderr);
				exit(1);
#endif
			}
			break;
		default:
			/* Nothing */ ;
		}
	}
	return;
}

/*
 * print a tokenizer error message
 */
void
token_err(int lineno, const char *file, const char *text, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	if (file)
		(void)fprintf(stderr, "%s:%d: ", file, lineno);
	if (fmt)
		(void)vfprintf(stderr, fmt, ap);
	fputc('\n', stderr);
	if (text)
		fprintf(stderr, "\t%s", text);
	va_end(ap);
	exit(1);
}

/*
 * Lookup fcode string in dictionary and spit it out.
 *
 * Fcode must be in dictionary.  No alias conversion done.
 */
int
emit(const char *str)
{
	struct fcode *code;
	if ((code = flookup( dictionary, str)))
		spit(code->num);
#ifdef DEBUG
	if (debug > 1) {
		if (code)
			(void)printf( "emitting `%s'\n", code->name);
		else
			(void)printf( "emit: not found `%s'\n", str);
	}
#endif
	return (code == NULL);
}

/*
 * Spit out an integral value as a series of FCodes.
 *
 * It will spit out one zero byte or as many bytes as are
 * non-zero.
 */
int
spit(long n)
{
	int count = 1;

	if (n >> 8)
		count += spit(n >> 8);
	if ((size_t)outpos >= outbufsiz) {
		while ((size_t)outpos >= outbufsiz) outbufsiz += BUFCLICK;
		if (!(outbuf = realloc(outbuf, outbufsiz))) {
			(void)printf( "realloc of %ld bytes failed -- out of memory\n",
				      (long)outbufsiz);
			exit(1);
		}
	}
	if (debug > 1) printf("spitting %2.2x\n", (unsigned char)n);
	outbuf[outpos++] = n;
	return (count);
}

/*
 * Spit out an FCode string.
 */
void
sspit(const char *s)
{
	int len = strlen(s);

	if (len > 255) {
		(void)printf( "string length %d too long\n", len);
		return;
	}
#ifdef DEBUG
	if (debug > 1)
		(void)printf( "sspit: len %d str `%s'\n", len, s);
#endif
	spit(len);
	while (*s)
		spit(*s++);
}

int 
yywrap(void)
{
	/* Always generate EOF */
	return (1);
}
