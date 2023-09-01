/******************************************************************************
 *
 * Module Name: aeinstall - Installation of operation region handlers
 *
 *****************************************************************************/

/*
 * Copyright (C) 2000 - 2023, Intel Corp.
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

#include "aecommon.h"

#define _COMPONENT          ACPI_TOOLS
        ACPI_MODULE_NAME    ("aeinstall")


static ACPI_STATUS
AeRegionInit (
    ACPI_HANDLE             RegionHandle,
    UINT32                  Function,
    void                    *HandlerContext,
    void                    **RegionContext);

static ACPI_STATUS
AeInstallEcHandler (
    ACPI_HANDLE             ObjHandle,
    UINT32                  Level,
    void                    *Context,
    void                    **ReturnValue);

static ACPI_STATUS
AeInstallPciHandler (
    ACPI_HANDLE             ObjHandle,
    UINT32                  Level,
    void                    *Context,
    void                    **ReturnValue);

static ACPI_STATUS
AeInstallGedHandler (
    ACPI_HANDLE             ObjHandle,
    UINT32                  Level,
    void                    *Context,
    void                    **ReturnValue);


BOOLEAN                     AcpiGbl_DisplayRegionAccess = FALSE;
ACPI_CONNECTION_INFO        AeMyContext;


/*
 * We will override some of the default region handlers, especially
 * the SystemMemory handler, which must be implemented locally.
 * These handlers are installed "early" - before any _REG methods
 * are executed - since they are special in the sense that the ACPI spec
 * declares that they must "always be available". Cannot override the
 * DataTable region handler either -- needed for test execution.
 *
 * NOTE: The local region handler will simulate access to these address
 * spaces by creating a memory buffer behind each operation region.
 */
static ACPI_ADR_SPACE_TYPE  DefaultSpaceIdList[] =
{
    ACPI_ADR_SPACE_SYSTEM_MEMORY,
    ACPI_ADR_SPACE_SYSTEM_IO,
    ACPI_ADR_SPACE_PCI_CONFIG,
    ACPI_ADR_SPACE_EC
};

/*
 * We will install handlers for some of the various address space IDs.
 * Test one user-defined address space (used by aslts).
 */
#define ACPI_ADR_SPACE_USER_DEFINED1        0x80
#define ACPI_ADR_SPACE_USER_DEFINED2        0xE4

static ACPI_ADR_SPACE_TYPE  SpaceIdList[] =
{
    ACPI_ADR_SPACE_SMBUS,
    ACPI_ADR_SPACE_CMOS,
    ACPI_ADR_SPACE_PCI_BAR_TARGET,
    ACPI_ADR_SPACE_IPMI,
    ACPI_ADR_SPACE_GPIO,
    ACPI_ADR_SPACE_GSBUS,
    ACPI_ADR_SPACE_PLATFORM_COMM,
    ACPI_ADR_SPACE_PLATFORM_RT,
    ACPI_ADR_SPACE_FIXED_HARDWARE,
    ACPI_ADR_SPACE_USER_DEFINED1,
    ACPI_ADR_SPACE_USER_DEFINED2
};


/******************************************************************************
 *
 * FUNCTION:    AeRegionInit
 *
 * PARAMETERS:  Region init handler
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Opregion init function.
 *
 *****************************************************************************/

