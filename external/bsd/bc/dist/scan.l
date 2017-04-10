/*	$NetBSD: scan.l,v 1.1.2.2 2017/04/10 02:28:24 phil Exp $ */

/*
 * Copyright (C) 1991-1994, 1997, 2006, 2008, 2012-2017 Free Software Foundation, Inc.
 * Copyright (C) 2016-2017 Philip A. Nelson.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The names Philip A. Nelson and Free Software Foundation may not be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY PHILIP A. NELSON ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL PHILIP A. NELSON OR THE FREE SOFTWARE FOUNDATION BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/* scan.l: the (f)lex description file for the scanner. */

%{

#include "bcdefs.h"
#include "bc.h"
#include "global.h"
#include "proto.h"
#include <errno.h>

/* Using flex, we can ask for a smaller input buffer.  With lex, this
   does nothing! */

#ifdef SMALL_BUF
#undef YY_READ_BUF_SIZE
#define YY_READ_BUF_SIZE 512
#endif

/* Force . as last for now. */
#define DOT_IS_LAST

/* We want to define our own yywrap. */
#undef yywrap
int yywrap (void);

#if defined(LIBEDIT)
/* Support for the BSD libedit with history for
   nicer input on the interactive part of input. */

#include <histedit.h>

/* Have input call the following function. */
#undef  YY_INPUT
#define YY_INPUT(buf,result,max_size) \
		bcel_input((char *)buf, (yy_size_t *)&result, max_size)

/* Variables to help interface editline with bc. */
static const char *bcel_line = (char *)NULL;
static int   bcel_len = 0;

/* bcel_input puts upto MAX characters into BUF with the number put in
   BUF placed in *RESULT.  If the yy input file is the same as
   stdin, use editline.  Otherwise, just read it.
*/

static void
bcel_input (char *buf, yy_size_t  *result, int max)
{
  ssize_t rdsize;
  if (!edit || yyin != stdin)
    {
      while ( (rdsize = read( fileno(yyin), buf, max )) < 0 )
        if (errno != EINTR)
	  {
	    yyerror( "read() in flex scanner failed" );
	    bc_exit (1);
	  }
      *result = (yy_size_t) rdsize;
      return;
    }

  /* Do we need a new string? */
  if (bcel_len == 0)
    {
      bcel_line = el_gets(edit, &bcel_len);
      if (bcel_line == NULL) {
	/* end of file */
	*result = 0;
	bcel_len = 0;
	return;
      }
      if (bcel_len != 0)
	history (hist, &histev, H_ENTER, bcel_line); 
      fflush (stdout);
    }

  if (bcel_len <= max)
    {
      strncpy (buf, bcel_line, bcel_len);
      *result = bcel_len;
      bcel_len = 0;
    }
  else
    {
      strncpy (buf, bcel_line, max);
      *result = max;
      bcel_line += max;
      bcel_len -= max;
    }
}
#endif

#ifdef READLINE
/* Support for the readline and history libraries.  This allows
   nicer input on the interactive part of input. */

/* Have input call the following function. */
#undef  YY_INPUT
#define YY_INPUT(buf,result,max_size) \
		rl_input((char *)buf, &result, max_size)

/* Variables to help interface readline with bc. */
static char *rl_line = (char *)NULL;
static char *rl_start = (char *)NULL;
static int   rl_len = 0;

/* Definitions for readline access. */
extern FILE *rl_instream;

/* rl_input puts upto MAX characters into BUF with the number put in
   BUF placed in *RESULT.  If the yy input file is the same as
   rl_instream (stdin), use readline.  Otherwise, just read it.
*/

static void
rl_input (char *buf, int *result, int max)
{
  if (yyin != rl_instream)
    {
      while ( (*result = read( fileno(yyin), buf, max )) < 0 )
        if (errno != EINTR)
	  {
	    yyerror( "read() in flex scanner failed" );
	    bc_exit (1);
	  }
      return;
    }

  /* Do we need a new string? */
  if (rl_len == 0)
    {
      if (rl_start)
	free(rl_start);
      rl_start = readline ("");
      if (rl_start == NULL) {
	/* end of file */
	*result = 0;
	rl_len = 0;
	return;
      }
      rl_line = rl_start;
      rl_len = strlen (rl_line)+1;
      if (rl_len != 1)
	add_history (rl_line); 
      rl_line[rl_len-1] = '\n';
      fflush (stdout);
    }

  if (rl_len <= max)
    {
      strncpy (buf, rl_line, rl_len);
      *result = rl_len;
      rl_len = 0;
    }
  else
    {
      strncpy (buf, rl_line, max);
      *result = max;
      rl_line += max;
      rl_len -= max;
    }
}
#endif

#if !defined(READLINE) && !defined(LIBEDIT)

/* MINIX returns from read with < 0 if SIGINT is  encountered.
   In flex, we can redefine YY_INPUT to the following.  In lex, this
   does nothing! */
#undef  YY_INPUT
#define YY_INPUT(buf,result,max_size) \
	while ( (result = read( fileno(yyin), (char *) buf, max_size )) < 0 ) \
	    if (errno != EINTR) \
		YY_FATAL_ERROR( "read() in flex scanner failed" );
#endif

%}
DIGIT [0-9A-Z]
LETTER [a-z]
%s slcomment
%%
"#"		{
 		  if (!std_only)
		    BEGIN(slcomment);
 		  else
		    yyerror ("illegal character: #");
		}
