/*
 * Copyright (c) Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
 */

%option c++
/* %option prefix="Elftosb" */
%option yylineno
%option never-interactive
%option yyclass="ElftosbLexer"
%option noyywrap

%{
#include "ElftosbLexer.h"
#include <stdlib.h>
#include <limits.h>
#include <string>
#include "HexValues.h"
#include "Value.h"

using namespace elftosb;

//! Always executed before all other actions when a token is matched.
//! This action just assign the first and last lines of the token to
//! the current line. In most cases this is correct.
#define YY_USER_ACTION	do {									\
							m_location.m_firstLine = m_line;		\
							m_location.m_lastLine = m_line;		\
						} while (0);

%}

DIGIT		[0-9]
HEXDIGIT	[0-9a-fA-F]
BINDIGIT	[0-1]
IDENT		[a-zA-Z_][a-zA-Z0-9_]*
ESC			\\(x{HEXDIGIT}{2}|.)

/* start conditions */
%x blob mlcmt

%%

options			{ return TOK_OPTIONS; }
constants		{ return TOK_CONSTANTS; }
sources			{ return TOK_SOURCES; }
filters			{ return TOK_FILTERS; }
section			{ return TOK_SECTION; }
extern			{ return TOK_EXTERN; }
from			{ return TOK_FROM; }
raw				{ return TOK_RAW; }
load			{ return TOK_LOAD; }
jump			{ return TOK_JUMP; }
call			{ return TOK_CALL; }
mode			{ return TOK_MODE; }
if				{ return TOK_IF; }
else			{ return TOK_ELSE; }
defined			{ return TOK_DEFINED; }
info			{ return TOK_INFO; }
warning			{ return TOK_WARNING; }
error			{ return TOK_ERROR; }
sizeof			{ return TOK_SIZEOF; }
dcd				{ return TOK_DCD; }
hab				{ return TOK_HAB; }
ivt             { return TOK_IVT; }

[whb]/[^a-zA-Z_0-9]					{	// must be followed by any non-ident char
										int_size_t theSize;
										switch (yytext[0])
										{
											case 'w':
												theSize = kWordSize;
												break;
											case 'h':
												theSize = kHalfWordSize;
												break;
											case 'b':
												theSize = kByteSize;
												break;
										}
										m_symbolValue.m_int = new elftosb::SizedIntegerValue(0, theSize);
										return TOK_INT_SIZE;
									}
									
true|yes							{
										m_symbolValue.m_int = new elftosb::SizedIntegerValue(1, kWordSize);
										return TOK_INT_LITERAL;
									}

false|no							{
										m_symbolValue.m_int = new elftosb::SizedIntegerValue(0, kWordSize);
										return TOK_INT_LITERAL;
									}

{IDENT}								{
										m_symbolValue.m_str = new std::string(yytext);
										if (isSourceName(m_symbolValue.m_str))
										{
											return TOK_SOURCE_NAME;
										}
										else
										{
											return TOK_IDENT;
										}
									}

({DIGIT}+|0x{HEXDIGIT}+|0b{BINDIGIT}+)([ \t]*[GMK])?			{
										int base = 0;
										uint32_t value;
										int mult;
										
										// check for binary number
										if (yytext[0] == '0' && yytext[1] == 'b')
										{
											base = 2;		// this is a binary number
											yytext += 2;	// skip over the "0b"
										}
										
										// convert value
										value = (uint32_t)strtoul(yytext, NULL, base);
										
										// find multiplier
										switch (yytext[strlen(yytext) - 1])
										{
											case 'G':
												mult = 1024 * 1024 * 1024;
												break;
											case 'M':
												mult = 1024 * 1024;
												break;
											case 'K':
												mult = 1024;
												break;
											default:
												mult = 1;
												break;
										}
										
										// set resulting symbol value
										m_symbolValue.m_int = new elftosb::SizedIntegerValue(value * mult, kWordSize);
										return TOK_INT_LITERAL;
									}

\'(.|ESC)\'|\'(.|ESC){2}\'|\'(.|ESC){4}\'		{
										uint32_t value = 0;
										int_size_t theSize;
										int len = strlen(yytext);
										if (len >= 3)
										{
											value = yytext[1];
											theSize = kByteSize;
										}
										if (len >= 4)
										{
											value = (value << 8) | yytext[2];
											theSize = kHalfWordSize;
										}
										if (len >= 6)
										{
											value = (value << 8) | yytext[3];
											value = (value << 8) | yytext[4];
											theSize = kWordSize;
										}
										m_symbolValue.m_int = new elftosb::SizedIntegerValue(value, theSize);
										return TOK_INT_LITERAL;
									}

\$[\.\*a-zA-Z0-9_\[\]\^\?\-]+					{
										// remove $ from string
										m_symbolValue.m_str = new std::string(&yytext[1]);
										return TOK_SECTION_NAME;
									}


"/*"                                { BEGIN(mlcmt); }

"{{"								{
										m_blob = new Blob();
										m_blobFirstLine = yylineno;
										BEGIN(blob);
									}

"{"									{ return '{'; }

"}"									{ return '}'; }

"("									{ return '('; }

")"									{ return ')'; }

"["									{ return '['; }

"]"									{ return ']'; }

"="									{ return '='; }

","									{ return ','; }

":"									{ return ':'; }

";"									{ return ';'; }

"."									{ return '.'; }

">"									{ return '>'; }

".."								{ return TOK_DOT_DOT; }

"+"									{ return '+'; }

"-"									{ return '-'; }

"*"									{ return '*'; }

"/"									{ return '/'; }

"%"									{ return '%'; }

"~"									{ return '~'; }

"^"									{ return '^'; }

"<<"								{ return TOK_LSHIFT; }

">>"								{ return TOK_RSHIFT; }

"&"									{ return '&'; }

"|"									{ return '|'; }

"**"								{ return TOK_POWER; }

"<"									{ return '<'; }

">="								{ return TOK_GEQ; }

"<="								{ return TOK_LEQ; }

"=="								{ return TOK_EQ; }

"!="								{ return TOK_NEQ; }

"&&"								{ return TOK_AND; }

"||"								{ return TOK_OR; }

"!"									{ return '!'; }

\"(ESC|[^\"])*\"					{
										// get rid of quotes
										yytext++;
										yytext[strlen(yytext) - 1] = 0;
//										processStringEscapes(yytext, yytext);
										m_symbolValue.m_str = new std::string(yytext);
										return TOK_STRING_LITERAL;
									}

<blob>{HEXDIGIT}{2}					{
										uint8_t x = (hexCharToInt(yytext[0]) << 4) | hexCharToInt(yytext[1]);
										m_blob->append(&x, 1);
									}

<blob>"}}"							{
										BEGIN(INITIAL);
										m_symbolValue.m_blob = m_blob;
										m_blob = NULL;
										m_location.m_firstLine = m_blobFirstLine;
										return TOK_BLOB;
									}

<mlcmt>\*\/                         {
                                        // end of multi-line comment, return to initial state
                                        BEGIN(INITIAL);
                                    }


(#|\/\/).*$							/* absorb single-line comment */

<*>[ \t]							/* eat up whitespace in all states */

<*>(\r\n|\r|\n)						{
										/* eat up whitespace and count lines in all states */
										m_line++;
									}

<mlcmt>.                            /* ignore all other chars in a multi-line comment */

<*>.								{
										/* all other chars produce errors */
										char msg[50];
										sprintf(msg, "unexpected character '%c' on line %d", yytext[0], m_line);
										LexerError(msg);
									}

%%

// verbatim code copied to the bottom of the output


