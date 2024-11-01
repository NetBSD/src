/*	$NetBSD: apei_cper.h,v 1.2.4.3 2024/11/01 14:45:36 martin Exp $	*/

/*-
 * Copyright (c) 2024 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * UEFI Common Platform Error Record
 *
 * https://uefi.org/specs/UEFI/2.10/Apx_N_Common_Platform_Error_Record.html
 */

#ifndef	_SYS_DEV_ACPI_APEI_CPER_H_
#define	_SYS_DEV_ACPI_APEI_CPER_H_

#include <sys/types.h>

#include <sys/cdefs.h>

/*
 * https://uefi.org/specs/UEFI/2.10/Apx_N_Common_Platform_Error_Record.html#record-header
 */
struct cper_header {
	char		SignatureStart[4];	/* `CPER' */
	uint16_t	Revision;
	uint32_t	SignatureEnd;		/* 0xffffffff */
	uint16_t	SectionCount;
	uint32_t	ErrorSeverity;
	uint32_t	ValidationBits;
	uint32_t	RecordLength;
	uint64_t	Timestamp;
	uint8_t		PlatformId[16];
	uint8_t		PartitionId[16];
	uint8_t		CreatorId[16];
	uint8_t		NotificationType[16];
	uint64_t	RecordId;
	uint32_t	Flags;
	uint64_t	PersistenceInfo;
	uint8_t		Reserved[12];
} __packed;
__CTASSERT(sizeof(struct cper_header) == 128);

enum {				/* struct cper_header::ErrorSeverity */
	CPER_ERROR_SEVERITY_RECOVERABLE		= 0,
	CPER_ERROR_SEVERITY_FATAL		= 1,
	CPER_ERROR_SEVERITY_CORRECTED		= 2,
	CPER_ERROR_SEVERITY_INFORMATIONAL	= 3,
};

enum {				/* struct cper_header::ValidationBits */
	CPER_VALID_PLATFORM_ID		= __BIT(0),
	CPER_VALID_TIMESTAMP		= __BIT(1),
	CPER_VALID_PARTITION_ID		= __BIT(2),
};

/*
 * https://uefi.org/specs/UEFI/2.10/Apx_N_Common_Platform_Error_Record.html#error-record-header-flags
 */
enum {				/* struct cper_header::Flags */
	CPER_HW_ERROR_FLAG_RECOVERED	= __BIT(0),
	CPER_HW_ERROR_FLAG_PREVERR	= __BIT(1),
	CPER_HW_ERROR_FLAG_SIMULATED	= __BIT(2),
};

/*
 * https://uefi.org/specs/UEFI/2.10/Apx_N_Common_Platform_Error_Record.html#section-descriptor-format
 */
enum {
	CPER_SECTION_FLAG_PRIMARY			= __BIT(0),
	CPER_SECTION_FLAG_CONTAINMENT_WARNING		= __BIT(1),
	CPER_SECTION_FLAG_RESET				= __BIT(2),
	CPER_SECTION_FLAG_ERROR_THRESHOLD_EXCEEDED	= __BIT(3),
	CPER_SECTION_FLAG_RESOURCE_NOT_ACCESSIBLE	= __BIT(4),
	CPER_SECTION_FLAG_LATENT_ERROR			= __BIT(5),
	CPER_SECTION_FLAG_PROPAGATED			= __BIT(6),
	CPER_SECTION_FLAG_OVERFLOW			= __BIT(7),
};

#define	CPER_SECTION_FLAGS_FMT	"\177\020"				      \
	"b\000"	"PRIMARY\0"						      \
	"b\001"	"CONTAINMENT_WARNING\0"					      \
	"b\002"	"RESET\0"						      \
	"b\003"	"ERROR_THRESHOLD_EXCEEDED\0"				      \
	"b\004"	"RESOURCE_NOT_ACCESSIBLE\0"				      \
	"b\005"	"LATENT_ERROR\0"					      \
	"b\006"	"PROPAGATED\0"						      \
	"b\007"	"OVERFLOW\0"						      \
	"\0"

/*
 * N.2.5. Memory Error Section
 *
 * https://uefi.org/specs/UEFI/2.10/Apx_N_Common_Platform_Error_Record.html#memory-error-section
 *
 * Type: {0xa5bc1114,0x6f64,0x4ede,{0xb8,0x63,0x3e,0x83,0xed,0x7c,0x83,0xb1}}
 */

