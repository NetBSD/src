
/******************************************************************************
 *
 * Module Name: aslstartup - Compiler startup routines, called from main
 *
 *****************************************************************************/

/*
 * Copyright (C) 2000 - 2011, Intel Corp.
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
#include "acapps.h"

#define _COMPONENT          ACPI_COMPILER
        ACPI_MODULE_NAME    ("aslstartup")


#define ASL_MAX_FILES   256
static char             *FileList[ASL_MAX_FILES];
static BOOLEAN          AslToFile = TRUE;


/* Local prototypes */

static char **
AsDoWildcard (
    char                    *DirectoryPathname,
    char                    *FileSpecifier);

static UINT8
AslDetectSourceFileType (
    ASL_FILE_INFO           *Info);


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

    Gbl_CurrentColumn = 0;
    Gbl_CurrentLineNumber = 1;
    Gbl_LogicalLineNumber = 1;
    Gbl_CurrentLineOffset = 0;
    Gbl_InputFieldCount = 0;
    Gbl_LineBufPtr = Gbl_CurrentLineBuffer;

    Gbl_ErrorLog = NULL;
    Gbl_NextError = NULL;
    Gbl_Signature = NULL;
    Gbl_FileType = 0;

    AslGbl_NextEvent = 0;
    for (i = 0; i < ASL_NUM_REPORT_LEVELS; i++)
    {
        Gbl_ExceptionCount[i] = 0;
    }

    Gbl_Files[ASL_FILE_AML_OUTPUT].Filename = NULL;
    Gbl_Files[ASL_FILE_AML_OUTPUT].Handle = NULL;

    Gbl_Files[ASL_FILE_SOURCE_OUTPUT].Filename = NULL;
    Gbl_Files[ASL_FILE_SOURCE_OUTPUT].Handle = NULL;
}


/******************************************************************************
 *
 * FUNCTION:    AsDoWildcard
 *
 * PARAMETERS:  None
 *
 * RETURN:      None
 *
 * DESCRIPTION: Process files via wildcards. This function is for the Windows
 *              case only.
 *
 ******************************************************************************/

