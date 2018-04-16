/******************************************************************************
 *
 * Module Name: asldebug -- Debug output support
 *
 *****************************************************************************/

/*
 * Copyright (C) 2000 - 2018, Intel Corp.
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
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR
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
#include "aslcompiler.y.h"


#define _COMPONENT          ACPI_COMPILER
        ACPI_MODULE_NAME    ("asldebug")


/* Local prototypes */

static void
UtDumpParseOpName (
    ACPI_PARSE_OBJECT       *Op,
    UINT32                  Level,
    UINT32                  DataLength);

static char *
UtCreateEscapeSequences (
    char                    *InString);


/*******************************************************************************
 *
 * FUNCTION:    CvDbgPrint
 *
 * PARAMETERS:  Type                - Type of output
 *              Fmt                 - Printf format string
 *              ...                 - variable printf list
 *
 * RETURN:      None
 *
 * DESCRIPTION: Print statement for debug messages within the converter.
 *
 ******************************************************************************/

void
CvDbgPrint (
    char                    *Fmt,
    ...)
{
    va_list                 Args;


    if (!AcpiGbl_CaptureComments || !AcpiGbl_DebugAslConversion)
    {
        return;
    }

    va_start (Args, Fmt);
    (void) vfprintf (AcpiGbl_ConvDebugFile, Fmt, Args);
    va_end (Args);
    return;
}


/*******************************************************************************
 *
 * FUNCTION:    UtDumpIntegerOp
 *
 * PARAMETERS:  Op                  - Current parse op
 *              Level               - Current output indentation level
 *              IntegerLength       - Output length of the integer (2/4/8/16)
 *
 * RETURN:      None
 *
 * DESCRIPTION: Emit formatted debug output for "integer" ops.
 *              Note: IntegerLength must be one of 2,4,8,16.
 *
 ******************************************************************************/

void
UtDumpIntegerOp (
    ACPI_PARSE_OBJECT       *Op,
    UINT32                  Level,
    UINT32                  IntegerLength)
{

    /* Emit the ParseOp name, leaving room for the integer */

    UtDumpParseOpName (Op, Level, IntegerLength);

    /* Emit the integer based upon length */

    switch (IntegerLength)
    {
    case 2: /* Byte */
    case 4: /* Word */
    case 8: /* Dword */

        DbgPrint (ASL_TREE_OUTPUT,
            "%*.*X", IntegerLength, IntegerLength, Op->Asl.Value.Integer);
        break;

    case 16: /* Qword and Integer */

        DbgPrint (ASL_TREE_OUTPUT,
            "%8.8X%8.8X", ACPI_FORMAT_UINT64 (Op->Asl.Value.Integer));
        break;

    default:
        break;
    }
}


/*******************************************************************************
 *
 * FUNCTION:    UtDumpStringOp
 *
 * PARAMETERS:  Op                  - Current parse op
 *              Level               - Current output indentation level
 *
 * RETURN:      None
 *
 * DESCRIPTION: Emit formatted debug output for String/Pathname ops.
 *
 ******************************************************************************/

void
UtDumpStringOp (
    ACPI_PARSE_OBJECT       *Op,
    UINT32                  Level)
{
    char                    *String;


    String = Op->Asl.Value.String;
    if (Op->Asl.ParseOpcode != PARSEOP_STRING_LITERAL)
    {
        /*
         * For the "path" ops NAMEPATH, NAMESEG, METHODCALL -- if the
         * ExternalName is valid, it takes precedence. In these cases the
         * Value.String is the raw "internal" name from the AML code, which
         * we don't want to use, because it contains non-ascii characters.
         */
        if (Op->Asl.ExternalName)
        {
            String = Op->Asl.ExternalName;
        }
    }

    if (!String)
    {
        DbgPrint (ASL_TREE_OUTPUT,
            " ERROR: Could not find a valid String/Path pointer\n");
        return;
    }

    String = UtCreateEscapeSequences (String);

    /* Emit the ParseOp name, leaving room for the string */

    UtDumpParseOpName (Op, Level, strlen (String));
    DbgPrint (ASL_TREE_OUTPUT, "%s", String);
}


/*******************************************************************************
 *
 * FUNCTION:    UtCreateEscapeSequences
 *
 * PARAMETERS:  InString            - ASCII string to be expanded
 *
 * RETURN:      Expanded string
 *
 * DESCRIPTION: Expand all non-printable ASCII bytes (0-0x1F) to escape
 *              sequences. For example, hex 14 becomes \x14
 *
 * NOTE:        Since this function is used for debug output only, it does
 *              not attempt to translate into the "known" escape sequences
 *              such as \a, \f, \t, etc.
 *
 ******************************************************************************/

