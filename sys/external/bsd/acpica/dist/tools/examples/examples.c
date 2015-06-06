/******************************************************************************
 *
 * Module Name: examples - Example ACPICA initialization and execution code
 *
 *****************************************************************************/

/*
 * Copyright (C) 2000 - 2015, Intel Corp.
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

#include "examples.h"

#define _COMPONENT          ACPI_EXAMPLE
        ACPI_MODULE_NAME    ("examples")


/******************************************************************************
 *
 * ACPICA Example Code
 *
 * This module contains examples of how the host OS should interface to the
 * ACPICA subsystem.
 *
 * 1) How to use the platform/acenv.h file and how to set configuration
 *      options.
 *
 * 2) main - using the debug output mechanism and the error/warning output
 *      macros.
 *
 * 3) Two examples of the ACPICA initialization sequence. The first is a
 *      initialization with no "early" ACPI table access. The second shows
 *      how to use ACPICA to obtain the tables very early during kernel
 *      initialization, even before dynamic memory is available.
 *
 * 4) How to invoke a control method, including argument setup and how to
 *      access the return value.
 *
 *****************************************************************************/


/* Local Prototypes */

static ACPI_STATUS
InitializeFullAcpica (void);

static ACPI_STATUS
InstallHandlers (void);

static void
NotifyHandler (
    ACPI_HANDLE             Device,
    UINT32                  Value,
    void                    *Context);

static ACPI_STATUS
RegionHandler (
    UINT32                  Function,
    ACPI_PHYSICAL_ADDRESS   Address,
    UINT32                  BitWidth,
    UINT64                  *Value,
    void                    *HandlerContext,
    void                    *RegionContext);

static ACPI_STATUS
RegionInit (
    ACPI_HANDLE             RegionHandle,
    UINT32                  Function,
    void                    *HandlerContext,
    void                    **RegionContext);

static void
ExecuteMAIN (void);

static void
ExecuteOSI (void);

ACPI_STATUS
InitializeAcpiTables (
    void);

ACPI_STATUS
InitializeAcpi (
    void);


/******************************************************************************
 *
 * FUNCTION:    main
 *
 * PARAMETERS:  argc, argv
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Main routine. Shows the use of the various output macros, as
 *              well as the use of the debug layer/level globals.
 *
 *****************************************************************************/

int ACPI_SYSTEM_XFACE
main (
    int                     argc,
    char                    **argv)
{

    ACPI_DEBUG_INITIALIZE (); /* For debug version only */

    printf (ACPI_COMMON_SIGNON ("ACPI Example Code"));

    /* Initialize the local ACPI tables (RSDP/RSDT/XSDT/FADT/DSDT/FACS) */

    ExInitializeAcpiTables ();

    /* Initialize the ACPICA subsystem */

    InitializeFullAcpica ();

    /* Example warning and error output */

    ACPI_INFO        ((AE_INFO, "Example ACPICA info message"));
    ACPI_WARNING     ((AE_INFO, "Example ACPICA warning message"));
    ACPI_ERROR       ((AE_INFO, "Example ACPICA error message"));
    ACPI_EXCEPTION   ((AE_INFO, AE_AML_OPERAND_TYPE, "Example ACPICA exception message"));

    ExecuteOSI ();
    ExecuteMAIN ();
    return (0);
}


/******************************************************************************
 *
 * Example ACPICA initialization code. This shows a full initialization with
 * no early ACPI table access.
 *
 *****************************************************************************/

static ACPI_STATUS
InitializeFullAcpica (void)
{
    ACPI_STATUS             Status;


    /* Initialize the ACPICA subsystem */

    Status = AcpiInitializeSubsystem ();
    if (ACPI_FAILURE (Status))
    {
        ACPI_EXCEPTION ((AE_INFO, Status, "While initializing ACPICA"));
        return (Status);
    }

    /* Initialize the ACPICA Table Manager and get all ACPI tables */

    ACPI_INFO ((AE_INFO, "Loading ACPI tables"));

    Status = AcpiInitializeTables (NULL, 16, FALSE);
    if (ACPI_FAILURE (Status))
    {
        ACPI_EXCEPTION ((AE_INFO, Status, "While initializing Table Manager"));
        return (Status);
    }

    /* Create the ACPI namespace from ACPI tables */

    Status = AcpiLoadTables ();
    if (ACPI_FAILURE (Status))
    {
        ACPI_EXCEPTION ((AE_INFO, Status, "While loading ACPI tables"));
        return (Status);
    }

    /* Install local handlers */

    Status = InstallHandlers ();
    if (ACPI_FAILURE (Status))
    {
        ACPI_EXCEPTION ((AE_INFO, Status, "While installing handlers"));
        return (Status);
    }

    /* Initialize the ACPI hardware */

    Status = AcpiEnableSubsystem (ACPI_FULL_INITIALIZATION);
    if (ACPI_FAILURE (Status))
    {
        ACPI_EXCEPTION ((AE_INFO, Status, "While enabling ACPICA"));
        return (Status);
    }

    /* Complete the ACPI namespace object initialization */

    Status = AcpiInitializeObjects (ACPI_FULL_INITIALIZATION);
    if (ACPI_FAILURE (Status))
    {
        ACPI_EXCEPTION ((AE_INFO, Status, "While initializing ACPICA objects"));
        return (Status);
    }

    return (AE_OK);
}


