/******************************************************************************
 *
 * Module Name: aslglobal.h - Global variable definitions
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

#ifndef __ASLGLOBAL_H
#define __ASLGLOBAL_H


/*
 * Global variables. Defined in aslmain.c only, externed in all other files
 */

#undef ASL_EXTERN

#ifdef _DECLARE_GLOBALS
#define ASL_EXTERN
#define ASL_INIT_GLOBAL(a,b)        (a)=(b)
#else
#define ASL_EXTERN                  extern
#define ASL_INIT_GLOBAL(a,b)        (a)
#endif


#ifdef _DECLARE_GLOBALS
UINT32                              Gbl_ExceptionCount[ASL_NUM_REPORT_LEVELS] = {0,0,0,0,0,0};

/* Table below must match ASL_FILE_TYPES in asltypes.h */

ASL_FILE_INFO                       Gbl_Files [ASL_NUM_FILES] =
{
    {NULL, NULL, "stdout:       ", "Standard Output"},
    {NULL, NULL, "stderr:       ", "Standard Error"},
    {NULL, NULL, "Table Input:  ", "Source Input"},
    {NULL, NULL, "Binary Output:", "AML Output"},
    {NULL, NULL, "Source Output:", "Source Output"},
    {NULL, NULL, "Preprocessor: ", "Preprocessor Output"},
    {NULL, NULL, "Preprocessor: ", "Preprocessor Temp File"},
    {NULL, NULL, "Listing File: ", "Listing Output"},
    {NULL, NULL, "Hex Dump:     ", "Hex Table Output"},
    {NULL, NULL, "Namespace:    ", "Namespace Output"},
    {NULL, NULL, "Debug File:   ", "Debug Output"},
    {NULL, NULL, "ASM Source:   ", "Assembly Code Output"},
    {NULL, NULL, "C Source:     ", "C Code Output"},
    {NULL, NULL, "ASM Include:  ", "Assembly Header Output"},
    {NULL, NULL, "C Include:    ", "C Header Output"},
    {NULL, NULL, "Offset Table: ", "C Offset Table Output"},
    {NULL, NULL, "Device Map:   ", "Device Map Output"},
    {NULL, NULL, "Cross Ref:    ", "Cross-reference Output"},
    {NULL, NULL, "Converter db :", "Converter debug Output"}
};

/* Table below must match the defines with the same names in actypes.h */

const char                          *Gbl_OpFlagNames[ACPI_NUM_OP_FLAGS] =
{
    "OP_VISITED",
    "OP_AML_PACKAGE",
    "OP_IS_TARGET",
    "OP_IS_RESOURCE_DESC",
    "OP_IS_RESOURCE_FIELD",
    "OP_HAS_NO_EXIT",
    "OP_IF_HAS_NO_EXIT",
    "OP_NAME_INTERNALIZED",
    "OP_METHOD_NO_RETVAL",
    "OP_METHOD_SOME_NO_RETVAL",
    "OP_RESULT_NOT_USED",
    "OP_METHOD_TYPED",
    "OP_COULD_NOT_REDUCE",
    "OP_COMPILE_TIME_CONST",
    "OP_IS_TERM_ARG",
    "OP_WAS_ONES_OP",
    "OP_IS_NAME_DECLARATION",
    "OP_COMPILER_EMITTED",
    "OP_IS_DUPLICATE",
    "OP_IS_RESOURCE_DATA",
    "OP_IS_NULL_RETURN"
};

#else
extern UINT32                       Gbl_ExceptionCount[ASL_NUM_REPORT_LEVELS];
extern ASL_FILE_INFO                Gbl_Files [ASL_NUM_FILES];
extern const char                   *Gbl_OpFlagNames[ACPI_NUM_OP_FLAGS];
#endif


/*
 * Parser and other externals
 */
extern int                          yydebug;
extern FILE                         *AslCompilerin;
extern int                          DtParserdebug;
extern int                          PrParserdebug;
extern const ASL_MAPPING_ENTRY      AslKeywordMapping[];
extern char                         *AslCompilertext;

/*
 * Older versions of Bison won't emit this external in the generated header.
 * Newer versions do emit the external, so we don't need to do it.
 */