struct cper_memory_error {
	uint64_t	ValidationBits;
	uint64_t	ErrorStatus;
	uint64_t	PhysicalAddress;
	uint64_t	PhysicalAddressMask;
	uint16_t	Node;
	uint16_t	Card;
	uint16_t	Module;
	uint16_t	Bank;
	uint16_t	Device;
	uint16_t	Row;
	uint16_t	Column;
	uint16_t	BitPosition;
	uint64_t	RequestorId;
	uint64_t	ResponderId;
	uint64_t	TargetId;
	uint8_t		MemoryErrorType;
} __packed;
__CTASSERT(sizeof(struct cper_memory_error) == 73);

struct cper_memory_error_ext {
	struct cper_memory_error	Base;
	uint8_t		Extended;
	uint16_t	RankNumber;
	uint16_t	CardHandle;
	uint16_t	ModuleHandle;
} __packed;
__CTASSERT(sizeof(struct cper_memory_error_ext) == 80);

enum {				/* struct cper_memory_error::ValidationBits */
	CPER_MEMORY_ERROR_VALID_ERROR_STATUS		= __BIT(0),
	CPER_MEMORY_ERROR_VALID_PHYSICAL_ADDRESS	= __BIT(1),
	CPER_MEMORY_ERROR_VALID_PHYSICAL_ADDRESS_MASK	= __BIT(2),
	CPER_MEMORY_ERROR_VALID_NODE			= __BIT(3),
	CPER_MEMORY_ERROR_VALID_CARD			= __BIT(4),
	CPER_MEMORY_ERROR_VALID_MODULE			= __BIT(5),
	CPER_MEMORY_ERROR_VALID_BANK			= __BIT(6),
	CPER_MEMORY_ERROR_VALID_DEVICE			= __BIT(7),
	CPER_MEMORY_ERROR_VALID_ROW			= __BIT(8),
	CPER_MEMORY_ERROR_VALID_COLUMN			= __BIT(9),
	CPER_MEMORY_ERROR_VALID_BIT_POSITION		= __BIT(10),
	CPER_MEMORY_ERROR_VALID_REQUESTOR_ID		= __BIT(11),
	CPER_MEMORY_ERROR_VALID_RESPONDER_ID		= __BIT(12),
	CPER_MEMORY_ERROR_VALID_TARGET_ID		= __BIT(13),
	CPER_MEMORY_ERROR_VALID_MEMORY_ERROR_TYPE	= __BIT(14),
	CPER_MEMORY_ERROR_VALID_RANK_NUMBER		= __BIT(15),
	CPER_MEMORY_ERROR_VALID_CARD_HANDLE		= __BIT(16),
	CPER_MEMORY_ERROR_VALID_MODULE_HANDLE		= __BIT(17),
	CPER_MEMORY_ERROR_VALID_EXTENDED_ROW		= __BIT(18),
	CPER_MEMORY_ERROR_VALID_BANK_GROUP		= __BIT(19),
	CPER_MEMORY_ERROR_VALID_BANK_ADDRESS		= __BIT(20),
	CPER_MEMORY_ERROR_VALID_CHIP_ID			= __BIT(21),
};

#define	CPER_MEMORY_ERROR_VALIDATION_BITS_FMT	"\177\020"		      \
	"b\000"	"ERROR_STATUS\0"					      \
	"b\001"	"PHYSICAL_ADDRESS\0"					      \
	"b\002"	"PHYSICAL_ADDRESS_MASK\0"				      \
	"b\003"	"NODE\0"						      \
	"b\004"	"CARD\0"						      \
	"b\005"	"MODULE\0"						      \
	"b\006"	"BANK\0"						      \
	"b\007"	"DEVICE\0"						      \
	"b\010"	"ROW\0"							      \
	"b\011"	"COLUMN\0"						      \
	"b\012"	"BIT_POSITION\0"					      \
	"b\013"	"REQUESTOR_ID\0"					      \
	"b\014"	"RESPONDER_ID\0"					      \
	"b\015"	"TARGET_ID\0"						      \
	"b\016"	"MEMORY_ERROR_TYPE\0"					      \
	"b\017"	"RANK_NUMBER\0"						      \
	"b\020"	"CARD_HANDLE\0"						      \
	"b\021"	"MODULE_HANDLE\0"					      \
	"b\022"	"EXTENDED_ROW\0"					      \
	"b\023"	"BANK_GROUP\0"						      \
	"b\024"	"BANK_ADDRESS\0"					      \
	"b\025"	"CHIP_ID\0"						      \
	"\0"

