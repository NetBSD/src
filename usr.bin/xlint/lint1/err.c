/*	$NetBSD: err.c,v 1.160 2022/04/16 13:25:27 rillig Exp $	*/

/*
 * Copyright (c) 1994, 1995 Jochen Pohl
 * All Rights Reserved.
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
 *      This product includes software developed by Jochen Pohl for
 *	The NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/cdefs.h>
#if defined(__RCSID) && !defined(lint)
__RCSID("$NetBSD: err.c,v 1.160 2022/04/16 13:25:27 rillig Exp $");
#endif

#include <sys/types.h>
#include <stdarg.h>
#include <stdlib.h>

#include "lint1.h"

/* number of errors found */
int	nerr;

/* number of syntax errors */
int	sytxerr;


const char *const msgs[] = {
	"empty declaration",					      /* 0 */
	"old style declaration; add 'int'",			      /* 1 */
	"empty declaration",					      /* 2 */
	"'%s' declared in argument declaration list",		      /* 3 */
	"illegal type combination",				      /* 4 */
	"modifying typedef with '%s'; only qualifiers allowed",	      /* 5 */
	"use 'double' instead of 'long float'",			      /* 6 */
	"only one storage class allowed",			      /* 7 */
	"illegal storage class",				      /* 8 */
	"only register valid as formal parameter storage class",      /* 9 */
	"duplicate '%s'",					      /* 10 */
	"bit-field initializer out of range",			      /* 11 */
	"compiler takes size of function",			      /* 12 */
	"incomplete enum type: %s",				      /* 13 */
	"",							      /* 14 */
	"function returns illegal type '%s'",			      /* 15 */
	"array of function is illegal",				      /* 16 */
	"null dimension",					      /* 17 */
	"illegal use of 'void'",				      /* 18 */
	"void type for '%s'",					      /* 19 */
	"negative array dimension (%d)",			      /* 20 */
	"redeclaration of formal parameter %s",			      /* 21 */
	"incomplete or misplaced function definition",		      /* 22 */
	"undefined label '%s'",					      /* 23 */
	"cannot initialize function: %s",			      /* 24 */
	"cannot initialize typedef: %s",			      /* 25 */
	"cannot initialize extern declaration: %s",		      /* 26 */
	"redeclaration of %s",					      /* 27 */
	"redefinition of %s",					      /* 28 */
	"previously declared extern, becomes static: %s",	      /* 29 */
	"redeclaration of %s; ANSI C requires static",		      /* 30 */
	"'%s' has incomplete type '%s'",			      /* 31 */
	"argument type defaults to 'int': %s",			      /* 32 */
	"duplicate member name: %s",				      /* 33 */
	"nonportable bit-field type '%s'",			      /* 34 */
	"illegal bit-field type '%s'",				      /* 35 */
	"illegal bit-field size: %d",				      /* 36 */
	"zero size bit-field",					      /* 37 */
	"function illegal in structure or union",		      /* 38 */
	"zero sized array in struct is a C99 extension: %s",	      /* 39 */
	"",			/* never used */		      /* 40 */
	"bit-field in union is very unusual",			      /* 41 */
	"forward reference to enum type",			      /* 42 */
	"redefinition hides earlier one: %s",			      /* 43 */
	"declaration introduces new type in ANSI C: %s %s",	      /* 44 */
	"base type is really '%s %s'",				      /* 45 */
	"%s tag '%s' redeclared as %s",				      /* 46 */
	"zero sized %s is a C99 feature",			      /* 47 */
	"overflow in enumeration values: %s",			      /* 48 */
	"anonymous struct/union members is a C11 feature",	      /* 49 */
	"a function is declared as an argument: %s",		      /* 50 */
	"parameter mismatch: %d declared, %d defined",		      /* 51 */
	"cannot initialize parameter: %s",			      /* 52 */
	"declared argument %s is missing",			      /* 53 */
	"trailing ',' prohibited in enum declaration",		      /* 54 */
	"integral constant expression expected",		      /* 55 */
	"integral constant too large",				      /* 56 */
	"enumeration constant hides parameter: %s",		      /* 57 */
	"type does not match prototype: %s",			      /* 58 */
	"formal parameter lacks name: param #%d",		      /* 59 */
	"void must be sole parameter",				      /* 60 */
	"void parameter cannot have name: %s",			      /* 61 */
	"function prototype parameters must have types",	      /* 62 */
	"prototype does not match old-style definition",	      /* 63 */
	"()-less function definition",				      /* 64 */
	"%s has no named members",				      /* 65 */
	"",							      /* 66 */
	"cannot return incomplete type",			      /* 67 */
	"typedef already qualified with '%s'",			      /* 68 */
	"inappropriate qualifiers with 'void'",			      /* 69 */
	"",			/* unused */			      /* 70 */
	"too many characters in character constant",		      /* 71 */
	"typedef declares no type name",			      /* 72 */
	"empty character constant",				      /* 73 */
	"no hex digits follow \\x",				      /* 74 */
	"overflow in hex escape",				      /* 75 */
	"character escape does not fit in character",		      /* 76 */
	"bad octal digit %c",					      /* 77 */
	"",			/* unused */			      /* 78 */
	"dubious escape \\%c",					      /* 79 */
	"dubious escape \\%o",					      /* 80 */
	"\\a undefined in traditional C",			      /* 81 */
	"\\x undefined in traditional C",			      /* 82 */
	"storage class after type is obsolescent",		      /* 83 */
	"ANSI C requires formal parameter before '...'",	      /* 84 */
	"dubious tag declaration: %s %s",			      /* 85 */
	"automatic hides external declaration: %s",		      /* 86 */
	"static hides external declaration: %s",		      /* 87 */
	"typedef hides external declaration: %s",		      /* 88 */
	"typedef redeclared: %s",				      /* 89 */
	"inconsistent redeclaration of extern: %s",		      /* 90 */
	"declaration hides parameter: %s",			      /* 91 */
	"inconsistent redeclaration of static: %s",		      /* 92 */
	"dubious static function at block level: %s",		      /* 93 */
	"function has illegal storage class: %s",		      /* 94 */
	"declaration hides earlier one: %s",			      /* 95 */
	"cannot dereference non-pointer type",			      /* 96 */
	"suffix U is illegal in traditional C",			      /* 97 */
	"suffixes F and L are illegal in traditional C",	      /* 98 */
	"'%s' undefined",					      /* 99 */
	"unary + is illegal in traditional C",			      /* 100 */
	"type '%s' does not have member '%s'",			      /* 101 */
	"illegal member use: %s",				      /* 102 */
	"left operand of '.' must be struct or union, not '%s'",      /* 103 */
	"left operand of '->' must be pointer to struct or union, not '%s'", /* 104 */
	"non-unique member requires struct/union %s",		      /* 105 */
	"left operand of '->' must be pointer",			      /* 106 */
	"operands of '%s' have incompatible types (%s != %s)",	      /* 107 */
	"operand of '%s' has invalid type (%s)",		      /* 108 */
	"void type illegal in expression",			      /* 109 */
	"pointer to function is not allowed here",		      /* 110 */
	"unacceptable operand of '%s'",				      /* 111 */
	"cannot take address of bit-field",			      /* 112 */
	"cannot take address of register %s",			      /* 113 */
	"%soperand of '%s' must be lvalue",			      /* 114 */
	"%soperand of '%s' must be modifiable lvalue",		      /* 115 */
	"illegal pointer subtraction",				      /* 116 */
	"bitwise '%s' on signed value possibly nonportable",	      /* 117 */
	"semantics of '%s' change in ANSI C; use explicit cast",      /* 118 */
	"conversion of '%s' to '%s' is out of range",		      /* 119 */
	"bitwise '%s' on signed value nonportable",		      /* 120 */
	"negative shift",					      /* 121 */
	"shift amount %llu is greater than bit-size %llu of '%s'",    /* 122 */
	"illegal combination of %s '%s' and %s '%s', op '%s'",	      /* 123 */
	"illegal combination of '%s' and '%s', op '%s'",	      /* 124 */
	"ANSI C forbids ordered comparisons of pointers to functions",/* 125 */
	"incompatible types '%s' and '%s' in conditional",	      /* 126 */
	"'&' before array or function: ignored",		      /* 127 */
	"operands have incompatible pointer types, op %s (%s != %s)", /* 128 */
	"expression has null effect",				      /* 129 */
	"enum type mismatch: '%s' '%s' '%s'",			      /* 130 */
	"conversion to '%s' may sign-extend incorrectly",	      /* 131 */
	"conversion from '%s' to '%s' may lose accuracy",	      /* 132 */
	"conversion of pointer to '%s' loses bits",		      /* 133 */
	"conversion of pointer to '%s' may lose bits",		      /* 134 */
	"converting '%s' to '%s' may cause alignment problem",	      /* 135 */
	"cannot do pointer arithmetic on operand of unknown size",    /* 136 */
	"use of incomplete enum type, op %s",			      /* 137 */
	"unknown operand size, op %s",				      /* 138 */
	"division by 0",					      /* 139 */
	"modulus by 0",						      /* 140 */
	"integer overflow detected, op '%s'",			      /* 141 */
	"floating point overflow detected, op %s",		      /* 142 */
	"cannot take size/alignment of incomplete type",	      /* 143 */
	"cannot take size/alignment of function type '%s'",	      /* 144 */
	"cannot take size/alignment of bit-field",		      /* 145 */
	"cannot take size/alignment of void",			      /* 146 */
	"invalid cast from '%s' to '%s'",			      /* 147 */
	"improper cast of void expression",			      /* 148 */
	"illegal function (type %s)",				      /* 149 */
	"argument mismatch: %d arg%s passed, %d expected",	      /* 150 */
	"void expressions may not be arguments, arg #%d",	      /* 151 */
	"argument cannot have unknown size, arg #%d",		      /* 152 */
	"converting '%s' to incompatible '%s' for argument %d",	      /* 153 */
	"illegal combination of %s (%s) and %s (%s), arg #%d",	      /* 154 */
	"passing '%s' to incompatible '%s', arg #%d",		      /* 155 */
	"enum type mismatch, arg #%d (%s != %s)",		      /* 156 */
	"ANSI C treats constant as unsigned",			      /* 157 */
	"%s may be used before set",				      /* 158 */
	"assignment in conditional context",			      /* 159 */
	"operator '==' found where '=' was expected",		      /* 160 */
	"constant in conditional context",			      /* 161 */
	"comparison of %s with %s, op %s",			      /* 162 */
	"a cast does not yield an lvalue",			      /* 163 */
	"assignment of negative constant to unsigned type",	      /* 164 */
	"constant truncated by assignment",			      /* 165 */
	"precision lost in bit-field assignment",		      /* 166 */
	"array subscript cannot be negative: %ld",		      /* 167 */
	"array subscript cannot be > %d: %ld",			      /* 168 */
	"precedence confusion possible: parenthesize!",		      /* 169 */
	"first operand must have scalar type, op ? :",		      /* 170 */
	"cannot assign to '%s' from '%s'",			      /* 171 */
	"too many struct/union initializers",			      /* 172 */
	"too many array initializers, expected %d",		      /* 173 */
	"too many initializers",				      /* 174 */
	"initialization of incomplete type '%s'",		      /* 175 */
	"",			/* no longer used */		      /* 176 */
	"non-constant initializer",				      /* 177 */
	"initializer does not fit",				      /* 178 */
	"cannot initialize struct/union with no named member",	      /* 179 */
	"bit-field initializer does not fit",			      /* 180 */
	"{}-enclosed initializer required",			      /* 181 */
	"incompatible pointer types (%s != %s)",		      /* 182 */
	"illegal combination of %s (%s) and %s (%s)",		      /* 183 */
	"illegal combination of '%s' and '%s'",			      /* 184 */
	"cannot initialize '%s' from '%s'",			      /* 185 */
	"bit-field initialization is illegal in traditional C",	      /* 186 */
	"string literal too long (%lu) for target array (%lu)",	      /* 187 */
	"no automatic aggregate initialization in traditional C",     /* 188 */
	"",			/* no longer used */		      /* 189 */
	"empty array declaration: %s",				      /* 190 */
	"'%s' set but not used in function '%s'",		      /* 191 */
	"'%s' unused in function '%s'",				      /* 192 */
	"statement not reached",				      /* 193 */
	"label %s redefined",					      /* 194 */
	"case not in switch",					      /* 195 */
	"case label affected by conversion",			      /* 196 */
	"non-constant case expression",				      /* 197 */
	"non-integral case expression",				      /* 198 */
	"duplicate case in switch: %ld",			      /* 199 */
	"duplicate case in switch: %lu",			      /* 200 */
	"default outside switch",				      /* 201 */
	"duplicate default in switch",				      /* 202 */
	"case label must be of type 'int' in traditional C",	      /* 203 */
	"controlling expressions must have scalar type",	      /* 204 */
	"switch expression must have integral type",		      /* 205 */
	"enumeration value(s) not handled in switch",		      /* 206 */
	"loop not entered at top",				      /* 207 */
	"break outside loop or switch",				      /* 208 */
	"continue outside loop",				      /* 209 */
	"enum type mismatch between '%s' and '%s' in initialization", /* 210 */
	"return value type mismatch (%s) and (%s)",		      /* 211 */
	"cannot return incomplete type",			      /* 212 */
	"void function %s cannot return value",			      /* 213 */
	"function '%s' expects to return value",		      /* 214 */
	"function '%s' implicitly declared to return int",	      /* 215 */
	"function %s has return (e); and return;",		      /* 216 */
	"function %s falls off bottom without returning value",	      /* 217 */
	"ANSI C treats constant as unsigned, op %s",		      /* 218 */
	"concatenated strings are illegal in traditional C",	      /* 219 */
	"fallthrough on case statement",			      /* 220 */
	"initialization of unsigned with negative constant",	      /* 221 */
	"conversion of negative constant to unsigned type",	      /* 222 */
	"end-of-loop code not reached",				      /* 223 */
	"cannot recover from previous errors",			      /* 224 */
	"static function called but not defined: %s()",		      /* 225 */
	"static variable %s unused",				      /* 226 */
	"const object %s should have initializer",		      /* 227 */
	"function cannot return const or volatile object",	      /* 228 */
	"converting '%s' to '%s' is questionable",		      /* 229 */
	"nonportable character comparison '%s %d'",		      /* 230 */
	"argument '%s' unused in function '%s'",		      /* 231 */
	"label '%s' unused in function '%s'",			      /* 232 */
	"struct %s never defined",				      /* 233 */
	"union %s never defined",				      /* 234 */
	"enum %s never defined",				      /* 235 */
	"static function %s unused",				      /* 236 */
	"redeclaration of formal parameter %s",			      /* 237 */
	"initialization of union is illegal in traditional C",	      /* 238 */
	"constant argument to '!'",				      /* 239 */
	"assignment of different structures (%s != %s)",	      /* 240 */
	"dubious operation on enum, op %s",			      /* 241 */
	"combination of '%s' and '%s', op %s",			      /* 242 */
	"dubious comparison of enums, op %s",			      /* 243 */
	"illegal structure pointer combination",		      /* 244 */
	"incompatible structure pointers: '%s' '%s' '%s'",	      /* 245 */
	"dubious conversion of enum to '%s'",			      /* 246 */
	"pointer cast from '%s' to '%s' may be troublesome",	      /* 247 */
	"floating-point constant out of range",			      /* 248 */
	"syntax error '%s'",					      /* 249 */
	"unknown character \\%o",				      /* 250 */
	"malformed integer constant",				      /* 251 */
	"integer constant out of range",			      /* 252 */
	"unterminated character constant",			      /* 253 */
	"newline in string or char constant",			      /* 254 */
	"undefined or invalid # directive",			      /* 255 */
	"unterminated comment",					      /* 256 */
	"extra characters in lint comment",			      /* 257 */
	"unterminated string constant",				      /* 258 */
	"argument #%d is converted from '%s' to '%s' due to prototype", /* 259 */
	"previous declaration of %s",				      /* 260 */
	"previous definition of %s",				      /* 261 */
	"\\\" inside character constants undefined in traditional C", /* 262 */
	"\\? undefined in traditional C",			      /* 263 */
	"\\v undefined in traditional C",			      /* 264 */
	"%s does not support 'long long'",			      /* 265 */
	"'long double' is illegal in traditional C",		      /* 266 */
	"shift equal to size of object",			      /* 267 */
	"variable declared inline: %s",				      /* 268 */
	"argument declared inline: %s",				      /* 269 */
	"function prototypes are illegal in traditional C",	      /* 270 */
	"switch expression must be of type 'int' in traditional C",   /* 271 */
	"empty translation unit",				      /* 272 */
	"bit-field type '%s' invalid in ANSI C",		      /* 273 */
	"ANSI C forbids comparison of %s with %s",		      /* 274 */
	"cast discards 'const' from type '%s'",			      /* 275 */
	"__%s__ is illegal for type %s",			      /* 276 */
	"initialization of '%s' with '%s'",			      /* 277 */
	"combination of '%s' and '%s', arg #%d",		      /* 278 */
	"combination of '%s' and '%s' in return",		      /* 279 */
	"must be outside function: /* %s */",			      /* 280 */
	"duplicate use of /* %s */",				      /* 281 */
	"must precede function definition: /* %s */",		      /* 282 */
	"argument number mismatch with directive: /* %s */",	      /* 283 */
	"fallthrough on default statement",			      /* 284 */
	"prototype declaration",				      /* 285 */
	"function definition is not a prototype",		      /* 286 */
	"function declaration is not a prototype",		      /* 287 */
	"dubious use of /* VARARGS */ with /* %s */",		      /* 288 */
	"can't be used together: /* PRINTFLIKE */ /* SCANFLIKE */",   /* 289 */
	"static function %s declared but not defined",		      /* 290 */
	"invalid multibyte character",				      /* 291 */
	"cannot concatenate wide and regular string literals",	      /* 292 */
	"argument %d must be 'char *' for PRINTFLIKE/SCANFLIKE",      /* 293 */
	"multi-character character constant",			      /* 294 */
	"conversion of '%s' to '%s' is out of range, arg #%d",	      /* 295 */
	"conversion of negative constant to unsigned type, arg #%d",  /* 296 */
	"conversion to '%s' may sign-extend incorrectly, arg #%d",    /* 297 */
	"conversion from '%s' to '%s' may lose accuracy, arg #%d",    /* 298 */
	"prototype does not match old style definition, arg #%d",     /* 299 */
	"old style definition",					      /* 300 */
	"array of incomplete type",				      /* 301 */
	"%s returns pointer to automatic object",		      /* 302 */
	"ANSI C forbids conversion of %s to %s",		      /* 303 */
	"ANSI C forbids conversion of %s to %s, arg #%d",	      /* 304 */
	"ANSI C forbids conversion of %s to %s, op %s",		      /* 305 */
	"constant truncated by conversion, op %s",		      /* 306 */
	"static variable %s set but not used",			      /* 307 */
	"invalid type for _Complex",				      /* 308 */
	"extra bits set to 0 in conversion of '%s' to '%s', op '%s'", /* 309 */
	"symbol renaming can't be used on function arguments",	      /* 310 */
	"symbol renaming can't be used on automatic variables",	      /* 311 */
	"%s does not support // comments",			      /* 312 */
	"struct or union member name in initializer is a C99 feature",/* 313 */
	"%s is not a structure or a union",			      /* 314 */
	"GCC style struct or union member name in initializer",	      /* 315 */
	"__FUNCTION__/__PRETTY_FUNCTION__ is a GCC extension",	      /* 316 */
	"__func__ is a C99 feature",				      /* 317 */
	"variable array dimension is a C99/GCC extension",	      /* 318 */
	"compound literals are a C99/GCC extension",		      /* 319 */
	"({ }) is a GCC extension",				      /* 320 */
	"array initializer with designators is a C99 feature",	      /* 321 */
	"zero sized array is a C99 extension",			      /* 322 */
	"continue in 'do ... while (0)' loop",			      /* 323 */
	"suggest cast from '%s' to '%s' on op %s to avoid overflow",  /* 324 */
	"variable declaration in for loop",			      /* 325 */
	"%s attribute ignored for %s",				      /* 326 */
	"declarations after statements is a C99 feature",	      /* 327 */
	"union cast is a GCC extension",			      /* 328 */
	"type '%s' is not a member of '%s'",			      /* 329 */
	"operand of '%s' must be bool, not '%s'",		      /* 330 */
	"left operand of '%s' must be bool, not '%s'",		      /* 331 */
	"right operand of '%s' must be bool, not '%s'",		      /* 332 */
	"controlling expression must be bool, not '%s'",	      /* 333 */
	"argument #%d expects '%s', gets passed '%s'",		      /* 334 */
	"operand of '%s' must not be bool",			      /* 335 */
	"left operand of '%s' must not be bool",		      /* 336 */
	"right operand of '%s' must not be bool",		      /* 337 */
	"option '%c' should be handled in the switch",		      /* 338 */
	"option '%c' should be listed in the options string",	      /* 339 */
	"initialization with '[a...b]' is a GCC extension",	      /* 340 */
	"argument to '%s' must be 'unsigned char' or EOF, not '%s'",  /* 341 */
	"argument to '%s' must be cast to 'unsigned char', not to '%s'", /* 342 */
	"static array size is a C11 extension",			      /* 343 */
	"bit-field of type plain 'int' has implementation-defined signedness", /* 344 */
	"generic selection requires C11 or later",		      /* 345 */
	"call to '%s' effectively discards 'const' from argument",    /* 346 */
	"redeclaration of '%s' with type '%s', expected '%s'",	      /* 347 */
	"maximum value %d of '%s' does not match maximum array index %d", /* 348 */
};