#ifndef ASLCOMPILER_ASLCOMPILERPARSE_H
extern int                  AslCompilerdebug;
#endif


#define ASL_DEFAULT_LINE_BUFFER_SIZE    (1024 * 32) /* 32K */
#define ASL_MSG_BUFFER_SIZE             (1024 * 32) /* 32k */
#define ASL_MAX_DISABLED_MESSAGES       32
#define ASL_MAX_EXPECTED_MESSAGES       32
#define HEX_TABLE_LINE_SIZE             8
#define HEX_LISTING_LINE_SIZE           8


/* Source code buffers and pointers for error reporting */

ASL_EXTERN char                     ASL_INIT_GLOBAL (*Gbl_CurrentLineBuffer, NULL);
ASL_EXTERN char                     ASL_INIT_GLOBAL (*Gbl_LineBufPtr, NULL);
ASL_EXTERN UINT32                   ASL_INIT_GLOBAL (Gbl_LineBufferSize, ASL_DEFAULT_LINE_BUFFER_SIZE);
ASL_EXTERN UINT32                   ASL_INIT_GLOBAL (Gbl_CurrentColumn, 0);
ASL_EXTERN UINT32                   ASL_INIT_GLOBAL (Gbl_CurrentLineNumber, 1);
ASL_EXTERN UINT32                   ASL_INIT_GLOBAL (Gbl_LogicalLineNumber, 1);
ASL_EXTERN UINT32                   ASL_INIT_GLOBAL (Gbl_CurrentLineOffset, 0);
ASL_EXTERN UINT32                   ASL_INIT_GLOBAL (Gbl_OriginalInputFileSize, 0);
ASL_EXTERN UINT8                    ASL_INIT_GLOBAL (Gbl_SyntaxError, 0);

/* Exception reporting */

ASL_EXTERN ASL_ERROR_MSG            ASL_INIT_GLOBAL (*Gbl_ErrorLog,NULL);
ASL_EXTERN ASL_ERROR_MSG            ASL_INIT_GLOBAL (*Gbl_NextError,NULL);

/* Option flags */

ASL_EXTERN BOOLEAN                  ASL_INIT_GLOBAL (Gbl_DoCompile, TRUE);
ASL_EXTERN BOOLEAN                  ASL_INIT_GLOBAL (Gbl_DoSignon, TRUE);
ASL_EXTERN BOOLEAN                  ASL_INIT_GLOBAL (Gbl_PreprocessOnly, FALSE);
ASL_EXTERN BOOLEAN                  ASL_INIT_GLOBAL (Gbl_PreprocessFlag, TRUE);
ASL_EXTERN BOOLEAN                  ASL_INIT_GLOBAL (Gbl_DisassembleAll, FALSE);