/******************************************************************************
 *
 * Example ACPICA initialization code with early ACPI table access. This shows
 * an initialization that requires early access to ACPI tables (before
 * kernel dynamic memory is available)
 *
 *****************************************************************************/

/*
 * The purpose of this static table array is to avoid the use of kernel
 * dynamic memory which may not be available during early ACPI table
 * access.
 */
#define ACPI_MAX_INIT_TABLES    16
static ACPI_TABLE_DESC      TableArray[ACPI_MAX_INIT_TABLES];


/*
 * This function would be called early in kernel initialization. After this
 * is called, all ACPI tables are available to the host.
 */
ACPI_STATUS
InitializeAcpiTables (
    void)
{
    ACPI_STATUS             Status;


    /* Initialize the ACPICA Table Manager and get all ACPI tables */

    Status = AcpiInitializeTables (TableArray, ACPI_MAX_INIT_TABLES, TRUE);
    return (Status);
}


/*
 * This function would be called after the kernel is initialized and
 * dynamic/virtual memory is available. It completes the initialization of
 * the ACPICA subsystem.
 */
ACPI_STATUS
InitializeAcpi (
    void)
{
    ACPI_STATUS             Status;


    /* Initialize the ACPICA subsystem */

    Status = AcpiInitializeSubsystem ();
    if (ACPI_FAILURE (Status))
    {
        return (Status);
    }

    /* Copy the root table list to dynamic memory */

    Status = AcpiReallocateRootTable ();
    if (ACPI_FAILURE (Status))
    {
        return (Status);
    }

    /* Create the ACPI namespace from ACPI tables */

    Status = AcpiLoadTables ();
    if (ACPI_FAILURE (Status))
    {
        return (Status);
    }

    /* Install local handlers */

    Status = InstallHandlers ();
    if (ACPI_FAILURE (Status))
    {
        ACPI_EXCEPTION ((AE_INFO, Status, "While installing handlers"));
        return (Status);
    }

    /* Initialize the ACPI hardware */

    Status = AcpiEnableSubsystem (ACPI_FULL_INITIALIZATION);
    if (ACPI_FAILURE (Status))
    {
        return (Status);
    }

    /* Complete the ACPI namespace object initialization */

    Status = AcpiInitializeObjects (ACPI_FULL_INITIALIZATION);
    if (ACPI_FAILURE (Status))
    {
        return (Status);
    }

    return (AE_OK);
}


/******************************************************************************
 *
 * Example ACPICA handler and handler installation
 *
 *****************************************************************************/

static void
NotifyHandler (
    ACPI_HANDLE                 Device,
    UINT32                      Value,
    void                        *Context)
{

    ACPI_INFO ((AE_INFO, "Received a notify 0x%X", Value));
}


static ACPI_STATUS
RegionInit (
    ACPI_HANDLE                 RegionHandle,
    UINT32                      Function,
    void                        *HandlerContext,
    void                        **RegionContext)
{

    if (Function == ACPI_REGION_DEACTIVATE)
    {
        *RegionContext = NULL;
    }
    else
    {
        *RegionContext = RegionHandle;
    }

    return (AE_OK);
}


static ACPI_STATUS
RegionHandler (
    UINT32                      Function,
    ACPI_PHYSICAL_ADDRESS       Address,
    UINT32                      BitWidth,
    UINT64                      *Value,
    void                        *HandlerContext,
    void                        *RegionContext)
{

    ACPI_INFO ((AE_INFO, "Received a region access"));

    return (AE_OK);
}