enum {				/* struct cper_memory_error::Bank */
	CPER_MEMORY_ERROR_BANK_ADDRESS	= __BITS(7,0),
	CPER_MEMORY_ERROR_BANK_GROUP	= __BITS(15,8),
};

#define	CPER_MEMORY_ERROR_TYPES(F)					      \
	F(CPER_MEMORY_ERROR_UNKNOWN, UNKNOWN, 0)			      \
	F(CPER_MEMORY_ERROR_NO_ERROR, NO_ERROR, 1)			      \
	F(CPER_MEMORY_ERROR_SINGLEBIT_ECC, SINGLEBIT_ECC, 2)		      \
	F(CPER_MEMORY_ERROR_MULTIBIT_ECC, MULTIBIT_ECC, 3)		      \
	F(CPER_MEMORY_ERROR_SINGLESYM_CHIPKILL_ECC, SINGLESYM_CHIPKILL_ECC, 4)\
	F(CPER_MEMORY_ERROR_MULTISYM_CHIPKILL_ECC, MULTISYM_CHIPKILL_ECC, 5)  \
	F(CPER_MEMORY_ERROR_MASTER_ABORT, MASTER_ABORT, 6)		      \
	F(CPER_MEMORY_ERROR_TARGET_ABORT, TARGET_ABORT, 7)		      \
	F(CPER_MEMORY_ERROR_PARITY_ERROR, PARITY_ERROR, 8)		      \
	F(CPER_MEMORY_ERROR_WATCHDOG_TIMEOUT, WATCHDOG_TIMEOUT, 9)	      \
	F(CPER_MEMORY_ERROR_INVALID_ADDRESS, INVALID_ADDRESS, 10)	      \
	F(CPER_MEMORY_ERROR_MIRROR_BROKEN, MIRROR_BROKEN, 11)		      \
	F(CPER_MEMORY_ERROR_MEMORY_SPARING, MEMORY_SPARING, 12)		      \
	F(CPER_MEMORY_ERROR_SCRUB_CORRECTED_ERROR, SCRUB_CORRECTED_ERROR, 13) \
	F(CPER_MEMORY_ERROR_SCRUB_UNCORRECTED_ERROR, SCRUB_UNCORRECTED_ERROR, \
	    14)								      \
	F(CPER_MEMORY_ERROR_PHYSMEM_MAPOUT_EVENT, PHYSMEM_MAPOUT_EVENT, 15)   \
	/* end of CPER_MEMORY_ERROR_TYPES */

enum cper_memory_error_type { /* struct cper_memory_error::MemoryErrorType */
#define	CPER_MEMORY_ERROR_TYPE_DEF(LN, SN, V)	LN = V,
	CPER_MEMORY_ERROR_TYPES(CPER_MEMORY_ERROR_TYPE_DEF)
#undef	CPER_MEMORY_ERROR_TYPE_DEF
};

enum {				/* struct cper_memory_error_ext::Extended */
	CPER_MEMORY_ERROR_EXTENDED_ROWBIT16		= __BIT(0),
	CPER_MEMORY_ERROR_EXTENDED_ROWBIT17		= __BIT(1),
	CPER_MEMORY_ERROR_EXTENDED_CHIPID		= __BITS(7,5),
};

/*
 * N.2.7. PCI Express Error Section
 *
 * https://uefi.org/specs/UEFI/2.10/Apx_N_Common_Platform_Error_Record.html#pci-express-error-section
 *
 * Type: {0xd995e954,0xbbc1,0x430f,{0xad,0x91,0xb4,0x4d,0xcb,0x3c,0x6f,0x35}}
 */