ASL_EXTERN BOOLEAN                  ASL_INIT_GLOBAL (Gbl_UseDefaultAmlFilename, TRUE);
ASL_EXTERN BOOLEAN                  ASL_INIT_GLOBAL (Gbl_MapfileFlag, FALSE);
ASL_EXTERN BOOLEAN                  ASL_INIT_GLOBAL (Gbl_NsOutputFlag, FALSE);
ASL_EXTERN BOOLEAN                  ASL_INIT_GLOBAL (Gbl_PreprocessorOutputFlag, FALSE);
ASL_EXTERN BOOLEAN                  ASL_INIT_GLOBAL (Gbl_KeepPreprocessorTempFile, FALSE);
ASL_EXTERN BOOLEAN                  ASL_INIT_GLOBAL (Gbl_DebugFlag, FALSE);
ASL_EXTERN BOOLEAN                  ASL_INIT_GLOBAL (Gbl_CrossReferenceOutput, FALSE);
ASL_EXTERN BOOLEAN                  ASL_INIT_GLOBAL (Gbl_AsmOutputFlag, FALSE);
ASL_EXTERN BOOLEAN                  ASL_INIT_GLOBAL (Gbl_C_OutputFlag, FALSE);
ASL_EXTERN BOOLEAN                  ASL_INIT_GLOBAL (Gbl_C_OffsetTableFlag, FALSE);
ASL_EXTERN BOOLEAN                  ASL_INIT_GLOBAL (Gbl_AsmIncludeOutputFlag, FALSE);
ASL_EXTERN BOOLEAN                  ASL_INIT_GLOBAL (Gbl_C_IncludeOutputFlag, FALSE);
ASL_EXTERN BOOLEAN                  ASL_INIT_GLOBAL (Gbl_ListingFlag, FALSE);
ASL_EXTERN BOOLEAN                  ASL_INIT_GLOBAL (Gbl_IgnoreErrors, FALSE);
ASL_EXTERN BOOLEAN                  ASL_INIT_GLOBAL (Gbl_SourceOutputFlag, FALSE);
ASL_EXTERN BOOLEAN                  ASL_INIT_GLOBAL (Gbl_ParseOnlyFlag, FALSE);
ASL_EXTERN BOOLEAN                  ASL_INIT_GLOBAL (Gbl_CompileTimesFlag, FALSE);
ASL_EXTERN BOOLEAN                  ASL_INIT_GLOBAL (Gbl_FoldConstants, TRUE);
ASL_EXTERN BOOLEAN                  ASL_INIT_GLOBAL (Gbl_VerboseErrors, TRUE);
ASL_EXTERN BOOLEAN                  ASL_INIT_GLOBAL (Gbl_NoErrors, FALSE);
ASL_EXTERN BOOLEAN                  ASL_INIT_GLOBAL (Gbl_WarningsAsErrors, FALSE);
ASL_EXTERN BOOLEAN                  ASL_INIT_GLOBAL (Gbl_NoResourceChecking, FALSE);
ASL_EXTERN BOOLEAN                  ASL_INIT_GLOBAL (Gbl_IntegerOptimizationFlag, TRUE);
ASL_EXTERN BOOLEAN                  ASL_INIT_GLOBAL (Gbl_ReferenceOptimizationFlag, TRUE);
ASL_EXTERN BOOLEAN                  ASL_INIT_GLOBAL (Gbl_DisplayRemarks, TRUE);
ASL_EXTERN BOOLEAN                  ASL_INIT_GLOBAL (Gbl_DisplayWarnings, TRUE);
ASL_EXTERN BOOLEAN                  ASL_INIT_GLOBAL (Gbl_DisplayOptimizations, FALSE);
ASL_EXTERN UINT8                    ASL_INIT_GLOBAL (Gbl_WarningLevel, ASL_WARNING);
ASL_EXTERN BOOLEAN                  ASL_INIT_GLOBAL (Gbl_UseOriginalCompilerId, FALSE);
ASL_EXTERN BOOLEAN                  ASL_INIT_GLOBAL (Gbl_VerboseTemplates, FALSE);
ASL_EXTERN BOOLEAN                  ASL_INIT_GLOBAL (Gbl_DoTemplates, FALSE);
ASL_EXTERN BOOLEAN                  ASL_INIT_GLOBAL (Gbl_CompileGeneric, FALSE);
ASL_EXTERN BOOLEAN                  ASL_INIT_GLOBAL (Gbl_AllExceptionsDisabled, FALSE);
ASL_EXTERN BOOLEAN                  ASL_INIT_GLOBAL (Gbl_PruneParseTree, FALSE);
ASL_EXTERN BOOLEAN                  ASL_INIT_GLOBAL (Gbl_DoTypechecking, TRUE);
ASL_EXTERN BOOLEAN                  ASL_INIT_GLOBAL (Gbl_EnableReferenceTypechecking, FALSE);
ASL_EXTERN BOOLEAN                  ASL_INIT_GLOBAL (Gbl_DoExternals, TRUE);
ASL_EXTERN BOOLEAN                  ASL_INIT_GLOBAL (Gbl_DoExternalsInPlace, FALSE);
ASL_EXTERN BOOLEAN                  ASL_INIT_GLOBAL (Gbl_DoAslConversion, FALSE);
ASL_EXTERN BOOLEAN                  ASL_INIT_GLOBAL (Gbl_OptimizeTrivialParseNodes, TRUE);


#define HEX_OUTPUT_NONE             0
#define HEX_OUTPUT_C                1
#define HEX_OUTPUT_ASM              2
#define HEX_OUTPUT_ASL              3

ASL_EXTERN BOOLEAN                  ASL_INIT_GLOBAL (Gbl_HexOutputFlag, HEX_OUTPUT_NONE);


/* Files */