static ACPI_STATUS
InstallHandlers (void)
{
    ACPI_STATUS             Status;


    /* Install global notify handler */

    Status = AcpiInstallNotifyHandler (ACPI_ROOT_OBJECT, ACPI_SYSTEM_NOTIFY,
                                        NotifyHandler, NULL);
    if (ACPI_FAILURE (Status))
    {
        ACPI_EXCEPTION ((AE_INFO, Status, "While installing Notify handler"));
        return (Status);
    }

    Status = AcpiInstallAddressSpaceHandler (ACPI_ROOT_OBJECT, ACPI_ADR_SPACE_SYSTEM_MEMORY,
        RegionHandler, RegionInit, NULL);
    if (ACPI_FAILURE (Status))
    {
        ACPI_EXCEPTION ((AE_INFO, Status, "While installing an OpRegion handler"));
        return (Status);
    }

    return (AE_OK);
}


/******************************************************************************
 *
 * Examples of control method execution.
 *
 * _OSI is a predefined method that is implemented internally within ACPICA.
 *
 * Shows the following elements:
 *
 * 1) How to setup a control method argument and argument list
 * 2) How to setup the return value object
 * 3) How to invoke AcpiEvaluateObject
 * 4) How to check the returned ACPI_STATUS
 * 5) How to analyze the return value
 *
 *****************************************************************************/

static void
ExecuteOSI (void)
{
    ACPI_STATUS             Status;
    ACPI_OBJECT_LIST        ArgList;
    ACPI_OBJECT             Arg[1];
    ACPI_BUFFER             ReturnValue;
    ACPI_OBJECT             *Object;


    ACPI_INFO ((AE_INFO, "Executing _OSI reserved method"));

    /* Setup input argument */

    ArgList.Count = 1;
    ArgList.Pointer = Arg;

    Arg[0].Type = ACPI_TYPE_STRING;
    Arg[0].String.Pointer = "Windows 2001";
    Arg[0].String.Length = strlen (Arg[0].String.Pointer);

    /* Ask ACPICA to allocate space for the return object */

    ReturnValue.Length = ACPI_ALLOCATE_BUFFER;

    Status = AcpiEvaluateObject (NULL, "\\_OSI", &ArgList, &ReturnValue);
    if (ACPI_FAILURE (Status))
    {
        ACPI_EXCEPTION ((AE_INFO, Status, "While executing _OSI"));
        return;
    }

    /* Ensure that the return object is large enough */

    if (ReturnValue.Length < sizeof (ACPI_OBJECT))
    {
        AcpiOsPrintf ("Return value from _OSI method too small, %.8X\n",
            ReturnValue.Length);
        goto ErrorExit;
    }

    /* Expect an integer return value from execution of _OSI */

    Object = ReturnValue.Pointer;
    if (Object->Type != ACPI_TYPE_INTEGER)
    {
        AcpiOsPrintf ("Invalid return type from _OSI, %.2X\n", Object->Type);
    }

    ACPI_INFO ((AE_INFO, "_OSI returned 0x%8.8X", (UINT32) Object->Integer.Value));


ErrorExit:

    /* Free a buffer created via ACPI_ALLOCATE_BUFFER */

    AcpiOsFree (ReturnValue.Pointer);
}


/******************************************************************************
 *
 * Execute an actual control method in the DSDT (MAIN)
 *
 *****************************************************************************/

static void
ExecuteMAIN (void)
{
    ACPI_STATUS             Status;
    ACPI_OBJECT_LIST        ArgList;
    ACPI_OBJECT             Arg[1];
    ACPI_BUFFER             ReturnValue;
    ACPI_OBJECT             *Object;


    ACPI_INFO ((AE_INFO, "Executing MAIN method"));

    /* Setup input argument */

    ArgList.Count = 1;
    ArgList.Pointer = Arg;

    Arg[0].Type = ACPI_TYPE_STRING;
    Arg[0].String.Pointer = "Method [MAIN] is executing";
    Arg[0].String.Length = strlen (Arg[0].String.Pointer);

    /* Ask ACPICA to allocate space for the return object */

    ReturnValue.Length = ACPI_ALLOCATE_BUFFER;

    Status = AcpiEvaluateObject (NULL, "\\MAIN", &ArgList, &ReturnValue);
    if (ACPI_FAILURE (Status))
    {
        ACPI_EXCEPTION ((AE_INFO, Status, "While executing MAIN"));
        return;
    }

    if (ReturnValue.Pointer)
    {
        /* Obtain and validate the returned ACPI_OBJECT */

        Object = ReturnValue.Pointer;
        if (Object->Type == ACPI_TYPE_STRING)
        {
            AcpiOsPrintf ("Method [MAIN] returned: \"%s\"\n", Object->String.Pointer);
        }

        ACPI_FREE (ReturnValue.Pointer);
    }
}
