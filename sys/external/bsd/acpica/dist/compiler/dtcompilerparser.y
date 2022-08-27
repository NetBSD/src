%{
/******************************************************************************
 *
 * Module Name: dtcompilerparser.y - Bison input file for table compiler parser
 *
 *****************************************************************************/

/*
 * Copyright (C) 2000 - 2022, Intel Corp.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer,
 *    without modification.
 * 2. Redistributions in binary form must reproduce at minimum a disclaimer
 *    substantially similar to the "NO WARRANTY" disclaimer below
 *    ("Disclaimer") and any redistribution must be conditioned upon
 *    including a substantially similar Disclaimer requirement for further
 *    binary redistribution.
 * 3. Neither the names of the above-listed copyright holders nor the names
 *    of any contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 *
 * NO WARRANTY
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDERS OR CONTRIBUTORS BE LIABLE FOR SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGES.
 */

#include "aslcompiler.h"


#define _COMPONENT          DT_COMPILER
        ACPI_MODULE_NAME    ("dtcompilerparser")

void *                      AslLocalAllocate (unsigned int Size);

/* Bison/yacc configuration */

#undef alloca
#define alloca              AslLocalAllocate

int                         DtCompilerParserlex (void);
int                         DtCompilerParserparse (void);
#ifdef YYBYACC
struct YYLTYPE;
#endif
void                        DtCompilerParsererror (
#ifdef YYBYACC
				    struct YYLTYPE *loc,
#endif
				    char const *msg);
extern char                 *DtCompilerParsertext;
extern DT_FIELD             *AslGbl_CurrentField;

extern int                  DtLabelByteOffset;
extern UINT64               DtCompilerParserlineno; /* Current line number */

extern UINT32               DtTokenFirstLine;
extern UINT32               DtTokenFirstColumn;

/* Bison/yacc configuration */

#define yytname             DtCompilerParsername
#define YYDEBUG             1               /* Enable debug output */
#define YYERROR_VERBOSE     1               /* Verbose error messages */
#define YYFLAG              -32768

/* Define YYMALLOC/YYFREE to prevent redefinition errors  */

#define YYMALLOC            malloc
#define YYFREE              free

%}


%union {
    char                *s;
    DT_FIELD            *f;
    DT_TABLE_UNIT       *u;
}


%type  <f> Table
%token <u> DT_PARSEOP_DATA
%token <u> DT_PARSEOP_LABEL
%token <u> DT_PARSEOP_STRING_DATA
%token <u> DT_PARSEOP_LINE_CONTINUATION
%type  <u> Data
%type  <u> Datum
%type  <u> MultiLineData
%type  <u> MultiLineDataList


%%

Table
    :
    FieldList { }
    ;

FieldList
    : Field FieldList
    | Field
    ;

Field
    : DT_PARSEOP_LABEL ':' Data { DtCreateField ($1, $3, DtLabelByteOffset); }
    ;

Data
    : MultiLineDataList        { $$ = $1; }
    | Datum                    { $$ = $1; }
    | Datum MultiLineDataList  { $$ = $1; } /* combine the string with strcat */
    ;

MultiLineDataList
    : MultiLineDataList MultiLineData { $$ = DtCreateTableUnit (AcpiUtStrcat(AcpiUtStrcat($1->Value, " "), $2->Value), $1->Line, $1->Column); } /* combine the strings with strcat */
    | MultiLineData                   { $$ = $1; }
    ;

MultiLineData
    : DT_PARSEOP_LINE_CONTINUATION Datum { DbgPrint (ASL_PARSE_OUTPUT, "line continuation detected\n"); $$ = $2; }
    ;

Datum
    : DT_PARSEOP_DATA        {
                                 DbgPrint (ASL_PARSE_OUTPUT, "parser        data: [%s]\n", DtCompilerParserlval.s);
                                 $$ = DtCreateTableUnit (AcpiUtStrdup(DtCompilerParserlval.s), DtTokenFirstLine, DtTokenFirstColumn);
                             }
    | DT_PARSEOP_STRING_DATA {
                                 DbgPrint (ASL_PARSE_OUTPUT, "parser string data: [%s]\n", DtCompilerParserlval.s);
                                 $$ = DtCreateTableUnit (AcpiUtStrdup(DtCompilerParserlval.s), DtTokenFirstLine, DtTokenFirstColumn);
                             }
    ;


%%


/*
 * Local support functions, including parser entry point
 */
/******************************************************************************
 *
 * FUNCTION:    DtCompilerParsererror
 *
 * PARAMETERS:  Message             - Parser-generated error message
 *
 * RETURN:      None
 *
 * DESCRIPTION: Handler for parser errors
 *
 *****************************************************************************/

void
DtCompilerParsererror (
#ifdef YYBYACC
    struct YYLTYPE *loc,
#endif
    char const              *Message)
{
    DtError (ASL_ERROR, ASL_MSG_SYNTAX,
        AslGbl_CurrentField, (char *) Message);
}

int
DtCompilerParserwrap(void)
{
  return (1);
}