static struct include_level {
	const char *filename;
	int lineno;
	struct include_level *by;
} *includes;


void
update_location(const char *filename, int lineno, bool is_begin, bool is_end)
{
	struct include_level *top;

	top = includes;
	if (is_begin && top != NULL)
		top->lineno = curr_pos.p_line;

	if (top == NULL || is_begin) {
		top = xmalloc(sizeof(*top));
		top->filename = filename;
		top->lineno = lineno;
		top->by = includes;
		includes = top;
	} else {
		if (is_end) {
			includes = top->by;
			free(top);
			top = includes;
		}
		if (top != NULL) {
			top->filename = filename;
			top->lineno = lineno;
		}
	}
}

static void
print_stack_trace(void)
{
	const struct include_level *top;

	if ((top = includes) == NULL)
		return;
	/*
	 * Skip the innermost include level since it is already listed in the
	 * diagnostic itself.  Furthermore, its lineno is the line number of
	 * the last '#' line, not the current line.
	 */
	for (top = top->by; top != NULL; top = top->by)
		printf("\tincluded from %s(%d)\n", top->filename, top->lineno);
}

/*
 * print a list of the messages with their ids
 */
void
msglist(void)
{
	size_t i;

	for (i = 0; i < sizeof(msgs) / sizeof(msgs[0]); i++) {
		if (msgs[i][0] != '\0')
			printf("%zu\t%s\n", i, msgs[i]);
		else
			printf("---\t(no longer used)\n");
	}
}

