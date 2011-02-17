/******************************************************************************
 *
 * Module Name: evregion - ACPI AddressSpace (OpRegion) handler dispatch
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


#define __EVREGION_C__

#include "acpi.h"
#include "accommon.h"
#include "acevents.h"
#include "acnamesp.h"
#include "acinterp.h"

#define _COMPONENT          ACPI_EVENTS
        ACPI_MODULE_NAME    ("evregion")


/* Local prototypes */

static BOOLEAN
AcpiEvHasDefaultHandler (
    ACPI_NAMESPACE_NODE     *Node,
    ACPI_ADR_SPACE_TYPE     SpaceId);

static ACPI_STATUS
AcpiEvRegRun (
    ACPI_HANDLE             ObjHandle,
    UINT32                  Level,
    void                    *Context,
    void                    **ReturnValue);

static ACPI_STATUS
AcpiEvInstallHandler (
    ACPI_HANDLE             ObjHandle,
    UINT32                  Level,
    void                    *Context,
    void                    **ReturnValue);

/* These are the address spaces that will get default handlers */

#define ACPI_NUM_DEFAULT_SPACES     4

static UINT8        AcpiGbl_DefaultAddressSpaces[ACPI_NUM_DEFAULT_SPACES] =
{
    ACPI_ADR_SPACE_SYSTEM_MEMORY,
    ACPI_ADR_SPACE_SYSTEM_IO,
    ACPI_ADR_SPACE_PCI_CONFIG,
    ACPI_ADR_SPACE_DATA_TABLE
};


/*******************************************************************************
 *
 * FUNCTION:    AcpiEvInstallRegionHandlers
 *
 * PARAMETERS:  None
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Installs the core subsystem default address space handlers.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiEvInstallRegionHandlers (
    void)
{
    ACPI_STATUS             Status;
    UINT32                  i;


    ACPI_FUNCTION_TRACE (EvInstallRegionHandlers);


    Status = AcpiUtAcquireMutex (ACPI_MTX_NAMESPACE);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    /*
     * All address spaces (PCI Config, EC, SMBus) are scope dependent and
     * registration must occur for a specific device.
     *
     * In the case of the system memory and IO address spaces there is
     * currently no device associated with the address space. For these we
     * use the root.
     *
     * We install the default PCI config space handler at the root so that
     * this space is immediately available even though the we have not
     * enumerated all the PCI Root Buses yet. This is to conform to the ACPI
     * specification which states that the PCI config space must be always
     * available -- even though we are nowhere near ready to find the PCI root
     * buses at this point.
     *
     * NOTE: We ignore AE_ALREADY_EXISTS because this means that a handler
     * has already been installed (via AcpiInstallAddressSpaceHandler).
     * Similar for AE_SAME_HANDLER.
     */
    for (i = 0; i < ACPI_NUM_DEFAULT_SPACES; i++)
    {
        Status = AcpiEvInstallSpaceHandler (AcpiGbl_RootNode,
                    AcpiGbl_DefaultAddressSpaces[i],
                    ACPI_DEFAULT_HANDLER, NULL, NULL);
        switch (Status)
        {
        case AE_OK:
        case AE_SAME_HANDLER:
        case AE_ALREADY_EXISTS:

            /* These exceptions are all OK */

            Status = AE_OK;
            break;

        default:

            goto UnlockAndExit;
        }
    }

UnlockAndExit:
    (void) AcpiUtReleaseMutex (ACPI_MTX_NAMESPACE);
    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiEvHasDefaultHandler
 *
 * PARAMETERS:  Node                - Namespace node for the device
 *              SpaceId             - The address space ID
 *
 * RETURN:      TRUE if default handler is installed, FALSE otherwise
 *
 * DESCRIPTION: Check if the default handler is installed for the requested
 *              space ID.
 *
 ******************************************************************************/

