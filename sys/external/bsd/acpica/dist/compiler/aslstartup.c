/******************************************************************************
 *
 * Module Name: aslstartup - Compiler startup routines, called from main
 *
 *****************************************************************************/

/*
 * Copyright (C) 2000 - 2017, Intel Corp.
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
#include "actables.h"
#include "acdisasm.h"
#include "acapps.h"
#include "acconvert.h"

#define _COMPONENT          ACPI_COMPILER
        ACPI_MODULE_NAME    ("aslstartup")


/* Local prototypes */

static UINT8
AslDetectSourceFileType (
    ASL_FILE_INFO           *Info);

static ACPI_STATUS
AslDoDisassembly (
    void);


/* Globals */

static BOOLEAN          AslToFile = TRUE;


/*******************************************************************************
 *
 * FUNCTION:    AslInitializeGlobals
 *
 * PARAMETERS:  None
 *
 * RETURN:      None
 *
 * DESCRIPTION: Re-initialize globals needed to restart the compiler. This
 *              allows multiple files to be disassembled and/or compiled.
 *
 ******************************************************************************/

void
AslInitializeGlobals (
    void)
{
    UINT32                  i;


    /* Init compiler globals */

    Gbl_SyntaxError = 0;
    Gbl_CurrentColumn = 0;
    Gbl_CurrentLineNumber = 1;
    Gbl_LogicalLineNumber = 1;
    Gbl_CurrentLineOffset = 0;
    Gbl_InputFieldCount = 0;
    Gbl_InputByteCount = 0;
    Gbl_NsLookupCount = 0;
    Gbl_LineBufPtr = Gbl_CurrentLineBuffer;

    Gbl_ErrorLog = NULL;
    Gbl_NextError = NULL;
    Gbl_Signature = NULL;
    Gbl_FileType = 0;

    TotalExecutableOpcodes = 0;
    TotalNamedObjects = 0;
    TotalKeywords = 0;
    TotalParseNodes = 0;
    TotalMethods = 0;
    TotalAllocations = 0;
    TotalAllocated = 0;
    TotalFolds = 0;

    AslGbl_NextEvent = 0;
    for (i = 0; i < ASL_NUM_REPORT_LEVELS; i++)
    {
        Gbl_ExceptionCount[i] = 0;
    }

    for (i = ASL_FILE_INPUT; i <= ASL_MAX_FILE_TYPE; i++)
    {
        Gbl_Files[i].Handle = NULL;
        Gbl_Files[i].Filename = NULL;
    }

    if (Gbl_CaptureComments)
    {
        Gbl_CommentState.SpacesBefore          = 0;
        Gbl_CommentState.CommentType           = 1;
        Gbl_CommentState.Latest_Parse_Node     = NULL;
        Gbl_CommentState.ParsingParenBraceNode = NULL;
        Gbl_CommentState.CaptureComments       = TRUE;
    }
}


/*******************************************************************************
 *
 * FUNCTION:    AslDetectSourceFileType
 *
 * PARAMETERS:  Info            - Name/Handle for the file (must be open)
 *
 * RETURN:      File Type
 *
 * DESCRIPTION: Determine the type of the input file. Either binary (contains
 *              non-ASCII characters), ASL file, or an ACPI Data Table file.
 *
 ******************************************************************************/

static UINT8
AslDetectSourceFileType (
    ASL_FILE_INFO           *Info)
{
    char                    *FileChar;
    UINT8                   Type = ASL_INPUT_TYPE_ASCII_DATA; /* default */
    ACPI_STATUS             Status;


    /* Check for 100% ASCII source file (comments are ignored) */

    Status = FlIsFileAsciiSource (Info->Filename, FALSE);
    if (ACPI_SUCCESS (Status))
    {
        /*
         * File contains ASCII source code. Determine if this is an ASL
         * file or an ACPI data table file.
         */
        while (fgets (Gbl_CurrentLineBuffer, Gbl_LineBufferSize, Info->Handle))
        {
            /* Uppercase the buffer for caseless compare */

            FileChar = Gbl_CurrentLineBuffer;
            while (*FileChar)
            {
                *FileChar = (char) toupper ((int) *FileChar);
                FileChar++;
            }

            /* Presence of "DefinitionBlock" indicates actual ASL code */

            if (strstr (Gbl_CurrentLineBuffer, "DEFINITIONBLOCK"))
            {
                /* Appears to be an ASL file */

                Type = ASL_INPUT_TYPE_ASCII_ASL;
                goto Cleanup;
            }
        }

        /* Appears to be an ASCII data table source file */

        Type = ASL_INPUT_TYPE_ASCII_DATA;
        goto Cleanup;
    }

    /* We have some sort of binary table, check for valid ACPI table */

    fseek (Info->Handle, 0, SEEK_SET);

    Status = AcValidateTableHeader (Info->Handle, 0);
    if (ACPI_SUCCESS (Status))
    {
        fprintf (stderr,
            "Binary file appears to be a valid ACPI table, disassembling\n");

        Type = ASL_INPUT_TYPE_BINARY_ACPI_TABLE;
        goto Cleanup;
    }

    Type = ASL_INPUT_TYPE_BINARY;


Cleanup:

    /* Must seek back to the start of the file */

    fseek (Info->Handle, 0, SEEK_SET);
    return (Type);
}