static char *
UtCreateEscapeSequences (
    char                    *InString)
{
    char                    *String = InString;
    char                    *OutString;
    char                    *OutStringPtr;
    UINT32                  InStringLength = 0;
    UINT32                  EscapeCount = 0;


    /*
     * Determine up front how many escapes are within the string.
     * Obtain the input string length while doing so.
     */
    while (*String)
    {
        if ((*String <= 0x1F) || (*String >= 0x7F))
        {
            EscapeCount++;
        }

        InStringLength++;
        String++;
    }

    if (!EscapeCount)
    {
        return (InString); /* No escapes, nothing to do */
    }

    /* New string buffer, 3 extra chars per escape (4 total) */

    OutString = UtLocalCacheCalloc (InStringLength + (EscapeCount * 3));
    OutStringPtr = OutString;

    /* Convert non-ascii or non-printable chars to escape sequences */

    while (*InString)
    {
        if ((*InString <= 0x1F) || (*InString >= 0x7F))
        {
            /* Insert a \x hex escape sequence */

            OutStringPtr[0] = '\\';
            OutStringPtr[1] = 'x';
            OutStringPtr[2] = AcpiUtHexToAsciiChar (*InString, 4);
            OutStringPtr[3] = AcpiUtHexToAsciiChar (*InString, 0);
            OutStringPtr += 4;
        }
        else /* Normal ASCII character */
        {
            *OutStringPtr = *InString;
            OutStringPtr++;
        }

        InString++;
    }

    return (OutString);
}


/*******************************************************************************
 *
 * FUNCTION:    UtDumpBasicOp
 *
 * PARAMETERS:  Op                  - Current parse op
 *              Level               - Current output indentation level
 *
 * RETURN:      None
 *
 * DESCRIPTION: Generic formatted debug output for "basic" ops that have no
 *              associated strings or integer values.
 *
 ******************************************************************************/

void
UtDumpBasicOp (
    ACPI_PARSE_OBJECT       *Op,
    UINT32                  Level)
{

    /* Just print out the ParseOp name, there is no extra data */

    UtDumpParseOpName (Op, Level, 0);
}


/*******************************************************************************
 *
 * FUNCTION:    UtDumpParseOpName
 *
 * PARAMETERS:  Op                  - Current parse op
 *              Level               - Current output indentation level
 *              DataLength          - Length of data to appear after the name
 *
 * RETURN:      None
 *
 * DESCRIPTION: Indent and emit the ascii ParseOp name for the op
 *
 ******************************************************************************/

static void
UtDumpParseOpName (
    ACPI_PARSE_OBJECT       *Op,
    UINT32                  Level,
    UINT32                  DataLength)
{
    char                    *ParseOpName;
    UINT32                  IndentLength;
    UINT32                  NameLength;
    UINT32                  LineLength;
    UINT32                  PaddingLength;


    /* Emit the LineNumber/IndentLevel prefix on each output line */

    DbgPrint (ASL_TREE_OUTPUT,
        "%5.5d [%2d]", Op->Asl.LogicalLineNumber, Level);

    ParseOpName = UtGetOpName (Op->Asl.ParseOpcode);

    /* Calculate various lengths for output alignment */

    IndentLength = Level * DEBUG_SPACES_PER_INDENT;
    NameLength = strlen (ParseOpName);
    LineLength = IndentLength + 1 + NameLength + 1 + DataLength;
    PaddingLength = (DEBUG_MAX_LINE_LENGTH + 1) - LineLength;

    /* Parse tree indentation is based upon the nesting/indent level */

    if (Level)
    {
        DbgPrint (ASL_TREE_OUTPUT, "%*s", IndentLength, " ");
    }

    /* Emit the actual name here */

    DbgPrint (ASL_TREE_OUTPUT, " %s", ParseOpName);

    /* Emit extra padding blanks for alignment of later data items */

    if (LineLength > DEBUG_MAX_LINE_LENGTH)
    {
        /* Split a long line immediately after the ParseOpName string */

        DbgPrint (ASL_TREE_OUTPUT, "\n%*s",
            (DEBUG_FULL_LINE_LENGTH - DataLength), " ");
    }
    else
    {
        DbgPrint (ASL_TREE_OUTPUT, "%*s", PaddingLength, " ");
    }
}
