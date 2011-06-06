/*******************************************************************************
 *
 * Module Name: utresrc - Resource managment utilities
 *
 ******************************************************************************/

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


#define __UTRESRC_C__

#include "acpi.h"
#include "accommon.h"
#include "amlresrc.h"


#define _COMPONENT          ACPI_UTILITIES
        ACPI_MODULE_NAME    ("utresrc")


#if defined(ACPI_DISASSEMBLER) || defined (ACPI_DEBUGGER)

/*
 * Strings used to decode resource descriptors.
 * Used by both the disasssembler and the debugger resource dump routines
 */
const char                      *AcpiGbl_BmDecode[] =
{
    "NotBusMaster",
    "BusMaster"
};

const char                      *AcpiGbl_ConfigDecode[] =
{
    "0 - Good Configuration",
    "1 - Acceptable Configuration",
    "2 - Suboptimal Configuration",
    "3 - ***Invalid Configuration***",
};

const char                      *AcpiGbl_ConsumeDecode[] =
{
    "ResourceProducer",
    "ResourceConsumer"
};

const char                      *AcpiGbl_DecDecode[] =
{
    "PosDecode",
    "SubDecode"
};

const char                      *AcpiGbl_HeDecode[] =
{
    "Level",
    "Edge"
};

const char                      *AcpiGbl_IoDecode[] =
{
    "Decode10",
    "Decode16"
};

const char                      *AcpiGbl_LlDecode[] =
{
    "ActiveHigh",
    "ActiveLow"
};

const char                      *AcpiGbl_MaxDecode[] =
{
    "MaxNotFixed",
    "MaxFixed"
};

const char                      *AcpiGbl_MemDecode[] =
{
    "NonCacheable",
    "Cacheable",
    "WriteCombining",
    "Prefetchable"
};

const char                      *AcpiGbl_MinDecode[] =
{
    "MinNotFixed",
    "MinFixed"
};

const char                      *AcpiGbl_MtpDecode[] =
{
    "AddressRangeMemory",
    "AddressRangeReserved",
    "AddressRangeACPI",
    "AddressRangeNVS"
};

const char                      *AcpiGbl_RngDecode[] =
{
    "InvalidRanges",
    "NonISAOnlyRanges",
    "ISAOnlyRanges",
    "EntireRange"
};

const char                      *AcpiGbl_RwDecode[] =
{
    "ReadOnly",
    "ReadWrite"
};

const char                      *AcpiGbl_ShrDecode[] =
{
    "Exclusive",
    "Shared"
};

const char                      *AcpiGbl_SizDecode[] =
{
    "Transfer8",
    "Transfer8_16",
    "Transfer16",
    "InvalidSize"
};

const char                      *AcpiGbl_TrsDecode[] =
{
    "DenseTranslation",
    "SparseTranslation"
};

const char                      *AcpiGbl_TtpDecode[] =
{
    "TypeStatic",
    "TypeTranslation"
};

const char                      *AcpiGbl_TypDecode[] =
{
    "Compatibility",
    "TypeA",
    "TypeB",
    "TypeF"
};

#endif


/*
 * Base sizes of the raw AML resource descriptors, indexed by resource type.
 * Zero indicates a reserved (and therefore invalid) resource type.
 */
const UINT8                 AcpiGbl_ResourceAmlSizes[] =
{
    /* Small descriptors */

    0,
    0,
    0,
    0,
    ACPI_AML_SIZE_SMALL (AML_RESOURCE_IRQ),
    ACPI_AML_SIZE_SMALL (AML_RESOURCE_DMA),
    ACPI_AML_SIZE_SMALL (AML_RESOURCE_START_DEPENDENT),
    ACPI_AML_SIZE_SMALL (AML_RESOURCE_END_DEPENDENT),
    ACPI_AML_SIZE_SMALL (AML_RESOURCE_IO),
    ACPI_AML_SIZE_SMALL (AML_RESOURCE_FIXED_IO),
    0,
    0,
    0,
    0,
    ACPI_AML_SIZE_SMALL (AML_RESOURCE_VENDOR_SMALL),
    ACPI_AML_SIZE_SMALL (AML_RESOURCE_END_TAG),

    /* Large descriptors */

    0,
    ACPI_AML_SIZE_LARGE (AML_RESOURCE_MEMORY24),
    ACPI_AML_SIZE_LARGE (AML_RESOURCE_GENERIC_REGISTER),
    0,
    ACPI_AML_SIZE_LARGE (AML_RESOURCE_VENDOR_LARGE),
    ACPI_AML_SIZE_LARGE (AML_RESOURCE_MEMORY32),
    ACPI_AML_SIZE_LARGE (AML_RESOURCE_FIXED_MEMORY32),
    ACPI_AML_SIZE_LARGE (AML_RESOURCE_ADDRESS32),
    ACPI_AML_SIZE_LARGE (AML_RESOURCE_ADDRESS16),
    ACPI_AML_SIZE_LARGE (AML_RESOURCE_EXTENDED_IRQ),
    ACPI_AML_SIZE_LARGE (AML_RESOURCE_ADDRESS64),
    ACPI_AML_SIZE_LARGE (AML_RESOURCE_EXTENDED_ADDRESS64)
};