static BOOLEAN
AcpiEvHasDefaultHandler (
    ACPI_NAMESPACE_NODE     *Node,
    ACPI_ADR_SPACE_TYPE     SpaceId)
{
    ACPI_OPERAND_OBJECT     *ObjDesc;
    ACPI_OPERAND_OBJECT     *HandlerObj;


    /* Must have an existing internal object */

    ObjDesc = AcpiNsGetAttachedObject (Node);
    if (ObjDesc)
    {
        HandlerObj = ObjDesc->Device.Handler;

        /* Walk the linked list of handlers for this object */

        while (HandlerObj)
        {
            if (HandlerObj->AddressSpace.SpaceId == SpaceId)
            {
                if (HandlerObj->AddressSpace.HandlerFlags &
                        ACPI_ADDR_HANDLER_DEFAULT_INSTALLED)
                {
                    return (TRUE);
                }
            }

            HandlerObj = HandlerObj->AddressSpace.Next;
        }
    }

    return (FALSE);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiEvInitializeOpRegions
 *
 * PARAMETERS:  None
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Execute _REG methods for all Operation Regions that have
 *              an installed default region handler.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiEvInitializeOpRegions (
    void)
{
    ACPI_STATUS             Status;
    UINT32                  i;


    ACPI_FUNCTION_TRACE (EvInitializeOpRegions);


    Status = AcpiUtAcquireMutex (ACPI_MTX_NAMESPACE);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    /* Run the _REG methods for OpRegions in each default address space */

    for (i = 0; i < ACPI_NUM_DEFAULT_SPACES; i++)
    {
        /*
         * Make sure the installed handler is the DEFAULT handler. If not the
         * default, the _REG methods will have already been run (when the
         * handler was installed)
         */
        if (AcpiEvHasDefaultHandler (AcpiGbl_RootNode,
               AcpiGbl_DefaultAddressSpaces[i]))
        {
            Status = AcpiEvExecuteRegMethods (AcpiGbl_RootNode,
                        AcpiGbl_DefaultAddressSpaces[i]);
        }
    }

    AcpiGbl_RegMethodsExecuted = TRUE;

    (void) AcpiUtReleaseMutex (ACPI_MTX_NAMESPACE);
    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiEvExecuteRegMethod
 *
 * PARAMETERS:  RegionObj           - Region object
 *              Function            - Passed to _REG: On (1) or Off (0)
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Execute _REG method for a region
 *
 ******************************************************************************/

ACPI_STATUS
AcpiEvExecuteRegMethod (
    ACPI_OPERAND_OBJECT     *RegionObj,
    UINT32                  Function)
{
    ACPI_EVALUATE_INFO      *Info;
    ACPI_OPERAND_OBJECT     *Args[3];
    ACPI_OPERAND_OBJECT     *RegionObj2;
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (EvExecuteRegMethod);


    RegionObj2 = AcpiNsGetSecondaryObject (RegionObj);
    if (!RegionObj2)
    {
        return_ACPI_STATUS (AE_NOT_EXIST);
    }

    if (RegionObj2->Extra.Method_REG == NULL)
    {
        return_ACPI_STATUS (AE_OK);
    }

    /* Allocate and initialize the evaluation information block */

    Info = ACPI_ALLOCATE_ZEROED (sizeof (ACPI_EVALUATE_INFO));
    if (!Info)
    {
        return_ACPI_STATUS (AE_NO_MEMORY);
    }

    Info->PrefixNode = RegionObj2->Extra.Method_REG;
    Info->Pathname = NULL;
    Info->Parameters = Args;
    Info->Flags = ACPI_IGNORE_RETURN_VALUE;

    /*
     * The _REG method has two arguments:
     *
     * Arg0 - Integer:
     *  Operation region space ID Same value as RegionObj->Region.SpaceId
     *
     * Arg1 - Integer:
     *  connection status 1 for connecting the handler, 0 for disconnecting
     *  the handler (Passed as a parameter)
     */
    Args[0] = AcpiUtCreateIntegerObject ((UINT64) RegionObj->Region.SpaceId);
    if (!Args[0])
    {
        Status = AE_NO_MEMORY;
        goto Cleanup1;
    }

    Args[1] = AcpiUtCreateIntegerObject ((UINT64) Function);
    if (!Args[1])
    {
        Status = AE_NO_MEMORY;
        goto Cleanup2;
    }

    Args[2] = NULL; /* Terminate list */

    /* Execute the method, no return value */

    ACPI_DEBUG_EXEC (
        AcpiUtDisplayInitPathname (ACPI_TYPE_METHOD, Info->PrefixNode, NULL));

    Status = AcpiNsEvaluate (Info);
    AcpiUtRemoveReference (Args[1]);

Cleanup2:
    AcpiUtRemoveReference (Args[0]);

Cleanup1:
    ACPI_FREE (Info);
    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiEvAddressSpaceDispatch
 *
 * PARAMETERS:  RegionObj           - Internal region object
 *              Function            - Read or Write operation
 *              RegionOffset        - Where in the region to read or write
 *              BitWidth            - Field width in bits (8, 16, 32, or 64)
 *              Value               - Pointer to in or out value, must be
 *                                    a full 64-bit integer
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Dispatch an address space or operation region access to
 *              a previously installed handler.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiEvAddressSpaceDispatch (
    ACPI_OPERAND_OBJECT     *RegionObj,
    UINT32                  Function,
    UINT32                  RegionOffset,
    UINT32                  BitWidth,
    UINT64                  *Value)
{
    ACPI_STATUS             Status;
    ACPI_ADR_SPACE_HANDLER  Handler;
    ACPI_ADR_SPACE_SETUP    RegionSetup;
    ACPI_OPERAND_OBJECT     *HandlerDesc;
    ACPI_OPERAND_OBJECT     *RegionObj2;
    void                    *RegionContext = NULL;


    ACPI_FUNCTION_TRACE (EvAddressSpaceDispatch);


    RegionObj2 = AcpiNsGetSecondaryObject (RegionObj);
    if (!RegionObj2)
    {
        return_ACPI_STATUS (AE_NOT_EXIST);
    }

    /* Ensure that there is a handler associated with this region */

    HandlerDesc = RegionObj->Region.Handler;
    if (!HandlerDesc)
    {
        ACPI_ERROR ((AE_INFO,
            "No handler for Region [%4.4s] (%p) [%s]",
            AcpiUtGetNodeName (RegionObj->Region.Node),
            RegionObj, AcpiUtGetRegionName (RegionObj->Region.SpaceId)));

        return_ACPI_STATUS (AE_NOT_EXIST);
    }

    /*
     * It may be the case that the region has never been initialized.
     * Some types of regions require special init code
     */
    if (!(RegionObj->Region.Flags & AOPOBJ_SETUP_COMPLETE))
    {
        /* This region has not been initialized yet, do it */

        RegionSetup = HandlerDesc->AddressSpace.Setup;
        if (!RegionSetup)
        {
            /* No initialization routine, exit with error */

            ACPI_ERROR ((AE_INFO,
                "No init routine for region(%p) [%s]",
                RegionObj, AcpiUtGetRegionName (RegionObj->Region.SpaceId)));
            return_ACPI_STATUS (AE_NOT_EXIST);
        }

        /*
         * We must exit the interpreter because the region setup will
         * potentially execute control methods (for example, the _REG method
         * for this region)
         */
        AcpiExExitInterpreter ();

        Status = RegionSetup (RegionObj, ACPI_REGION_ACTIVATE,
                    HandlerDesc->AddressSpace.Context, &RegionContext);

        /* Re-enter the interpreter */

        AcpiExEnterInterpreter ();

        /* Check for failure of the Region Setup */

        if (ACPI_FAILURE (Status))
        {
            ACPI_EXCEPTION ((AE_INFO, Status,
                "During region initialization: [%s]",
                AcpiUtGetRegionName (RegionObj->Region.SpaceId)));
            return_ACPI_STATUS (Status);
        }

        /* Region initialization may have been completed by RegionSetup */

        if (!(RegionObj->Region.Flags & AOPOBJ_SETUP_COMPLETE))
        {
            RegionObj->Region.Flags |= AOPOBJ_SETUP_COMPLETE;

            if (RegionObj2->Extra.RegionContext)
            {
                /* The handler for this region was already installed */

                ACPI_FREE (RegionContext);
            }
            else
            {
                /*
                 * Save the returned context for use in all accesses to
                 * this particular region
                 */
                RegionObj2->Extra.RegionContext = RegionContext;
            }
        }
    }

    /* We have everything we need, we can invoke the address space handler */

    Handler = HandlerDesc->AddressSpace.Handler;

    ACPI_DEBUG_PRINT ((ACPI_DB_OPREGION,
        "Handler %p (@%p) Address %8.8X%8.8X [%s]\n",
        &RegionObj->Region.Handler->AddressSpace, Handler,
        ACPI_FORMAT_NATIVE_UINT (RegionObj->Region.Address + RegionOffset),
        AcpiUtGetRegionName (RegionObj->Region.SpaceId)));

    if (!(HandlerDesc->AddressSpace.HandlerFlags &
            ACPI_ADDR_HANDLER_DEFAULT_INSTALLED))
    {
        /*
         * For handlers other than the default (supplied) handlers, we must
         * exit the interpreter because the handler *might* block -- we don't
         * know what it will do, so we can't hold the lock on the intepreter.
         */
        AcpiExExitInterpreter();
    }

    /* Call the handler */

    Status = Handler (Function,
        (RegionObj->Region.Address + RegionOffset), BitWidth, Value,
        HandlerDesc->AddressSpace.Context, RegionObj2->Extra.RegionContext);

    if (ACPI_FAILURE (Status))
    {
        ACPI_EXCEPTION ((AE_INFO, Status, "Returned by Handler for [%s]",
            AcpiUtGetRegionName (RegionObj->Region.SpaceId)));
    }

    if (!(HandlerDesc->AddressSpace.HandlerFlags &
            ACPI_ADDR_HANDLER_DEFAULT_INSTALLED))
    {
        /*
         * We just returned from a non-default handler, we must re-enter the
         * interpreter
         */
       AcpiExEnterInterpreter ();
    }

    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiEvDetachRegion
 *
 * PARAMETERS:  RegionObj           - Region Object
 *              AcpiNsIsLocked      - Namespace Region Already Locked?
 *
 * RETURN:      None
 *
 * DESCRIPTION: Break the association between the handler and the region
 *              this is a two way association.
 *
 ******************************************************************************/

void
AcpiEvDetachRegion(
    ACPI_OPERAND_OBJECT     *RegionObj,
    BOOLEAN                 AcpiNsIsLocked)
{
    ACPI_OPERAND_OBJECT     *HandlerObj;
    ACPI_OPERAND_OBJECT     *ObjDesc;
    ACPI_OPERAND_OBJECT     **LastObjPtr;
    ACPI_ADR_SPACE_SETUP    RegionSetup;
    void                    **RegionContext;
    ACPI_OPERAND_OBJECT     *RegionObj2;
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (EvDetachRegion);


    RegionObj2 = AcpiNsGetSecondaryObject (RegionObj);
    if (!RegionObj2)
    {
        return_VOID;
    }
    RegionContext = &RegionObj2->Extra.RegionContext;

    /* Get the address handler from the region object */

    HandlerObj = RegionObj->Region.Handler;
    if (!HandlerObj)
    {
        /* This region has no handler, all done */

        return_VOID;
    }

    /* Find this region in the handler's list */

    ObjDesc = HandlerObj->AddressSpace.RegionList;
    LastObjPtr = &HandlerObj->AddressSpace.RegionList;

    while (ObjDesc)
    {
        /* Is this the correct Region? */

        if (ObjDesc == RegionObj)
        {
            ACPI_DEBUG_PRINT ((ACPI_DB_OPREGION,
                "Removing Region %p from address handler %p\n",
                RegionObj, HandlerObj));

            /* This is it, remove it from the handler's list */

            *LastObjPtr = ObjDesc->Region.Next;
            ObjDesc->Region.Next = NULL;        /* Must clear field */

            if (AcpiNsIsLocked)
            {
                Status = AcpiUtReleaseMutex (ACPI_MTX_NAMESPACE);
                if (ACPI_FAILURE (Status))
                {
                    return_VOID;
                }
            }

            /* Now stop region accesses by executing the _REG method */

            Status = AcpiEvExecuteRegMethod (RegionObj, 0);
            if (ACPI_FAILURE (Status))
            {
                ACPI_EXCEPTION ((AE_INFO, Status, "from region _REG, [%s]",
                    AcpiUtGetRegionName (RegionObj->Region.SpaceId)));
            }

            if (AcpiNsIsLocked)
            {
                Status = AcpiUtAcquireMutex (ACPI_MTX_NAMESPACE);
                if (ACPI_FAILURE (Status))
                {
                    return_VOID;
                }
            }

            /*
             * If the region has been activated, call the setup handler with
             * the deactivate notification
             */
            if (RegionObj->Region.Flags & AOPOBJ_SETUP_COMPLETE)
            {
                RegionSetup = HandlerObj->AddressSpace.Setup;
                Status = RegionSetup (RegionObj, ACPI_REGION_DEACTIVATE,
                    HandlerObj->AddressSpace.Context, RegionContext);

                /* Init routine may fail, Just ignore errors */

                if (ACPI_FAILURE (Status))
                {
                    ACPI_EXCEPTION ((AE_INFO, Status,
                        "from region handler - deactivate, [%s]",
                        AcpiUtGetRegionName (RegionObj->Region.SpaceId)));
                }

                RegionObj->Region.Flags &= ~(AOPOBJ_SETUP_COMPLETE);
            }

            /*
             * Remove handler reference in the region
             *
             * NOTE: this doesn't mean that the region goes away, the region
             * is just inaccessible as indicated to the _REG method
             *
             * If the region is on the handler's list, this must be the
             * region's handler
             */
            RegionObj->Region.Handler = NULL;
            AcpiUtRemoveReference (HandlerObj);

            return_VOID;
        }

        /* Walk the linked list of handlers */

        LastObjPtr = &ObjDesc->Region.Next;
        ObjDesc = ObjDesc->Region.Next;
    }

    /* If we get here, the region was not in the handler's region list */

    ACPI_DEBUG_PRINT ((ACPI_DB_OPREGION,
        "Cannot remove region %p from address handler %p\n",
        RegionObj, HandlerObj));

    return_VOID;
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiEvAttachRegion
 *
 * PARAMETERS:  HandlerObj          - Handler Object
 *              RegionObj           - Region Object
 *              AcpiNsIsLocked      - Namespace Region Already Locked?
 *
 * RETURN:      None
 *
 * DESCRIPTION: Create the association between the handler and the region
 *              this is a two way association.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiEvAttachRegion (
    ACPI_OPERAND_OBJECT     *HandlerObj,
    ACPI_OPERAND_OBJECT     *RegionObj,
    BOOLEAN                 AcpiNsIsLocked)
{

    ACPI_FUNCTION_TRACE (EvAttachRegion);


    ACPI_DEBUG_PRINT ((ACPI_DB_OPREGION,
        "Adding Region [%4.4s] %p to address handler %p [%s]\n",
        AcpiUtGetNodeName (RegionObj->Region.Node),
        RegionObj, HandlerObj,
        AcpiUtGetRegionName (RegionObj->Region.SpaceId)));

    /* Link this region to the front of the handler's list */

    RegionObj->Region.Next = HandlerObj->AddressSpace.RegionList;
    HandlerObj->AddressSpace.RegionList = RegionObj;

    /* Install the region's handler */

    if (RegionObj->Region.Handler)
    {
        return_ACPI_STATUS (AE_ALREADY_EXISTS);
    }

    RegionObj->Region.Handler = HandlerObj;
    AcpiUtAddReference (HandlerObj);

    return_ACPI_STATUS (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiEvInstallHandler
 *
 * PARAMETERS:  WalkNamespace callback
 *
 * DESCRIPTION: This routine installs an address handler into objects that are
 *              of type Region or Device.
 *
 *              If the Object is a Device, and the device has a handler of
 *              the same type then the search is terminated in that branch.
 *
 *              This is because the existing handler is closer in proximity
 *              to any more regions than the one we are trying to install.
 *
 ******************************************************************************/

static ACPI_STATUS
AcpiEvInstallHandler (
    ACPI_HANDLE             ObjHandle,
    UINT32                  Level,
    void                    *Context,
    void                    **ReturnValue)
{
    ACPI_OPERAND_OBJECT     *HandlerObj;
    ACPI_OPERAND_OBJECT     *NextHandlerObj;
    ACPI_OPERAND_OBJECT     *ObjDesc;
    ACPI_NAMESPACE_NODE     *Node;
    ACPI_STATUS             Status;


    ACPI_FUNCTION_NAME (EvInstallHandler);


    HandlerObj = (ACPI_OPERAND_OBJECT  *) Context;

    /* Parameter validation */

    if (!HandlerObj)
    {
        return (AE_OK);
    }

    /* Convert and validate the device handle */

    Node = AcpiNsValidateHandle (ObjHandle);
    if (!Node)
    {
        return (AE_BAD_PARAMETER);
    }

    /*
     * We only care about regions and objects that are allowed to have
     * address space handlers
     */
    if ((Node->Type != ACPI_TYPE_DEVICE) &&
        (Node->Type != ACPI_TYPE_REGION) &&
        (Node != AcpiGbl_RootNode))
    {
        return (AE_OK);
    }

    /* Check for an existing internal object */

    ObjDesc = AcpiNsGetAttachedObject (Node);
    if (!ObjDesc)
    {
        /* No object, just exit */

        return (AE_OK);
    }

    /* Devices are handled different than regions */

    if (ObjDesc->Common.Type == ACPI_TYPE_DEVICE)
    {
        /* Check if this Device already has a handler for this address space */

        NextHandlerObj = ObjDesc->Device.Handler;
        while (NextHandlerObj)
        {
            /* Found a handler, is it for the same address space? */

            if (NextHandlerObj->AddressSpace.SpaceId ==
                    HandlerObj->AddressSpace.SpaceId)
            {
                ACPI_DEBUG_PRINT ((ACPI_DB_OPREGION,
                    "Found handler for region [%s] in device %p(%p) "
                    "handler %p\n",
                    AcpiUtGetRegionName (HandlerObj->AddressSpace.SpaceId),
                    ObjDesc, NextHandlerObj, HandlerObj));

                /*
                 * Since the object we found it on was a device, then it
                 * means that someone has already installed a handler for
                 * the branch of the namespace from this device on. Just
                 * bail out telling the walk routine to not traverse this
                 * branch. This preserves the scoping rule for handlers.
                 */
                return (AE_CTRL_DEPTH);
            }

            /* Walk the linked list of handlers attached to this device */

            NextHandlerObj = NextHandlerObj->AddressSpace.Next;
        }

        /*
         * As long as the device didn't have a handler for this space we
         * don't care about it. We just ignore it and proceed.
         */
        return (AE_OK);
    }

    /* Object is a Region */

    if (ObjDesc->Region.SpaceId != HandlerObj->AddressSpace.SpaceId)
    {
        /* This region is for a different address space, just ignore it */

        return (AE_OK);
    }

    /*
     * Now we have a region and it is for the handler's address space type.
     *
     * First disconnect region for any previous handler (if any)
     */
    AcpiEvDetachRegion (ObjDesc, FALSE);

    /* Connect the region to the new handler */

    Status = AcpiEvAttachRegion (HandlerObj, ObjDesc, FALSE);
    return (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiEvInstallSpaceHandler
 *
 * PARAMETERS:  Node            - Namespace node for the device
 *              SpaceId         - The address space ID
 *              Handler         - Address of the handler
 *              Setup           - Address of the setup function
 *              Context         - Value passed to the handler on each access
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Install a handler for all OpRegions of a given SpaceId.
 *              Assumes namespace is locked
 *
 ******************************************************************************/

ACPI_STATUS
AcpiEvInstallSpaceHandler (
    ACPI_NAMESPACE_NODE     *Node,
    ACPI_ADR_SPACE_TYPE     SpaceId,
    ACPI_ADR_SPACE_HANDLER  Handler,
    ACPI_ADR_SPACE_SETUP    Setup,
    void                    *Context)
{
    ACPI_OPERAND_OBJECT     *ObjDesc;
    ACPI_OPERAND_OBJECT     *HandlerObj;
    ACPI_STATUS             Status;
    ACPI_OBJECT_TYPE        Type;
    UINT8                  Flags = 0;


    ACPI_FUNCTION_TRACE (EvInstallSpaceHandler);


    /*
     * This registration is valid for only the types below and the root. This
     * is where the default handlers get placed.
     */
    if ((Node->Type != ACPI_TYPE_DEVICE)     &&
        (Node->Type != ACPI_TYPE_PROCESSOR)  &&
        (Node->Type != ACPI_TYPE_THERMAL)    &&
        (Node != AcpiGbl_RootNode))
    {
        Status = AE_BAD_PARAMETER;
        goto UnlockAndExit;
    }

    if (Handler == ACPI_DEFAULT_HANDLER)
    {
        Flags = ACPI_ADDR_HANDLER_DEFAULT_INSTALLED;

        switch (SpaceId)
        {
        case ACPI_ADR_SPACE_SYSTEM_MEMORY:
            Handler = AcpiExSystemMemorySpaceHandler;
            Setup   = AcpiEvSystemMemoryRegionSetup;
            break;

        case ACPI_ADR_SPACE_SYSTEM_IO:
            Handler = AcpiExSystemIoSpaceHandler;
            Setup   = AcpiEvIoSpaceRegionSetup;
            break;

        case ACPI_ADR_SPACE_PCI_CONFIG:
            Handler = AcpiExPciConfigSpaceHandler;
            Setup   = AcpiEvPciConfigRegionSetup;
            break;

        case ACPI_ADR_SPACE_CMOS:
            Handler = AcpiExCmosSpaceHandler;
            Setup   = AcpiEvCmosRegionSetup;
            break;

        case ACPI_ADR_SPACE_PCI_BAR_TARGET:
            Handler = AcpiExPciBarSpaceHandler;
            Setup   = AcpiEvPciBarRegionSetup;
            break;

        case ACPI_ADR_SPACE_DATA_TABLE:
            Handler = AcpiExDataTableSpaceHandler;
            Setup   = NULL;
            break;

        default:
            Status = AE_BAD_PARAMETER;
            goto UnlockAndExit;
        }
    }

    /* If the caller hasn't specified a setup routine, use the default */

    if (!Setup)
    {
        Setup = AcpiEvDefaultRegionSetup;
    }

    /* Check for an existing internal object */

    ObjDesc = AcpiNsGetAttachedObject (Node);
    if (ObjDesc)
    {
        /*
         * The attached device object already exists. Make sure the handler
         * is not already installed.
         */
        HandlerObj = ObjDesc->Device.Handler;

        /* Walk the handler list for this device */

        while (HandlerObj)
        {
            /* Same SpaceId indicates a handler already installed */

            if (HandlerObj->AddressSpace.SpaceId == SpaceId)
            {
                if (HandlerObj->AddressSpace.Handler == Handler)
                {
                    /*
                     * It is (relatively) OK to attempt to install the SAME
                     * handler twice. This can easily happen with the
                     * PCI_Config space.
                     */
                    Status = AE_SAME_HANDLER;
                    goto UnlockAndExit;
                }
                else
                {
                    /* A handler is already installed */

                    Status = AE_ALREADY_EXISTS;
                }
                goto UnlockAndExit;
            }

            /* Walk the linked list of handlers */

            HandlerObj = HandlerObj->AddressSpace.Next;
        }
    }
    else
    {
        ACPI_DEBUG_PRINT ((ACPI_DB_OPREGION,
            "Creating object on Device %p while installing handler\n", Node));

        /* ObjDesc does not exist, create one */

        if (Node->Type == ACPI_TYPE_ANY)
        {
            Type = ACPI_TYPE_DEVICE;
        }
        else
        {
            Type = Node->Type;
        }

        ObjDesc = AcpiUtCreateInternalObject (Type);
        if (!ObjDesc)
        {
            Status = AE_NO_MEMORY;
            goto UnlockAndExit;
        }

        /* Init new descriptor */

        ObjDesc->Common.Type = (UINT8) Type;

        /* Attach the new object to the Node */

        Status = AcpiNsAttachObject (Node, ObjDesc, Type);

        /* Remove local reference to the object */

        AcpiUtRemoveReference (ObjDesc);

        if (ACPI_FAILURE (Status))
        {
            goto UnlockAndExit;
        }
    }

    ACPI_DEBUG_PRINT ((ACPI_DB_OPREGION,
        "Installing address handler for region %s(%X) on Device %4.4s %p(%p)\n",
        AcpiUtGetRegionName (SpaceId), SpaceId,
        AcpiUtGetNodeName (Node), Node, ObjDesc));

    /*
     * Install the handler
     *
     * At this point there is no existing handler. Just allocate the object
     * for the handler and link it into the list.
     */
    HandlerObj = AcpiUtCreateInternalObject (ACPI_TYPE_LOCAL_ADDRESS_HANDLER);
    if (!HandlerObj)
    {
        Status = AE_NO_MEMORY;
        goto UnlockAndExit;
    }

    /* Init handler obj */

    HandlerObj->AddressSpace.SpaceId = (UINT8) SpaceId;
    HandlerObj->AddressSpace.HandlerFlags = Flags;
    HandlerObj->AddressSpace.RegionList = NULL;
    HandlerObj->AddressSpace.Node = Node;
    HandlerObj->AddressSpace.Handler = Handler;
    HandlerObj->AddressSpace.Context = Context;
    HandlerObj->AddressSpace.Setup  = Setup;

    /* Install at head of Device.AddressSpace list */

    HandlerObj->AddressSpace.Next = ObjDesc->Device.Handler;

    /*
     * The Device object is the first reference on the HandlerObj.
     * Each region that uses the handler adds a reference.
     */
    ObjDesc->Device.Handler = HandlerObj;

    /*
     * Walk the namespace finding all of the regions this
     * handler will manage.
     *
     * Start at the device and search the branch toward
     * the leaf nodes until either the leaf is encountered or
     * a device is detected that has an address handler of the
     * same type.
     *
     * In either case, back up and search down the remainder
     * of the branch
     */
    Status = AcpiNsWalkNamespace (ACPI_TYPE_ANY, Node, ACPI_UINT32_MAX,
                ACPI_NS_WALK_UNLOCK, AcpiEvInstallHandler, NULL,
                HandlerObj, NULL);

UnlockAndExit:
    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiEvExecuteRegMethods
 *
 * PARAMETERS:  Node            - Namespace node for the device
 *              SpaceId         - The address space ID
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Run all _REG methods for the input Space ID;
 *              Note: assumes namespace is locked, or system init time.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiEvExecuteRegMethods (
    ACPI_NAMESPACE_NODE     *Node,
    ACPI_ADR_SPACE_TYPE     SpaceId)
{
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (EvExecuteRegMethods);


    /*
     * Run all _REG methods for all Operation Regions for this space ID. This
     * is a separate walk in order to handle any interdependencies between
     * regions and _REG methods. (i.e. handlers must be installed for all
     * regions of this Space ID before we can run any _REG methods)
     */
    Status = AcpiNsWalkNamespace (ACPI_TYPE_ANY, Node, ACPI_UINT32_MAX,
                ACPI_NS_WALK_UNLOCK, AcpiEvRegRun, NULL,
                &SpaceId, NULL);

    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiEvRegRun
 *
 * PARAMETERS:  WalkNamespace callback
 *
 * DESCRIPTION: Run _REG method for region objects of the requested spaceID
 *
 ******************************************************************************/

static ACPI_STATUS
AcpiEvRegRun (
    ACPI_HANDLE             ObjHandle,
    UINT32                  Level,
    void                    *Context,
    void                    **ReturnValue)
{
    ACPI_OPERAND_OBJECT     *ObjDesc;
    ACPI_NAMESPACE_NODE     *Node;
    ACPI_ADR_SPACE_TYPE     SpaceId;
    ACPI_STATUS             Status;


    SpaceId = *ACPI_CAST_PTR (ACPI_ADR_SPACE_TYPE, Context);

    /* Convert and validate the device handle */

    Node = AcpiNsValidateHandle (ObjHandle);
    if (!Node)
    {
        return (AE_BAD_PARAMETER);
    }

    /*
     * We only care about regions.and objects that are allowed to have address
     * space handlers
     */
    if ((Node->Type != ACPI_TYPE_REGION) &&
        (Node != AcpiGbl_RootNode))
    {
        return (AE_OK);
    }

    /* Check for an existing internal object */

    ObjDesc = AcpiNsGetAttachedObject (Node);
    if (!ObjDesc)
    {
        /* No object, just exit */

        return (AE_OK);
    }

    /* Object is a Region */

    if (ObjDesc->Region.SpaceId != SpaceId)
    {
        /* This region is for a different address space, just ignore it */

        return (AE_OK);
    }

    Status = AcpiEvExecuteRegMethod (ObjDesc, 1);
    return (Status);
}

