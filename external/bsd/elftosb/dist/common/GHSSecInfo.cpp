/*
 * File:    GHSSecInfo.cpp
 *
 * Copyright (c) Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
 */

#include "GHSSecInfo.h"
#include <stdexcept>
#include "Logging.h"
#include "EndianUtilities.h"

//! The name of the GHS-specific section info table ELF section.
const char * const kSecInfoSectionName = ".secinfo";

using namespace elftosb;

//! The ELF file passed into this constructor as the \a elf argument must remain
//! valid for the life of this object.
//!
//! \param elf The ELF file parser. An assertion is raised if this is NULL.
GHSSecInfo::GHSSecInfo(StELFFile * elf)
:	m_elf(elf), m_hasInfo(false), m_info(0), m_entryCount(0)
{
	assert(elf);
	
	// look up the section. if it's not there just leave m_info and m_entryCount to 0
	unsigned sectionIndex = m_elf->getIndexOfSectionWithName(kSecInfoSectionName);
	if (sectionIndex == SHN_UNDEF)
	{
		return;
	}
	
	// get the section data
	const Elf32_Shdr & secInfo = m_elf->getSectionAtIndex(sectionIndex);
	if (secInfo.sh_type != SHT_PROGBITS)
	{
		// .secinfo section isn't the right type, so something is wrong
		return;
	}

	m_hasInfo = true;
	m_info = (ghs_secinfo_t *)m_elf->getSectionDataAtIndex(sectionIndex);
	m_entryCount = secInfo.sh_size / sizeof(ghs_secinfo_t);
}

//! Looks up \a addr for \a length in the .secinfo array. Only if that address is in the
//! .secinfo array does this section need to be filled. If the section is found but the
//! length does not match the \a length argument, a message is logged at the
//! #Logger::WARNING level.
//!
//! If the .secinfo section is not present in the ELF file, this method always returns
//! true.
//!
//! \param addr The start address of the section to query.
//! \param length The length of the section. If a section with a start address matching
//!		\a addr is found, its length must match \a length to be considered.
//!
//! \retval true The section matching \a addr and \a length was found and should be filled.
//!		True is also returned when the ELF file does not have a .secinfo section.
//! \retval false The section was not found and should not be filled.
bool GHSSecInfo::isSectionFilled(uint32_t addr, uint32_t length)
{
	if (!m_hasInfo)
	{
		return true;
	}

	unsigned i;
	for (i = 0; i < m_entryCount; ++i)
	{
		// byte swap these values into host endianness
		uint32_t clearAddr = ENDIAN_LITTLE_TO_HOST_U32(m_info[i].m_clearAddr);
		uint32_t numBytesToClear = ENDIAN_LITTLE_TO_HOST_U32(m_info[i].m_numBytesToClear);
		
		// we only consider non-zero length clear regions
		if ((addr == clearAddr) && (numBytesToClear != 0))
		{
			// it is an error if the address matches but the length does not
			if (length != numBytesToClear)
			{
				Log::log(Logger::WARNING, "ELF Error: Size mismatch @ sect=%u, .secinfo=%u at addr 0x%08X\n", length, numBytesToClear, addr);
			}
			return true;
		}
	}

	return false;
}

//! Simply calls through to isSectionFilled(uint32_t, uint32_t) to determine
//! if \a section should be filled.
//!
//! If the .secinfo section is not present in the ELF file, this method always returns
//! true.
bool GHSSecInfo::isSectionFilled(const Elf32_Shdr & section)
{
	return isSectionFilled(section.sh_addr, section.sh_size);
}