ASL_EXTERN char                     *Gbl_DirectoryPath;
ASL_EXTERN char                     ASL_INIT_GLOBAL (*Gbl_IncludeFilename, NULL);
ASL_EXTERN char                     ASL_INIT_GLOBAL (*Gbl_OutputFilenamePrefix, NULL);
ASL_EXTERN ASL_INCLUDE_DIR          ASL_INIT_GLOBAL (*Gbl_IncludeDirList, NULL);
ASL_EXTERN char                     *Gbl_CurrentInputFilename;
ASL_EXTERN char                     ASL_INIT_GLOBAL (*Gbl_ExternalRefFilename, NULL);
ASL_EXTERN char                     ASL_INIT_GLOBAL (*Gbl_PreviousIncludeFilename, NULL);

ASL_EXTERN BOOLEAN                  ASL_INIT_GLOBAL (Gbl_HasIncludeFiles, FALSE);


/* Statistics */

ASL_EXTERN UINT32                   ASL_INIT_GLOBAL (Gbl_InputByteCount, 0);
ASL_EXTERN UINT32                   ASL_INIT_GLOBAL (Gbl_InputFieldCount, 0);
ASL_EXTERN UINT32                   ASL_INIT_GLOBAL (Gbl_NsLookupCount, 0);
ASL_EXTERN UINT32                   ASL_INIT_GLOBAL (TotalKeywords, 0);
ASL_EXTERN UINT32                   ASL_INIT_GLOBAL (TotalNamedObjects, 0);
ASL_EXTERN UINT32                   ASL_INIT_GLOBAL (TotalExecutableOpcodes, 0);
ASL_EXTERN UINT32                   ASL_INIT_GLOBAL (TotalParseNodes, 0);
ASL_EXTERN UINT32                   ASL_INIT_GLOBAL (TotalMethods, 0);
ASL_EXTERN UINT32                   ASL_INIT_GLOBAL (TotalAllocations, 0);
ASL_EXTERN UINT32                   ASL_INIT_GLOBAL (TotalAllocated, 0);
ASL_EXTERN UINT32                   ASL_INIT_GLOBAL (TotalFolds, 0);


/* Local caches */

ASL_EXTERN UINT32                   ASL_INIT_GLOBAL (Gbl_ParseOpCount, 0);
ASL_EXTERN ASL_CACHE_INFO           ASL_INIT_GLOBAL (*Gbl_ParseOpCacheList, NULL);
ASL_EXTERN ACPI_PARSE_OBJECT        ASL_INIT_GLOBAL (*Gbl_ParseOpCacheNext, NULL);
ASL_EXTERN ACPI_PARSE_OBJECT        ASL_INIT_GLOBAL (*Gbl_ParseOpCacheLast, NULL);

ASL_EXTERN UINT32                   ASL_INIT_GLOBAL (Gbl_StringCount, 0);
ASL_EXTERN UINT32                   ASL_INIT_GLOBAL (Gbl_StringSize, 0);
ASL_EXTERN ASL_CACHE_INFO           ASL_INIT_GLOBAL (*Gbl_StringCacheList, NULL);
ASL_EXTERN char                     ASL_INIT_GLOBAL (*Gbl_StringCacheNext, NULL);
ASL_EXTERN char                     ASL_INIT_GLOBAL (*Gbl_StringCacheLast, NULL);

/* Map file */

ASL_EXTERN ACPI_GPIO_INFO           ASL_INIT_GLOBAL (*Gbl_GpioList, NULL);
ASL_EXTERN ACPI_SERIAL_INFO         ASL_INIT_GLOBAL (*Gbl_SerialList, NULL);


/* Misc */

ASL_EXTERN UINT8                    ASL_INIT_GLOBAL (Gbl_RevisionOverride, 0);
ASL_EXTERN UINT8                    ASL_INIT_GLOBAL (Gbl_TempCount, 0);
ASL_EXTERN ACPI_PARSE_OBJECT        ASL_INIT_GLOBAL (*Gbl_ParseTreeRoot, NULL);
ASL_EXTERN ACPI_PARSE_OBJECT        ASL_INIT_GLOBAL (*Gbl_ExternalsListHead, NULL);
ASL_EXTERN UINT32                   ASL_INIT_GLOBAL (Gbl_TableLength, 0);
ASL_EXTERN UINT32                   ASL_INIT_GLOBAL (Gbl_SourceLine, 0);
ASL_EXTERN ASL_LISTING_NODE         ASL_INIT_GLOBAL (*Gbl_ListingNode, NULL);
ASL_EXTERN UINT8                    ASL_INIT_GLOBAL (Gbl_FileType, 0);
ASL_EXTERN char                     ASL_INIT_GLOBAL (*Gbl_Signature, NULL);