<slcomment>[^\n]* { BEGIN(INITIAL); }
<slcomment>"\n" { line_no++; BEGIN(INITIAL); return(ENDOFLINE); }
define return(Define);
break  return(Break);
quit   return(Quit);
length return(Length);
return return(Return);
for    return(For);
if     return(If);
while  return(While);
sqrt   return(Sqrt);
scale  return(Scale);
ibase  return(Ibase);
obase  return(Obase);
auto   return(Auto);
else   return(Else);
read   return(Read);
random return(Random);
halt   return(Halt);
last   return(Last);
void   return(Void); 
history {
#if defined(READLINE) || defined(LIBEDIT)
	  return(HistoryVar);
#else
	  yylval.s_value = strcopyof(yytext); return(NAME);
#endif
	}

warranty return(Warranty);
continue return(Continue);
print  return(Print);
limits return(Limits);
"." {
#ifdef DOT_IS_LAST
       return(Last);
#else
       yyerror ("illegal character: %s",yytext);
#endif
    }
"+"|"-"|";"|"("|")"|"{"|"}"|"["|"]"|","|"^" { yylval.c_value = yytext[0]; 
					      return((int)yytext[0]); }
&& { return(AND); }
\|\| { return(OR); }
"!" { return(NOT); }
"*"|"/"|"%"|"&" { yylval.c_value = yytext[0]; return((int)yytext[0]); }
"="|\+=|-=|\*=|\/=|%=|\^=  { yylval.c_value = yytext[0]; return(ASSIGN_OP); }
=\+|=-|=\*|=\/|=%|=\^  { 
#ifdef OLD_EQ_OP
			 char warn_save;
			 warn_save = warn_not_std;
			 warn_not_std = TRUE;
			 ct_warn ("Old fashioned =<op>");
			 warn_not_std = warn_save;
			 yylval.c_value = yytext[1];
#else
			 yylval.c_value = '=';
			 yyless (1);
#endif
			 return(ASSIGN_OP);
		       }
==|\<=|\>=|\!=|"<"|">" { yylval.s_value = strcopyof(yytext); return(REL_OP); }
\+\+|-- { yylval.c_value = yytext[0]; return(INCR_DECR); }
"\n" { line_no++; return(ENDOFLINE); }
\\\n {  line_no++;  /* ignore a "quoted" newline */ }
[ \t]+  { /* ignore spaces and tabs */ }
"/*"  {
	int c;

	for (;;)
	  {
	    while ( ((c=input()) != '*') && (c != EOF)) 
	      /* eat it */
	      if (c == '\n') line_no++;
	    if (c == '*')
 	      {
		while ( (c=input()) == '*') /* eat it*/;
		if (c == '/') break; /* at end of comment */
		if (c == '\n') line_no++;
	      }
	    if (c == EOF)
	      {
		fprintf (stderr,"EOF encountered in a comment.\n");
		break;
	      }
	  }
      }
[a-z][a-z0-9_]* { yylval.s_value = strcopyof(yytext); return(NAME); }
\"[^\"]*\"  {
 	      const char *look;
	      int count = 0;
	      yylval.s_value = strcopyof(yytext);
	      for (look = yytext; *look != 0; look++)
		{
		  if (*look == '\n') line_no++;
		  if (*look == '"')  count++;
		}
	      if (count != 2) yyerror ("NUL character in string.");
	      return(STRING);
	    }
{DIGIT}({DIGIT}|\\\n)*("."({DIGIT}|\\\n)*)?|"."(\\\n)*{DIGIT}({DIGIT}|\\\n)* {
	      char *src, *dst;
	      int len;
	      /* remove a trailing decimal point. */
	      len = strlen(yytext);
	      if (yytext[len-1] == '.')
	        yytext[len-1] = 0;
	      /* remove leading zeros. */
	      src = yytext;
	      dst = yytext;
	      while (*src == '0') src++;
	      if (*src == 0) src--;
	      /* Copy strings removing the newlines. */
	      while (*src != 0)
		{
	          if (*src == '\\')
		    {
		      src++; src++;
		      line_no++;
		    }
		  if (*src == ',')
		    {
		      src++;
		      ct_warn("Commas in numbers");
		    }		    
		  else
		    *dst++ = *src++;
	        }
	      *dst = 0;
	      yylval.s_value = strcopyof(yytext); 
	      return(NUMBER);
	    }
.       {
	  if (yytext[0] < ' ')
	    yyerror ("illegal character: ^%c",yytext[0] + '@');
	  else
	    if (yytext[0] > '~')
	      yyerror ("illegal character: \\%03o", (int) yytext[0]);
	    else
	      yyerror ("illegal character: %s",yytext);
	}
%%



/* This is the way to get multiple files input into lex. */

int
yywrap(void)
{
  if (!open_new_file ()) return (1);	/* EOF on standard in. */
  return (0);          			/* We have more input. */
  yyunput(0,NULL);	/* Make sure the compiler think yyunput is used. */
}