/*
 * Resource types, used to validate the resource length field.
 * The length of fixed-length types must match exactly, variable
 * lengths must meet the minimum required length, etc.
 * Zero indicates a reserved (and therefore invalid) resource type.
 */
static const UINT8          AcpiGbl_ResourceTypes[] =
{
    /* Small descriptors */

    0,
    0,
    0,
    0,
    ACPI_SMALL_VARIABLE_LENGTH,
    ACPI_FIXED_LENGTH,
    ACPI_SMALL_VARIABLE_LENGTH,
    ACPI_FIXED_LENGTH,
    ACPI_FIXED_LENGTH,
    ACPI_FIXED_LENGTH,
    0,
    0,
    0,
    0,
    ACPI_VARIABLE_LENGTH,
    ACPI_FIXED_LENGTH,

    /* Large descriptors */

    0,
    ACPI_FIXED_LENGTH,
    ACPI_FIXED_LENGTH,
    0,
    ACPI_VARIABLE_LENGTH,
    ACPI_FIXED_LENGTH,
    ACPI_FIXED_LENGTH,
    ACPI_VARIABLE_LENGTH,
    ACPI_VARIABLE_LENGTH,
    ACPI_VARIABLE_LENGTH,
    ACPI_VARIABLE_LENGTH,
    ACPI_FIXED_LENGTH
};


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtWalkAmlResources
 *
 * PARAMETERS:  Aml             - Pointer to the raw AML resource template
 *              AmlLength       - Length of the entire template
 *              UserFunction    - Called once for each descriptor found. If
 *                                NULL, a pointer to the EndTag is returned
 *              Context         - Passed to UserFunction
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Walk a raw AML resource list(buffer). User function called
 *              once for each resource found.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiUtWalkAmlResources (
    UINT8                   *Aml,
    ACPI_SIZE               AmlLength,
    ACPI_WALK_AML_CALLBACK  UserFunction,
    void                    *Context)
{
    ACPI_STATUS             Status;
    UINT8                   *EndAml;
    UINT8                   ResourceIndex;
    UINT32                  Length;
    UINT32                  Offset = 0;


    ACPI_FUNCTION_TRACE (UtWalkAmlResources);


    /* The absolute minimum resource template is one EndTag descriptor */

    if (AmlLength < sizeof (AML_RESOURCE_END_TAG))
    {
        return_ACPI_STATUS (AE_AML_NO_RESOURCE_END_TAG);
    }

    /* Point to the end of the resource template buffer */

    EndAml = Aml + AmlLength;

    /* Walk the byte list, abort on any invalid descriptor type or length */

    while (Aml < EndAml)
    {
        /* Validate the Resource Type and Resource Length */

        Status = AcpiUtValidateResource (Aml, &ResourceIndex);
        if (ACPI_FAILURE (Status))
        {
            return_ACPI_STATUS (Status);
        }

        /* Get the length of this descriptor */

        Length = AcpiUtGetDescriptorLength (Aml);

        /* Invoke the user function */

        if (UserFunction)
        {
            Status = UserFunction (Aml, Length, Offset, ResourceIndex, Context);
            if (ACPI_FAILURE (Status))
            {
                return (Status);
            }
        }

        /* An EndTag descriptor terminates this resource template */

        if (AcpiUtGetResourceType (Aml) == ACPI_RESOURCE_NAME_END_TAG)
        {
            /*
             * There must be at least one more byte in the buffer for
             * the 2nd byte of the EndTag
             */
            if ((Aml + 1) >= EndAml)
            {
                return_ACPI_STATUS (AE_AML_NO_RESOURCE_END_TAG);
            }

            /* Return the pointer to the EndTag if requested */

            if (!UserFunction)
            {
                *(void **) Context = Aml;
            }

            /* Normal exit */

            return_ACPI_STATUS (AE_OK);
        }

        Aml += Length;
        Offset += Length;
    }

    /* Did not find an EndTag descriptor */

    return (AE_AML_NO_RESOURCE_END_TAG);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtValidateResource
 *
 * PARAMETERS:  Aml             - Pointer to the raw AML resource descriptor
 *              ReturnIndex     - Where the resource index is returned. NULL
 *                                if the index is not required.
 *
 * RETURN:      Status, and optionally the Index into the global resource tables
 *
 * DESCRIPTION: Validate an AML resource descriptor by checking the Resource
 *              Type and Resource Length. Returns an index into the global
 *              resource information/dispatch tables for later use.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiUtValidateResource (
    void                    *Aml,
    UINT8                   *ReturnIndex)
{
    UINT8                   ResourceType;
    UINT8                   ResourceIndex;
    ACPI_RS_LENGTH          ResourceLength;
    ACPI_RS_LENGTH          MinimumResourceLength;


    ACPI_FUNCTION_ENTRY ();


    /*
     * 1) Validate the ResourceType field (Byte 0)
     */
    ResourceType = ACPI_GET8 (Aml);

    /*
     * Byte 0 contains the descriptor name (Resource Type)
     * Examine the large/small bit in the resource header
     */
    if (ResourceType & ACPI_RESOURCE_NAME_LARGE)
    {
        /* Verify the large resource type (name) against the max */

        if (ResourceType > ACPI_RESOURCE_NAME_LARGE_MAX)
        {
            return (AE_AML_INVALID_RESOURCE_TYPE);
        }

        /*
         * Large Resource Type -- bits 6:0 contain the name
         * Translate range 0x80-0x8B to index range 0x10-0x1B
         */
        ResourceIndex = (UINT8) (ResourceType - 0x70);
    }
    else
    {
        /*
         * Small Resource Type -- bits 6:3 contain the name
         * Shift range to index range 0x00-0x0F
         */
        ResourceIndex = (UINT8)
            ((ResourceType & ACPI_RESOURCE_NAME_SMALL_MASK) >> 3);
    }

    /* Check validity of the resource type, zero indicates name is invalid */

    if (!AcpiGbl_ResourceTypes[ResourceIndex])
    {
        return (AE_AML_INVALID_RESOURCE_TYPE);
    }


    /*
     * 2) Validate the ResourceLength field. This ensures that the length
     *    is at least reasonable, and guarantees that it is non-zero.
     */
    ResourceLength = AcpiUtGetResourceLength (Aml);
    MinimumResourceLength = AcpiGbl_ResourceAmlSizes[ResourceIndex];

    /* Validate based upon the type of resource - fixed length or variable */

    switch (AcpiGbl_ResourceTypes[ResourceIndex])
    {
    case ACPI_FIXED_LENGTH:

        /* Fixed length resource, length must match exactly */

        if (ResourceLength != MinimumResourceLength)
        {
            return (AE_AML_BAD_RESOURCE_LENGTH);
        }
        break;

    case ACPI_VARIABLE_LENGTH:

        /* Variable length resource, length must be at least the minimum */

        if (ResourceLength < MinimumResourceLength)
        {
            return (AE_AML_BAD_RESOURCE_LENGTH);
        }
        break;

    case ACPI_SMALL_VARIABLE_LENGTH:

        /* Small variable length resource, length can be (Min) or (Min-1) */

        if ((ResourceLength > MinimumResourceLength) ||
            (ResourceLength < (MinimumResourceLength - 1)))
        {
            return (AE_AML_BAD_RESOURCE_LENGTH);
        }
        break;

    default:

        /* Shouldn't happen (because of validation earlier), but be sure */

        return (AE_AML_INVALID_RESOURCE_TYPE);
    }

    /* Optionally return the resource table index */

    if (ReturnIndex)
    {
        *ReturnIndex = ResourceIndex;
    }

    return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtGetResourceType
 *
 * PARAMETERS:  Aml             - Pointer to the raw AML resource descriptor
 *
 * RETURN:      The Resource Type with no extraneous bits (except the
 *              Large/Small descriptor bit -- this is left alone)
 *
 * DESCRIPTION: Extract the Resource Type/Name from the first byte of
 *              a resource descriptor.
 *
 ******************************************************************************/

UINT8
AcpiUtGetResourceType (
    void                    *Aml)
{
    ACPI_FUNCTION_ENTRY ();


    /*
     * Byte 0 contains the descriptor name (Resource Type)
     * Examine the large/small bit in the resource header
     */
    if (ACPI_GET8 (Aml) & ACPI_RESOURCE_NAME_LARGE)
    {
        /* Large Resource Type -- bits 6:0 contain the name */

        return (ACPI_GET8 (Aml));
    }
    else
    {
        /* Small Resource Type -- bits 6:3 contain the name */

        return ((UINT8) (ACPI_GET8 (Aml) & ACPI_RESOURCE_NAME_SMALL_MASK));
    }
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtGetResourceLength
 *
 * PARAMETERS:  Aml             - Pointer to the raw AML resource descriptor
 *
 * RETURN:      Byte Length
 *
 * DESCRIPTION: Get the "Resource Length" of a raw AML descriptor. By
 *              definition, this does not include the size of the descriptor
 *              header or the length field itself.
 *
 ******************************************************************************/

UINT16
AcpiUtGetResourceLength (
    void                    *Aml)
{
    ACPI_RS_LENGTH          ResourceLength;


    ACPI_FUNCTION_ENTRY ();


    /*
     * Byte 0 contains the descriptor name (Resource Type)
     * Examine the large/small bit in the resource header
     */
    if (ACPI_GET8 (Aml) & ACPI_RESOURCE_NAME_LARGE)
    {
        /* Large Resource type -- bytes 1-2 contain the 16-bit length */

        ACPI_MOVE_16_TO_16 (&ResourceLength, ACPI_ADD_PTR (UINT8, Aml, 1));

    }
    else
    {
        /* Small Resource type -- bits 2:0 of byte 0 contain the length */

        ResourceLength = (UINT16) (ACPI_GET8 (Aml) &
                                    ACPI_RESOURCE_NAME_SMALL_LENGTH_MASK);
    }

    return (ResourceLength);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtGetResourceHeaderLength
 *
 * PARAMETERS:  Aml             - Pointer to the raw AML resource descriptor
 *
 * RETURN:      Length of the AML header (depends on large/small descriptor)
 *
 * DESCRIPTION: Get the length of the header for this resource.
 *
 ******************************************************************************/

UINT8
AcpiUtGetResourceHeaderLength (
    void                    *Aml)
{
    ACPI_FUNCTION_ENTRY ();


    /* Examine the large/small bit in the resource header */

    if (ACPI_GET8 (Aml) & ACPI_RESOURCE_NAME_LARGE)
    {
        return (sizeof (AML_RESOURCE_LARGE_HEADER));
    }
    else
    {
        return (sizeof (AML_RESOURCE_SMALL_HEADER));
    }
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtGetDescriptorLength
 *
 * PARAMETERS:  Aml             - Pointer to the raw AML resource descriptor
 *
 * RETURN:      Byte length
 *
 * DESCRIPTION: Get the total byte length of a raw AML descriptor, including the
 *              length of the descriptor header and the length field itself.
 *              Used to walk descriptor lists.
 *
 ******************************************************************************/

UINT32
AcpiUtGetDescriptorLength (
    void                    *Aml)
{
    ACPI_FUNCTION_ENTRY ();


    /*
     * Get the Resource Length (does not include header length) and add
     * the header length (depends on if this is a small or large resource)
     */
    return (AcpiUtGetResourceLength (Aml) +
            AcpiUtGetResourceHeaderLength (Aml));
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtGetResourceEndTag
 *
 * PARAMETERS:  ObjDesc         - The resource template buffer object
 *              EndTag          - Where the pointer to the EndTag is returned
 *
 * RETURN:      Status, pointer to the end tag
 *
 * DESCRIPTION: Find the EndTag resource descriptor in an AML resource template
 *              Note: allows a buffer length of zero.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiUtGetResourceEndTag (
    ACPI_OPERAND_OBJECT     *ObjDesc,
    UINT8                   **EndTag)
{
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (UtGetResourceEndTag);


    /* Allow a buffer length of zero */

    if (!ObjDesc->Buffer.Length)
    {
        *EndTag = ObjDesc->Buffer.Pointer;
        return_ACPI_STATUS (AE_OK);
    }

    /* Validate the template and get a pointer to the EndTag */

    Status = AcpiUtWalkAmlResources (ObjDesc->Buffer.Pointer,
                ObjDesc->Buffer.Length, NULL, EndTag);

    return_ACPI_STATUS (Status);
}