ASL_EXTERN ACPI_PARSE_OBJECT        *Gbl_FirstLevelInsertionNode;

ASL_EXTERN UINT32                   ASL_INIT_GLOBAL (Gbl_CurrentHexColumn, 0);
ASL_EXTERN UINT32                   ASL_INIT_GLOBAL (Gbl_CurrentAmlOffset, 0);
ASL_EXTERN UINT32                   ASL_INIT_GLOBAL (Gbl_CurrentLine, 0);
ASL_EXTERN UINT32                   ASL_INIT_GLOBAL (Gbl_DisabledMessagesIndex, 0);
ASL_EXTERN UINT32                   ASL_INIT_GLOBAL (Gbl_ExpectedMessagesIndex, 0);
ASL_EXTERN UINT8                    ASL_INIT_GLOBAL (Gbl_HexBytesWereWritten, FALSE);
ASL_EXTERN UINT32                   ASL_INIT_GLOBAL (Gbl_NumNamespaceObjects, 0);
ASL_EXTERN UINT32                   ASL_INIT_GLOBAL (Gbl_ReservedMethods, 0);
ASL_EXTERN char                     ASL_INIT_GLOBAL (*Gbl_TableSignature, "NO_SIG");
ASL_EXTERN char                     ASL_INIT_GLOBAL (*Gbl_TableId, "NO_ID");
ASL_EXTERN UINT8                    ASL_INIT_GLOBAL (Gbl_PruneDepth, 0);
ASL_EXTERN UINT16                   ASL_INIT_GLOBAL (Gbl_PruneType, 0);

ASL_EXTERN ASL_FILE_NODE            ASL_INIT_GLOBAL (*Gbl_IncludeFileStack, NULL);

/* Specific to the -q option */

ASL_EXTERN ASL_COMMENT_STATE        Gbl_CommentState;


/*
 * Determines if an inline comment should be saved in the InlineComment or NodeEndComment
 *  field of ACPI_PARSE_OBJECT.
 */
ASL_EXTERN ACPI_COMMENT_NODE        ASL_INIT_GLOBAL (*Gbl_CommentListHead, NULL);
ASL_EXTERN ACPI_COMMENT_NODE        ASL_INIT_GLOBAL (*Gbl_CommentListTail, NULL);
ASL_EXTERN char                     ASL_INIT_GLOBAL (*Gbl_InlineCommentBuffer, NULL);

/* Static structures */

ASL_EXTERN ASL_ANALYSIS_WALK_INFO   AnalysisWalkInfo;
ASL_EXTERN ACPI_TABLE_HEADER        TableHeader;

/* Event timing */

#define ASL_NUM_EVENTS              24
ASL_EXTERN ASL_EVENT_INFO           AslGbl_Events[ASL_NUM_EVENTS];
ASL_EXTERN UINT8                    AslGbl_NextEvent;
ASL_EXTERN UINT8                    AslGbl_NamespaceEvent;

/* Scratch buffers */

ASL_EXTERN UINT8                    Gbl_AmlBuffer[HEX_LISTING_LINE_SIZE];
ASL_EXTERN char                     MsgBuffer[ASL_MSG_BUFFER_SIZE];
ASL_EXTERN char                     StringBuffer[ASL_MSG_BUFFER_SIZE];
ASL_EXTERN char                     StringBuffer2[ASL_MSG_BUFFER_SIZE];
ASL_EXTERN UINT32                   Gbl_DisabledMessages[ASL_MAX_DISABLED_MESSAGES];
ASL_EXTERN ASL_EXPECTED_MESSAGE     Gbl_ExpectedMessages[ASL_MAX_EXPECTED_MESSAGES];


#endif /* __ASLGLOBAL_H */