/*******************************************************************************
 *
 * FUNCTION:    AslDoDisassembly
 *
 * PARAMETERS:  None
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Initiate AML file disassembly. Uses ACPICA subsystem to build
 *              namespace.
 *
 ******************************************************************************/

static ACPI_STATUS
AslDoDisassembly (
    void)
{
    ACPI_STATUS             Status;


    /* ACPICA subsystem initialization */

    Status = AdInitialize ();
    if (ACPI_FAILURE (Status))
    {
        return (Status);
    }

    Status = AcpiAllocateRootTable (4);
    if (ACPI_FAILURE (Status))
    {
        AcpiOsPrintf ("Could not initialize ACPI Table Manager, %s\n",
            AcpiFormatException (Status));
        return (Status);
    }

    /* Handle additional output files for disassembler */

    Gbl_FileType = ASL_INPUT_TYPE_BINARY_ACPI_TABLE;
    Status = FlOpenMiscOutputFiles (Gbl_OutputFilenamePrefix);

    /* This is where the disassembly happens */

    AcpiGbl_DmOpt_Disasm = TRUE;
    Status = AdAmlDisassemble (AslToFile,
        Gbl_Files[ASL_FILE_INPUT].Filename, Gbl_OutputFilenamePrefix,
        &Gbl_Files[ASL_FILE_INPUT].Filename);
    if (ACPI_FAILURE (Status))
    {
        return (Status);
    }

    /* Check if any control methods were unresolved */

    AcpiDmUnresolvedWarning (0);

    /* Shutdown compiler and ACPICA subsystem */

    AeClearErrorLog ();
    (void) AcpiTerminate ();

    /*
     * Gbl_Files[ASL_FILE_INPUT].Filename was replaced with the
     * .DSL disassembly file, which can now be compiled if requested
     */
    if (Gbl_DoCompile)
    {
        AcpiOsPrintf ("\nCompiling \"%s\"\n",
            Gbl_Files[ASL_FILE_INPUT].Filename);
        return (AE_CTRL_CONTINUE);
    }

    /* No need to free the filename string */

    Gbl_Files[ASL_FILE_INPUT].Filename = NULL;

    CmDeleteCaches ();
    return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AslDoOneFile
 *
 * PARAMETERS:  Filename        - Name of the file
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Process a single file - either disassemble, compile, or both
 *
 ******************************************************************************/

ACPI_STATUS
AslDoOneFile (
    char                    *Filename)
{
    ACPI_STATUS             Status;


    /* Re-initialize "some" compiler/preprocessor globals */

    AslInitializeGlobals ();
    PrInitializeGlobals ();

    /*
     * Extract the directory path. This path is used for possible include
     * files and the optional AML filename embedded in the input file
     * DefinitionBlock declaration.
     */
    Status = FlSplitInputPathname (Filename, &Gbl_DirectoryPath, NULL);
    if (ACPI_FAILURE (Status))
    {
        return (Status);
    }

    /* Take a copy of the input filename, convert any backslashes */

    Gbl_Files[ASL_FILE_INPUT].Filename =
        UtStringCacheCalloc (strlen (Filename) + 1);

    strcpy (Gbl_Files[ASL_FILE_INPUT].Filename, Filename);
    UtConvertBackslashes (Gbl_Files[ASL_FILE_INPUT].Filename);

    /*
     * AML Disassembly (Optional)
     */
    if (Gbl_DisasmFlag)
    {
        Status = AslDoDisassembly ();
        if (Status != AE_CTRL_CONTINUE)
        {
            return (Status);
        }
    }

    /*
     * Open the input file. Here, this should be an ASCII source file,
     * either an ASL file or a Data Table file
     */
    Status = FlOpenInputFile (Gbl_Files[ASL_FILE_INPUT].Filename);
    if (ACPI_FAILURE (Status))
    {
        AePrintErrorLog (ASL_FILE_STDERR);
        return (AE_ERROR);
    }

    Gbl_OriginalInputFileSize = FlGetFileSize (ASL_FILE_INPUT);

    /* Determine input file type */

    Gbl_FileType = AslDetectSourceFileType (&Gbl_Files[ASL_FILE_INPUT]);
    if (Gbl_FileType == ASL_INPUT_TYPE_BINARY)
    {
        return (AE_ERROR);
    }

    /*
     * If -p not specified, we will use the input filename as the
     * output filename prefix
     */
    if (Gbl_UseDefaultAmlFilename)
    {
        Gbl_OutputFilenamePrefix = Gbl_Files[ASL_FILE_INPUT].Filename;
    }

    /* Open the optional output files (listings, etc.) */

    Status = FlOpenMiscOutputFiles (Gbl_OutputFilenamePrefix);
    if (ACPI_FAILURE (Status))
    {
        AePrintErrorLog (ASL_FILE_STDERR);
        return (AE_ERROR);
    }

    /*
     * Compilation of ASL source versus DataTable source uses different
     * compiler subsystems
     */
    switch (Gbl_FileType)
    {
    /*
     * Data Table Compilation
     */
    case ASL_INPUT_TYPE_ASCII_DATA:

        Status = DtDoCompile ();
        if (ACPI_FAILURE (Status))
        {
            return (Status);
        }

        if (Gbl_Signature)
        {
            Gbl_Signature = NULL;
        }

        /* Check if any errors occurred during compile */

        Status = AslCheckForErrorExit ();
        if (ACPI_FAILURE (Status))
        {
            return (Status);
        }

        /* Cleanup (for next source file) and exit */

        AeClearErrorLog ();
        PrTerminatePreprocessor ();
        return (Status);

    /*
     * ASL Compilation
     */
    case ASL_INPUT_TYPE_ASCII_ASL:

        /* ACPICA subsystem initialization */

        Status = AdInitialize ();
        if (ACPI_FAILURE (Status))
        {
            return (Status);
        }

        (void) CmDoCompile ();
        (void) AcpiTerminate ();

        /* Check if any errors occurred during compile */

        Status = AslCheckForErrorExit ();
        if (ACPI_FAILURE (Status))
        {
            return (Status);
        }

        /* Cleanup (for next source file) and exit */

        AeClearErrorLog ();
        PrTerminatePreprocessor ();

        /* ASL-to-ASL+ conversion - Perform immediate disassembly */

        if (Gbl_DoAslConversion)
        {
            /*
             * New input file is the output AML file from above.
             * New output is from the input ASL file from above.
             */
            Gbl_OutputFilenamePrefix = Gbl_Files[ASL_FILE_INPUT].Filename;
        CvDbgPrint ("OUTPUTFILENAME: %s\n", Gbl_OutputFilenamePrefix);
            Gbl_Files[ASL_FILE_INPUT].Filename =
                Gbl_Files[ASL_FILE_AML_OUTPUT].Filename;

            fprintf (stderr, "\n");
            AslDoDisassembly ();

            /* delete the AML file. This AML file should never be utilized by AML interpreters. */

            FlDeleteFile (ASL_FILE_AML_OUTPUT);
        }

        return (AE_OK);

    /*
     * Binary ACPI table was auto-detected, disassemble it
     */
    case ASL_INPUT_TYPE_BINARY_ACPI_TABLE:

        /* We have what appears to be an ACPI table, disassemble it */

        FlCloseFile (ASL_FILE_INPUT);
        Gbl_DoCompile = FALSE;
        Gbl_DisasmFlag = TRUE;
        Status = AslDoDisassembly ();
        return (Status);

    /* Unknown binary table */

    case ASL_INPUT_TYPE_BINARY:

        AePrintErrorLog (ASL_FILE_STDERR);
        return (AE_ERROR);

    default:

        printf ("Unknown file type %X\n", Gbl_FileType);
        return (AE_ERROR);
    }
}


/*******************************************************************************
 *
 * FUNCTION:    AslCheckForErrorExit
 *
 * PARAMETERS:  None. Examines global exception count array
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Determine if compiler should abort with error status
 *
 ******************************************************************************/

ACPI_STATUS
AslCheckForErrorExit (
    void)
{

    /*
     * Return non-zero exit code if there have been errors, unless the
     * global ignore error flag has been set
     */
    if (!Gbl_IgnoreErrors)
    {
        if (Gbl_ExceptionCount[ASL_ERROR] > 0)
        {
            return (AE_ERROR);
        }

        /* Optionally treat warnings as errors */

        if (Gbl_WarningsAsErrors)
        {
            if ((Gbl_ExceptionCount[ASL_WARNING] > 0)  ||
                (Gbl_ExceptionCount[ASL_WARNING2] > 0) ||
                (Gbl_ExceptionCount[ASL_WARNING3] > 0))
            {
                return (AE_ERROR);
            }
        }
    }

    return (AE_OK);
}