/*
 * If Fflag is not set, lbasename() returns a pointer to the last
 * component of the path, otherwise it returns the argument.
 */
static const char *
lbasename(const char *path)
{
	const char *p, *base, *dir;

	if (Fflag)
		return path;

	p = base = dir = path;
	while (*p != '\0') {
		if (*p++ == '/') {
			dir = base;
			base = p;
		}
	}
	return *base != '\0' ? base : dir;
}

static void
verror_at(int msgid, const pos_t *pos, va_list ap)
{
	const	char *fn;

	if (ERR_ISSET(msgid, &msgset))
		return;

	fn = lbasename(pos->p_file);
	(void)printf("%s(%d): error: ", fn, pos->p_line);
	(void)vprintf(msgs[msgid], ap);
	(void)printf(" [%d]\n", msgid);
	nerr++;
	print_stack_trace();
}

static void
vwarning_at(int msgid, const pos_t *pos, va_list ap)
{
	const	char *fn;

	if (ERR_ISSET(msgid, &msgset))
		return;

	debug_step("%s: lwarn=%d msgid=%d", __func__, lwarn, msgid);
	if (lwarn == LWARN_NONE || lwarn == msgid)
		/* this warning is suppressed by a LINTED comment */
		return;

	fn = lbasename(pos->p_file);
	(void)printf("%s(%d): warning: ", fn, pos->p_line);
	(void)vprintf(msgs[msgid], ap);
	(void)printf(" [%d]\n", msgid);
	if (wflag)
		nerr++;
	print_stack_trace();
}

