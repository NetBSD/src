#include <stdio.h>
#include "awk.h"
#include "awkgram.h"

static const char * const printname[93] = {
	"FIRSTTOKEN",	/* 257 */
	"PROGRAM",	/* 258 */
	"PASTAT",	/* 259 */
	"PASTAT2",	/* 260 */
	"XBEGIN",	/* 261 */
	"XEND",	/* 262 */
	"NL",	/* 263 */
	"ARRAY",	/* 264 */
	"MATCH",	/* 265 */
	"NOTMATCH",	/* 266 */
	"MATCHOP",	/* 267 */
	"FINAL",	/* 268 */
	"DOT",	/* 269 */
	"ALL",	/* 270 */
	"CCL",	/* 271 */
	"NCCL",	/* 272 */
	"CHAR",	/* 273 */
	"OR",	/* 274 */
	"STAR",	/* 275 */
	"QUEST",	/* 276 */
	"PLUS",	/* 277 */
	"AND",	/* 278 */
	"BOR",	/* 279 */
	"APPEND",	/* 280 */
	"EQ",	/* 281 */
	"GE",	/* 282 */
	"GT",	/* 283 */
	"LE",	/* 284 */
	"LT",	/* 285 */
	"NE",	/* 286 */
	"IN",	/* 287 */
	"ARG",	/* 288 */
	"BLTIN",	/* 289 */
	"BREAK",	/* 290 */
	"CLOSE",	/* 291 */
	"CONTINUE",	/* 292 */
	"DELETE",	/* 293 */
	"DO",	/* 294 */
	"EXIT",	/* 295 */
	"FOR",	/* 296 */
	"FUNC",	/* 297 */
	"SUB",	/* 298 */
	"GSUB",	/* 299 */
	"IF",	/* 300 */
	"INDEX",	/* 301 */
	"LSUBSTR",	/* 302 */
	"MATCHFCN",	/* 303 */
	"NEXT",	/* 304 */
	"NEXTFILE",	/* 305 */
	"ADD",	/* 306 */
	"MINUS",	/* 307 */
	"MULT",	/* 308 */
	"DIVIDE",	/* 309 */
	"MOD",	/* 310 */
	"ASSIGN",	/* 311 */
	"ASGNOP",	/* 312 */
	"ADDEQ",	/* 313 */
	"SUBEQ",	/* 314 */
	"MULTEQ",	/* 315 */
	"DIVEQ",	/* 316 */
	"MODEQ",	/* 317 */
	"POWEQ",	/* 318 */
	"PRINT",	/* 319 */
	"PRINTF",	/* 320 */
	"SPRINTF",	/* 321 */
	"ELSE",	/* 322 */
	"INTEST",	/* 323 */
	"CONDEXPR",	/* 324 */
	"POSTINCR",	/* 325 */
	"PREINCR",	/* 326 */
	"POSTDECR",	/* 327 */
	"PREDECR",	/* 328 */
	"VAR",	/* 329 */
	"IVAR",	/* 330 */
	"VARNF",	/* 331 */
	"CALL",	/* 332 */
	"NUMBER",	/* 333 */
	"STRING",	/* 334 */
	"REGEXPR",	/* 335 */
	"GETLINE",	/* 336 */
	"GENSUB",	/* 337 */
	"RETURN",	/* 338 */
	"SPLIT",	/* 339 */
	"SUBSTR",	/* 340 */
	"WHILE",	/* 341 */
	"CAT",	/* 342 */
	"NOT",	/* 343 */
	"UMINUS",	/* 344 */
	"POWER",	/* 345 */
	"DECR",	/* 346 */
	"INCR",	/* 347 */
	"INDIRECT",	/* 348 */
	"LASTTOKEN",	/* 349 */
};


Cell *(*proctab[93])(Node **, int) = {
	nullproc,	/* FIRSTTOKEN */
	program,	/* PROGRAM */
	pastat,	/* PASTAT */
	dopa2,	/* PASTAT2 */
	nullproc,	/* XBEGIN */
	nullproc,	/* XEND */
	nullproc,	/* NL */
	array,	/* ARRAY */
	matchop,	/* MATCH */
	matchop,	/* NOTMATCH */
	nullproc,	/* MATCHOP */
	nullproc,	/* FINAL */
	nullproc,	/* DOT */
	nullproc,	/* ALL */
	nullproc,	/* CCL */
	nullproc,	/* NCCL */
	nullproc,	/* CHAR */
	nullproc,	/* OR */
	nullproc,	/* STAR */
	nullproc,	/* QUEST */
	nullproc,	/* PLUS */
	boolop,	/* AND */
	boolop,	/* BOR */
	nullproc,	/* APPEND */
	relop,	/* EQ */
	relop,	/* GE */
	relop,	/* GT */
	relop,	/* LE */
	relop,	/* LT */
	relop,	/* NE */
	instat,	/* IN */
	arg,	/* ARG */
	bltin,	/* BLTIN */
	jump,	/* BREAK */
	closefile,	/* CLOSE */
	jump,	/* CONTINUE */
	awkdelete,	/* DELETE */
	dostat,	/* DO */
	jump,	/* EXIT */
	forstat,	/* FOR */
	nullproc,	/* FUNC */
	sub,	/* SUB */
	gsub,	/* GSUB */
	ifstat,	/* IF */
	sindex,	/* INDEX */
	nullproc,	/* LSUBSTR */
	matchop,	/* MATCHFCN */
	jump,	/* NEXT */
	jump,	/* NEXTFILE */
	arith,	/* ADD */
	arith,	/* MINUS */
	arith,	/* MULT */
	arith,	/* DIVIDE */
	arith,	/* MOD */
	assign,	/* ASSIGN */
	nullproc,	/* ASGNOP */
	assign,	/* ADDEQ */
	assign,	/* SUBEQ */
	assign,	/* MULTEQ */
	assign,	/* DIVEQ */
	assign,	/* MODEQ */
	assign,	/* POWEQ */
	printstat,	/* PRINT */
	awkprintf,	/* PRINTF */
	awksprintf,	/* SPRINTF */
	nullproc,	/* ELSE */
	intest,	/* INTEST */
	condexpr,	/* CONDEXPR */
	incrdecr,	/* POSTINCR */
	incrdecr,	/* PREINCR */
	incrdecr,	/* POSTDECR */
	incrdecr,	/* PREDECR */
	nullproc,	/* VAR */
	nullproc,	/* IVAR */
	getnf,	/* VARNF */
	call,	/* CALL */
	nullproc,	/* NUMBER */
	nullproc,	/* STRING */
	nullproc,	/* REGEXPR */
	getline,	/* GETLINE */
	gensub,	/* GENSUB */
	jump,	/* RETURN */
	split,	/* SPLIT */
	substr,	/* SUBSTR */
	whilestat,	/* WHILE */
	cat,	/* CAT */
	boolop,	/* NOT */
	arith,	/* UMINUS */
	arith,	/* POWER */
	nullproc,	/* DECR */
	nullproc,	/* INCR */
	indirect,	/* INDIRECT */
	nullproc,	/* LASTTOKEN */
};

const char *tokname(int n)
{
	static char buf[100];

	if (n < FIRSTTOKEN || n > LASTTOKEN) {
		snprintf(buf, sizeof(buf), "token %d", n);
		return buf;
	}
	return printname[n-FIRSTTOKEN];
}
