%{
/******************************************************************************
 *
 * Module Name: dtcompilerparser.l - Flex input file for table compiler lexer
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
#include "dtcompilerparser.y.h"


#define _COMPONENT          ACPI_COMPILER
        ACPI_MODULE_NAME    ("dtcompilerscanner")

/* handle locations */

int DtCompilerParsercolumn = 1;
int DtLabelByteOffset = 0;
int DtCompilerParserByteOffset = 0;

UINT32 DtTokenFirstLine = 0;
UINT32 DtTokenFirstColumn = 0;

#define YY_USER_ACTION \
            DtTokenFirstLine = DtCompilerParserlineno; \
            DtTokenFirstColumn = DtCompilerParsercolumn; \
            DtCompilerParsercolumn += DtCompilerParserleng; \
            DtCompilerParserByteOffset += DtCompilerParserleng; \
            DbgPrint (ASL_PARSE_OUTPUT,\
                "user action occurred. DtCompilerParserlloc.first_line: %u\n",\
                DtTokenFirstLine);
%}

%option nounput noinput yylineno

    /* Indicates a state used for parsing multiline C comments */
%x ML_COMMENT
%x DATA_STATE

WhiteSpace      [ \t\v\r]+
NewLines        [\n]+

    /* Avoid ", \n, and [] as a part of label name. These are not valid characters of a label name */
LabelName       [^ ":\n\[\]]([^":\n\[\]]*[^" :\n\[\]])?

    /* Avoid ", \n, \\, and [] as a part of data. These are not valid characters of data */
Data            [^ \\":\n\[\]]([^":\n\[\]\\]*[^" :\n\[\]\\])?

Text            [^ ":\n][^":\n]*
Comment         \[[^\n\[\]]*\]
CommentField    {LabelName}{WhiteSpace}*:{WhiteSpace}{Comment}?$


%%

<DATA_STATE>{WhiteSpace}"\\\n" {
        DbgPrint(ASL_PARSE_OUTPUT,"Continuation matched\n");
        return (DT_PARSEOP_LINE_CONTINUATION);
    }

":" {
        DbgPrint(ASL_PARSE_OUTPUT, ": Matched\n");
        BEGIN (DATA_STATE);
        return (':');
    }

<INITIAL,DATA_STATE>{WhiteSpace} { DbgPrint(ASL_PARSE_OUTPUT,"Whitespace matched\n"); }

<INITIAL,DATA_STATE>{Comment}    { DbgPrint(ASL_PARSE_OUTPUT,"Comment matched\n"); }

"/*"                     { BEGIN (ML_COMMENT); }
<ML_COMMENT>"*/"         { BEGIN (INITIAL); }
<ML_COMMENT>"*/\n"       { BEGIN (INITIAL); }
<ML_COMMENT>([^*]|\n)+|. /* Ignore */
"//".*                   /* Ignore */


<DATA_STATE>{Data} {
      char *s;
      int size = strlen (DtCompilerParsertext);
      s=UtLocalCacheCalloc (size + 1);
      AcpiUtSafeStrncpy (s, DtCompilerParsertext, size + 1);
      DtCompilerParserlval.s = s;
      DbgPrint (ASL_PARSE_OUTPUT, "Data: %s\n", s);
      return (DT_PARSEOP_DATA);
}

{CommentField}  /* ignore */

{LabelName} {
    char *s;
    int size = strlen (DtCompilerParsertext);
    s=UtLocalCacheCalloc (size + 1);
    AcpiUtSafeStrncpy (s, DtCompilerParsertext, size + 1);
    DtCompilerParserlval.u = (DT_TABLE_UNIT *) UtLocalCacheCalloc (sizeof (DT_TABLE_UNIT));
    DtCompilerParserlval.u->Value = s;
    DtCompilerParserlval.u->Line = DtCompilerParserlineno;
    DtCompilerParserlval.u->Column = DtCompilerParsercolumn;
    DtLabelByteOffset = DtCompilerParserByteOffset;
    DbgPrint (ASL_PARSE_OUTPUT, "Label: %s\n", s);
    return (DT_PARSEOP_LABEL);
}


<DATA_STATE>\"{Text}?\" { // remove outer quotes from the string, they are unnecessary
    char *s;
    int size = strlen (DtCompilerParsertext);
    s=UtLocalCacheCalloc (size - 1);
    AcpiUtSafeStrncpy (s, DtCompilerParsertext + 1, size - 1);
    DtCompilerParserlval.s = s;
    DbgPrint (ASL_PARSE_OUTPUT, "String Data: %s\n", s);
    BEGIN (INITIAL);
    return (DT_PARSEOP_STRING_DATA);
}


<INITIAL,DATA_STATE>{NewLines} {
    DbgPrint(ASL_PARSE_OUTPUT,
        "Newline matched (data state). Current line number: %u\n",DtCompilerParserlineno);
    BEGIN (INITIAL); DtCompilerParsercolumn = 1;
}


%%


/*
 * Local support functions
 */

void
DtCompilerInitLexer (
    FILE                *inFile)
{
    yyin = inFile;
}

void
DtCompilerTerminateLexer (
    void)
{
    /*
     * Flex/Bison increments the lineno for the EOF so decrement by 1 to get
     * the correct number of lines.
     */
    AslGbl_CurrentLineNumber = DtCompilerParserlineno - 1;
    AslGbl_InputByteCount = DtCompilerParserByteOffset;
}