static ACPI_STATUS
AeRegionInit (
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


/******************************************************************************
 *
 * FUNCTION:    AeOverrideRegionHandlers
 *
 * PARAMETERS:  None
 *
 * RETURN:      None
 *
 * DESCRIPTION: Override the default region handlers for memory, i/o, and
 *              pci_config. Also install a handler for EC. This is part of
 *              the "install early handlers" functionality.
 *
 *****************************************************************************/

void
AeOverrideRegionHandlers (
    void)
{
    UINT32                  i;
    ACPI_STATUS             Status;

    /*
     * Install handlers that will override the default handlers for some of
     * the space IDs.
     */
    for (i = 0; i < ACPI_ARRAY_LENGTH (DefaultSpaceIdList); i++)
    {
        /* Install handler at the root object */

        Status = AcpiInstallAddressSpaceHandler (ACPI_ROOT_OBJECT,
            DefaultSpaceIdList[i], AeRegionHandler, AeRegionInit,
            &AeMyContext);

        if (ACPI_FAILURE (Status))
        {
            ACPI_EXCEPTION ((AE_INFO, Status,
                "Could not install an OpRegion handler for %s space(%u)",
                AcpiUtGetRegionName ((UINT8) DefaultSpaceIdList[i]),
                DefaultSpaceIdList[i]));
        }
    }
}


/******************************************************************************
 *
 * FUNCTION:    AeInstallRegionHandlers
 *
 * PARAMETERS:  None
 *
 * RETURN:      None
 *
 * DESCRIPTION: Install handlers for the address spaces other than
 *              SystemMemory, SystemIO, and PCI_CONFIG.
 *
 *****************************************************************************/

void
AeInstallRegionHandlers (
    void)
{
    UINT32                  i;
    ACPI_STATUS             Status;


    /*
     * Install handlers for some of the "device driver" address spaces
     * such as SMBus, etc.
     */
    for (i = 0; i < ACPI_ARRAY_LENGTH (SpaceIdList); i++)
    {
        /* Install handler at the root object */

        Status = AcpiInstallAddressSpaceHandler (ACPI_ROOT_OBJECT,
            SpaceIdList[i], AeRegionHandler, AeRegionInit,
            &AeMyContext);

        if (ACPI_FAILURE (Status))
        {
            ACPI_EXCEPTION ((AE_INFO, Status,
                "Could not install an OpRegion handler for %s space(%u)",
                AcpiUtGetRegionName((UINT8) SpaceIdList[i]), SpaceIdList[i]));
            return;
        }
    }
}

/*******************************************************************************
 *
 * FUNCTION:    AeInstallGedHandler
 *
 * PARAMETERS:  ACPI_WALK_NAMESPACE callback
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Walk entire namespace, install a handler for every GED
 *              device found.
 *
 ******************************************************************************/
static ACPI_STATUS
AeInstallGedHandler (
    ACPI_HANDLE             ObjHandle,
    UINT32                  Level,
    void                    *Context,
    void                    **ReturnValue)
{

	ACPI_BUFFER             ReturnBuffer;
	ACPI_STATUS Status;
	ACPI_RESOURCE *ResourceList;
	ACPI_RESOURCE_EXTENDED_IRQ *extended_irq_rsc;
    ACPI_NAMESPACE_NODE     *Node;
	ACPI_NAMESPACE_NODE		*EvtMethodNode;

    ACPI_FUNCTION_ENTRY();

	/* Obtain the Namespace Node of this GED object handle. */
	Node = AcpiNsValidateHandle (ObjHandle);
	if (!Node)
	{
		return (AE_BAD_PARAMETER);
	}

	/*
	 * A GED device must have one _EVT method.
	 * Obtain the _EVT method and store it in the global
	 * GED register.
	 */
	Status = AcpiNsSearchOneScope (
		*ACPI_CAST_PTR (ACPI_NAME, METHOD_NAME__EVT),
		Node,
		ACPI_TYPE_METHOD,
		&EvtMethodNode
	);
	if (ACPI_FAILURE (Status))
	{
		AcpiOsPrintf ("Failed to obtain _EVT method for the GED device.\n");
		return Status;
	}

	ReturnBuffer.Pointer = NULL;
    ReturnBuffer.Length = ACPI_ALLOCATE_LOCAL_BUFFER;

	Status = AcpiGetCurrentResources (ObjHandle, &ReturnBuffer);
	if (ACPI_FAILURE (Status))
	{
		AcpiOsPrintf ("AcpiGetCurrentResources failed: %s\n",
			AcpiFormatException (Status));
		return Status;
	}

	/* Traverse the _CRS resource list */
	ResourceList = ACPI_CAST_PTR (ACPI_RESOURCE, ReturnBuffer.Pointer);
	while (ResourceList->Type != ACPI_RESOURCE_TYPE_END_TAG) {

		switch (ResourceList->Type) {
		 case ACPI_RESOURCE_TYPE_EXTENDED_IRQ: {

			/*
			 * Found an Interrupt resource. Link the interrupt resource
			 * and the _EVT method of this GED device in the GED event handler list.
			 */
			ACPI_GED_HANDLER_INFO *GedHandler =
			ACPI_ALLOCATE (sizeof (ACPI_SCI_HANDLER_INFO));
			if (!GedHandler)
			{
				return AE_NO_MEMORY;
			}

			GedHandler->Next = AcpiGbl_GedHandlerList;
			AcpiGbl_GedHandlerList = GedHandler;

			extended_irq_rsc = &ResourceList->Data.ExtendedIrq;

			GedHandler->IntId = extended_irq_rsc->Interrupts[0];
			GedHandler->EvtMethod = EvtMethodNode;

			AcpiOsPrintf ("Interrupt ID %d\n", extended_irq_rsc->Interrupts[0]);

			break;
		 }
		 default:

			AcpiOsPrintf ("Resource type %X\n", ResourceList->Type);
		}

		ResourceList = ACPI_NEXT_RESOURCE (ResourceList);
	}

	return AE_OK;
}

/*******************************************************************************
 *
 * FUNCTION:    AeInstallDeviceHandlers
 *
 * PARAMETERS:  None
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Install handlers for all EC, PCI and GED devices in the namespace
 *
 ******************************************************************************/

ACPI_STATUS
AeInstallDeviceHandlers (
    void)
{

    /* Find all Embedded Controller devices */

    AcpiGetDevices ("PNP0C09", AeInstallEcHandler, NULL, NULL);

    /* Install a PCI handler */

    AcpiGetDevices ("PNP0A08", AeInstallPciHandler, NULL, NULL);

    /* Install a GED handler */

    AcpiGetDevices ("ACPI0013", AeInstallGedHandler, NULL, NULL);

    return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AeInstallEcHandler
 *
 * PARAMETERS:  ACPI_WALK_NAMESPACE callback
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Walk entire namespace, install a handler for every EC
 *              device found.
 *
 ******************************************************************************/

static ACPI_STATUS
AeInstallEcHandler (
    ACPI_HANDLE             ObjHandle,
    UINT32                  Level,
    void                    *Context,
    void                    **ReturnValue)
{
    ACPI_STATUS             Status;


    /* Install the handler for this EC device */

    Status = AcpiInstallAddressSpaceHandler (ObjHandle,
        ACPI_ADR_SPACE_EC, AeRegionHandler, AeRegionInit, &AeMyContext);
    if (ACPI_FAILURE (Status))
    {
        ACPI_EXCEPTION ((AE_INFO, Status,
            "Could not install an OpRegion handler for EC device (%p)",
            ObjHandle));
    }

    return (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AeInstallPciHandler
 *
 * PARAMETERS:  ACPI_WALK_NAMESPACE callback
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Walk entire namespace, install a handler for every PCI
 *              device found.
 *
 ******************************************************************************/

static ACPI_STATUS
AeInstallPciHandler (
    ACPI_HANDLE             ObjHandle,
    UINT32                  Level,
    void                    *Context,
    void                    **ReturnValue)
{
    ACPI_STATUS             Status;


    /* Install memory and I/O handlers for the PCI device */

    Status = AcpiInstallAddressSpaceHandler (ObjHandle,
        ACPI_ADR_SPACE_SYSTEM_IO, AeRegionHandler, AeRegionInit,
        &AeMyContext);
    if (ACPI_FAILURE (Status))
    {
        ACPI_EXCEPTION ((AE_INFO, Status,
            "Could not install an OpRegion handler for PCI device (%p)",
            ObjHandle));
    }

    Status = AcpiInstallAddressSpaceHandler (ObjHandle,
        ACPI_ADR_SPACE_SYSTEM_MEMORY, AeRegionHandler, AeRegionInit,
        &AeMyContext);
    if (ACPI_FAILURE (Status))
    {
        ACPI_EXCEPTION ((AE_INFO, Status,
            "Could not install an OpRegion handler for PCI device (%p)",
            ObjHandle));
    }

    return (AE_CTRL_TERMINATE);
}