static void
vmessage_at(int msgid, const pos_t *pos, va_list ap)
{
	const char *fn;

	if (ERR_ISSET(msgid, &msgset))
		return;

	fn = lbasename(pos->p_file);
	(void)printf("%s(%d): ", fn, pos->p_line);
	(void)vprintf(msgs[msgid], ap);
	(void)printf(" [%d]\n", msgid);
	print_stack_trace();
}

void
(error_at)(int msgid, const pos_t *pos, ...)
{
	va_list	ap;

	va_start(ap, pos);
	verror_at(msgid, pos, ap);
	va_end(ap);
}

void
(error)(int msgid, ...)
{
	va_list	ap;

	va_start(ap, msgid);
	verror_at(msgid, &curr_pos, ap);
	va_end(ap);
}

void
internal_error(const char *file, int line, const char *msg, ...)
{
	va_list	ap;
	const	char *fn;

	fn = lbasename(curr_pos.p_file);
	(void)fflush(stdout);
	(void)fprintf(stderr, "lint: internal error in %s:%d near %s:%d: ",
	    file, line, fn, curr_pos.p_line);
	va_start(ap, msg);
	(void)vfprintf(stderr, msg, ap);
	va_end(ap);
	(void)fprintf(stderr, "\n");
	print_stack_trace();
	abort();
}