struct cper_pcie_error {
	uint64_t	ValidationBits;
	uint32_t	PortType;
	uint32_t	Version;
	uint32_t	CommandStatus;
	uint32_t	Reserved0;
	struct {
		uint8_t		VendorID[2];
		uint8_t		DeviceID[2]; /* product */
		uint8_t		ClassCode[3];
		uint8_t		Function;
		uint8_t		Device;
		uint8_t		Segment[2];
		uint8_t		Bus;
		uint8_t		SecondaryBus;
		uint8_t		Slot[2]; /* bits 0:2 resv, bits 3:15 slot */
		uint8_t		Reserved0;
	}		DeviceID;
	uint64_t	DeviceSerial;
	uint32_t	BridgeControlStatus;
	uint8_t		CapabilityStructure[60];
	uint8_t		AERInfo[96];
} __packed;
__CTASSERT(sizeof(struct cper_pcie_error) == 208);

enum {				/* struct cper_pcie_error::ValidationBits */
	CPER_PCIE_ERROR_VALID_PORT_TYPE			= __BIT(0),
	CPER_PCIE_ERROR_VALID_VERSION			= __BIT(1),
	CPER_PCIE_ERROR_VALID_COMMAND_STATUS		= __BIT(2),
	CPER_PCIE_ERROR_VALID_DEVICE_ID			= __BIT(3),
	CPER_PCIE_ERROR_VALID_DEVICE_SERIAL		= __BIT(4),
	CPER_PCIE_ERROR_VALID_BRIDGE_CONTROL_STATUS	= __BIT(5),
	CPER_PCIE_ERROR_VALID_CAPABILITY_STRUCTURE	= __BIT(6),
	CPER_PCIE_ERROR_VALID_AER_INFO			= __BIT(7),
};

#define	CPER_PCIE_ERROR_VALIDATION_BITS_FMT	"\177\020"		      \
	"b\000"	"PORT_TYPE\0"						      \
	"b\001"	"VERSION\0"						      \
	"b\002"	"COMMAND_STATUS\0"					      \
	"b\003"	"DEVICE_ID\0"						      \
	"b\004"	"DEVICE_SERIAL\0"					      \
	"b\005"	"BRIDGE_CONTROL_STATUS\0"				      \
	"b\006"	"CAPABILITY_STRUCTURE\0"				      \
	"b\007"	"AER_INFO\0"						      \
	"\0"

#define	CPER_PCIE_ERROR_PORT_TYPES(F)					      \
	F(CPER_PCIE_ERROR_PORT_TYPE_PCIE_ENDPOINT, PCIE_ENDPOINT, 0)	      \
	F(CPER_PCIE_ERROR_PORT_TYPE_LEGACY_PCI_ENDPOINT, LEGACY_PCI_ENDPOINT, \
	    1)								      \
	F(CPER_PCIE_ERROR_PORT_TYPE_ROOTPORT5_UPSTREAMSWITCH,		      \
	    ROOTPORT5_UPSTREAMSWITCH, 4)				      \
	F(CPER_PCIE_ERROR_PORT_TYPE_DOWNSTREAMSWITCH, DOWNSTREAMSWITCH, 6)    \
	F(CPER_PCIE_ERROR_PORT_TYPE_PCIE_PCI_BRIDGE, PCIE_PCI_BRIDGE, 7)      \
	F(CPER_PCIE_ERROR_PORT_TYPE_PCI_PCIE_BRIDGE, PCI_PCIE_BRIDGE, 8)      \
	F(CPER_PCIE_ERROR_PORT_TYPE_RCIEP_DEV, RCIEP_DEV, 9)		      \
		/* Root Complex Integrated Endpoint Device */		      \
	F(CPER_PCIE_ERROR_PORT_TYPE_RCEC, RCEC, 10)			      \
		/* Root Complex Event Collector */			      \
	/* end of CPER_PCIE_ERROR_PORT_TYPES */

enum cper_pcie_error_port_type { /* struct cper_pcie_error::PortType */
#define	CPER_PCIE_ERROR_PORT_TYPE_DEF(LN, SN, V)	LN = V,
	CPER_PCIE_ERROR_PORT_TYPES(CPER_PCIE_ERROR_PORT_TYPE_DEF)
#undef	CPER_PCIE_ERROR_PORT_TYPE_DEF
};

#endif	/* _SYS_DEV_ACPI_APEI_CPER_H_ */