static char **
AsDoWildcard (
    char                    *DirectoryPathname,
    char                    *FileSpecifier)
{
#ifdef WIN32
    void                    *DirInfo;
    char                    *Filename;
    int                     FileCount;


    FileCount = 0;

    /* Open parent directory */

    DirInfo = AcpiOsOpenDirectory (DirectoryPathname, FileSpecifier, REQUEST_FILE_ONLY);
    if (!DirInfo)
    {
        /* Either the directory of file does not exist */

        Gbl_Files[ASL_FILE_INPUT].Filename = FileSpecifier;
        FlFileError (ASL_FILE_INPUT, ASL_MSG_OPEN);
        AslAbort ();
    }

    /* Process each file that matches the wildcard specification */

    while ((Filename = AcpiOsGetNextFilename (DirInfo)))
    {
        /* Add the filename to the file list */

        FileList[FileCount] = AcpiOsAllocate (strlen (Filename) + 1);
        strcpy (FileList[FileCount], Filename);
        FileCount++;

        if (FileCount >= ASL_MAX_FILES)
        {
            printf ("Max files reached\n");
            FileList[0] = NULL;
            return (FileList);
        }
    }

    /* Cleanup */

    AcpiOsCloseDirectory (DirInfo);
    FileList[FileCount] = NULL;
    return (FileList);

#else
    /*
     * Linux/Unix cases - Wildcards are expanded by the shell automatically.
     * Just return the filename in a null terminated list
     */
    FileList[0] = AcpiOsAllocate (strlen (FileSpecifier) + 1);
    strcpy (FileList[0], FileSpecifier);
    FileList[1] = NULL;

    return (FileList);
#endif
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
    UINT8                   Type;
    ACPI_STATUS             Status;


    /* Check for 100% ASCII source file (comments are ignored) */

    Status = FlCheckForAscii (Info);
    if (ACPI_FAILURE (Status))
    {
        printf ("Non-ascii input file - %s\n", Info->Filename);
        Type = ASL_INPUT_TYPE_BINARY;
        goto Cleanup;
    }

    /*
     * File is ASCII. Determine if this is an ASL file or an ACPI data
     * table file.
     */
    while (fgets (Gbl_CurrentLineBuffer, ASL_LINE_BUFFER_SIZE, Info->Handle))
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

    /* Not an ASL source file, default to a data table source file */

    Type = ASL_INPUT_TYPE_ASCII_DATA;

Cleanup:

    /* Must seek back to the start of the file */

    fseek (Info->Handle, 0, SEEK_SET);
    return (Type);
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


    Gbl_Files[ASL_FILE_INPUT].Filename = Filename;

    /* Re-initialize "some" compiler globals */

    AslInitializeGlobals ();

    /*
     * AML Disassembly (Optional)
     */
    if (Gbl_DisasmFlag || Gbl_GetAllTables)
    {
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

        /* This is where the disassembly happens */

        AcpiGbl_DbOpt_disasm = TRUE;
        Status = AdAmlDisassemble (AslToFile,
                    Gbl_Files[ASL_FILE_INPUT].Filename,
                    Gbl_OutputFilenamePrefix,
                    &Gbl_Files[ASL_FILE_INPUT].Filename,
                    Gbl_GetAllTables);
        if (ACPI_FAILURE (Status))
        {
            return (Status);
        }

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
        }
        else
        {
            Gbl_Files[ASL_FILE_INPUT].Filename = NULL;
            return (AE_OK);
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

        if (Gbl_Signature)
        {
            ACPI_FREE (Gbl_Signature);
            Gbl_Signature = NULL;
        }
        AeClearErrorLog ();
        return (Status);

    /*
     * ASL Compilation (Optional)
     */
    case ASL_INPUT_TYPE_ASCII_ASL:

        /* ACPICA subsystem initialization */

        Status = AdInitialize ();
        if (ACPI_FAILURE (Status))
        {
            return (Status);
        }

        Status = CmDoCompile ();
        (void) AcpiTerminate ();

        /*
         * Return non-zero exit code if there have been errors, unless the
         * global ignore error flag has been set
         */
        if ((Gbl_ExceptionCount[ASL_ERROR] > 0) && (!Gbl_IgnoreErrors))
        {
            return (AE_ERROR);
        }

        AeClearErrorLog ();
        return (AE_OK);

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
 * FUNCTION:    AslDoOnePathname
 *
 * PARAMETERS:  Pathname            - Full pathname, possibly with wildcards
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Process one pathname, possible terminated with a wildcard
 *              specification. If a wildcard, it is expanded and the multiple
 *              files are processed.
 *
 ******************************************************************************/

ACPI_STATUS
AslDoOnePathname (
    char                    *Pathname,
    ASL_PATHNAME_CALLBACK   PathCallback)
{
    ACPI_STATUS             Status = AE_OK;
    char                    **WildcardList;
    char                    *Filename;
    char                    *FullPathname;


    /* Split incoming path into a directory/filename combo */

    Status = FlSplitInputPathname (Pathname, &Gbl_DirectoryPath, &Filename);
    if (ACPI_FAILURE (Status))
    {
        return (Status);
    }

    /* Expand possible wildcard into a file list (Windows/DOS only) */

    WildcardList = AsDoWildcard (Gbl_DirectoryPath, Filename);
    while (*WildcardList)
    {
        FullPathname = ACPI_ALLOCATE (
            strlen (Gbl_DirectoryPath) + strlen (*WildcardList) + 1);

        /* Construct a full path to the file */

        strcpy (FullPathname, Gbl_DirectoryPath);
        strcat (FullPathname, *WildcardList);

        /*
         * If -p not specified, we will use the input filename as the
         * output filename prefix
         */
        if (Gbl_UseDefaultAmlFilename)
        {
            Gbl_OutputFilenamePrefix = FullPathname;
        }

        /* Save status from all compiles */

        Status |= (*PathCallback) (FullPathname);

        ACPI_FREE (FullPathname);
        ACPI_FREE (*WildcardList);
        *WildcardList = NULL;
        WildcardList++;
    }

    ACPI_FREE (Gbl_DirectoryPath);
    ACPI_FREE (Filename);
    return (Status);
}