void
assert_failed(const char *file, int line, const char *func, const char *cond)
{
	const	char *fn;

	fn = lbasename(curr_pos.p_file);
	(void)fflush(stdout);
	(void)fprintf(stderr,
	    "lint: assertion \"%s\" failed in %s at %s:%d near %s:%d\n",
	    cond, func, file, line, fn, curr_pos.p_line);
	print_stack_trace();
	(void)fflush(stdout);
	abort();
}

void
(warning_at)(int msgid, const pos_t *pos, ...)
{
	va_list	ap;

	va_start(ap, pos);
	vwarning_at(msgid, pos, ap);
	va_end(ap);
}

void
(warning)(int msgid, ...)
{
	va_list	ap;

	va_start(ap, msgid);
	vwarning_at(msgid, &curr_pos, ap);
	va_end(ap);
}

void
(message_at)(int msgid, const pos_t *pos, ...)
{
	va_list ap;

	va_start(ap, pos);
	vmessage_at(msgid, pos, ap);
	va_end(ap);
}

void
(c99ism)(int msgid, ...)
{
	va_list	ap;
	bool extensions_ok = Sflag || gflag;

	va_start(ap, msgid);
	if (sflag && !extensions_ok) {
		verror_at(msgid, &curr_pos, ap);
	} else if (sflag || !extensions_ok) {
		vwarning_at(msgid, &curr_pos, ap);
	}
	va_end(ap);
}

void
(c11ism)(int msgid, ...)
{
	va_list	ap;

	if (c11flag || gflag)
		return;
	va_start(ap, msgid);
	verror_at(msgid, &curr_pos, ap);
	va_end(ap);
}

void
(gnuism)(int msgid, ...)
{
	va_list	ap;

	va_start(ap, msgid);
	if (sflag && !gflag) {
		verror_at(msgid, &curr_pos, ap);
	} else if (sflag || !gflag) {
		vwarning_at(msgid, &curr_pos, ap);
	}
	va_end(ap);
}
