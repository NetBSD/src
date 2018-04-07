/******************************************************************************
 *
 * Module Name: aslmain - compiler main and utilities
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

#define _DECLARE_GLOBALS

#include "aslcompiler.h"
#include "acapps.h"
#include "acdisasm.h"
#include <signal.h>

#define _COMPONENT          ACPI_COMPILER
        ACPI_MODULE_NAME    ("aslmain")

/*
 * Main routine for the iASL compiler.
 *
 * Portability note: The compiler depends upon the host for command-line
 * wildcard support - it is not implemented locally. For example:
 *
 * Linux/Unix systems: Shell expands wildcards automatically.
 *
 * Windows: The setargv.obj module must be linked in to automatically
 * expand wildcards.
 */

/* Local prototypes */

static void ACPI_SYSTEM_XFACE
AslSignalHandler (
    int                     Sig);

static void
AslInitialize (
    void);


/*******************************************************************************
 *
 * FUNCTION:    main
 *
 * PARAMETERS:  Standard argc/argv
 *
 * RETURN:      Program termination code
 *
 * DESCRIPTION: C main routine for the iASL Compiler/Disassembler. Process
 *  command line options and begin the compile/disassembly for each file on
 *  the command line (wildcards supported).
 *
 ******************************************************************************/

int ACPI_SYSTEM_XFACE
main (
    int                     argc,
    char                    **argv)
{
    ACPI_STATUS             Status;
    int                     Index1;
    int                     Index2;
    int                     ReturnStatus = 0;


    signal (SIGINT, AslSignalHandler);
    signal (SIGSEGV, AslSignalHandler);

    /*
     * Big-endian machines are not currently supported. ACPI tables must
     * be little-endian, and support for big-endian machines needs to
     * be implemented.
     */
    if (UtIsBigEndianMachine ())
    {
        fprintf (stderr,
            "iASL is not currently supported on big-endian machines.\n");
        return (-1);
    }

    AcpiOsInitialize ();
    ACPI_DEBUG_INITIALIZE (); /* For debug version only */

    /* Initialize preprocessor and compiler before command line processing */

    AcpiGbl_ExternalFileList = NULL;
    AcpiDbgLevel = 0;
    PrInitializePreprocessor ();
    AslInitialize ();

    Index1 = Index2 =
        AslCommandLine (argc, argv);

    /* Allocate the line buffer(s), must be after command line */

    Gbl_LineBufferSize /= 2;
    UtExpandLineBuffers ();

    /* Perform global actions first/only */

    if (Gbl_DisassembleAll)
    {
        while (argv[Index1])
        {
            Status = AcpiDmAddToExternalFileList (argv[Index1]);
            if (ACPI_FAILURE (Status))
            {
                return (-1);
            }

            Index1++;
        }
    }


    /* Process each pathname/filename in the list, with possible wildcards */

    while (argv[Index2])
    {
        /*
         * If -p not specified, we will use the input filename as the
         * output filename prefix
         */
        if (Gbl_UseDefaultAmlFilename)
        {
            Gbl_OutputFilenamePrefix = argv[Index2];
            UtConvertBackslashes (Gbl_OutputFilenamePrefix);
        }

        Status = AslDoOneFile (argv[Index2]);
        if (ACPI_FAILURE (Status))
        {
            ReturnStatus = -1;
            goto CleanupAndExit;
        }

        Index2++;
    }


CleanupAndExit:

    UtFreeLineBuffers ();
    AslParserCleanup ();

    if (AcpiGbl_ExternalFileList)
    {
        AcpiDmClearExternalFileList();
    }

    return (ReturnStatus);
}


/******************************************************************************
 *
 * FUNCTION:    AslSignalHandler
 *
 * PARAMETERS:  Sig                 - Signal that invoked this handler
 *
 * RETURN:      None
 *
 * DESCRIPTION: Signal interrupt handler. Delete any intermediate files and
 *              any output files that may be left in an indeterminate state.
 *              Currently handles SIGINT (control-c) and SIGSEGV (segmentation
 *              fault).
 *
 *****************************************************************************/

static void ACPI_SYSTEM_XFACE
AslSignalHandler (
    int                     Sig)
{
    UINT32                  i;


    signal (Sig, SIG_IGN);
    fflush (stdout);
    fflush (stderr);

    switch (Sig)
    {
    case SIGINT:

        printf ("\n" ASL_PREFIX "<Control-C>\n");
        break;

    case SIGSEGV:

        /* Even on a seg fault, we will try to delete any partial files */

        printf (ASL_PREFIX "Segmentation Fault\n");
        break;

    default:

        printf (ASL_PREFIX "Unknown interrupt signal (%u), ignoring\n", Sig);
        return;
    }

    /*
     * Close all open files
     * Note: the .pre file is the same as the input source file
     */
    Gbl_Files[ASL_FILE_PREPROCESSOR].Handle = NULL;

    for (i = ASL_FILE_INPUT; i < ASL_MAX_FILE_TYPE; i++)
    {
        FlCloseFile (i);
    }

    /* Delete any output files */

    for (i = ASL_FILE_AML_OUTPUT; i < ASL_MAX_FILE_TYPE; i++)
    {
        FlDeleteFile (i);
    }

    printf (ASL_PREFIX "Terminating\n");
    exit (0);
}


/*******************************************************************************
 *
 * FUNCTION:    AslInitialize
 *
 * PARAMETERS:  None
 *
 * RETURN:      None
 *
 * DESCRIPTION: Initialize compiler globals
 *
 ******************************************************************************/

static void
AslInitialize (
    void)
{
    UINT32                  i;


    AcpiGbl_DmOpt_Verbose = FALSE;

    /* Default integer width is 32 bits */

    AcpiGbl_IntegerBitWidth = 32;
    AcpiGbl_IntegerNybbleWidth = 8;
    AcpiGbl_IntegerByteWidth = 4;

    for (i = 0; i < ASL_NUM_FILES; i++)
    {
        Gbl_Files[i].Handle = NULL;
        Gbl_Files[i].Filename = NULL;
    }

    Gbl_Files[ASL_FILE_STDOUT].Handle   = stdout;
    Gbl_Files[ASL_FILE_STDOUT].Filename = "STDOUT";

    Gbl_Files[ASL_FILE_STDERR].Handle   = stderr;
    Gbl_Files[ASL_FILE_STDERR].Filename = "STDERR";
}
