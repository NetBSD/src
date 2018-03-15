/* $NetBSD: ixgbe_osdep.c,v 1.2.4.1 2018/03/15 09:12:06 pgoyette Exp $ */

/******************************************************************************

  Copyright (c) 2001-2017, Intel Corporation
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.

   3. Neither the name of the Intel Corporation nor the names of its
      contributors may be used to endorse or promote products derived from
      this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.

******************************************************************************/
/*$FreeBSD: head/sys/dev/ixgbe/ixgbe_osdep.c 320688 2017-07-05 17:27:03Z erj $*/

#include "ixgbe_osdep.h"
#include "ixgbe.h"

inline device_t
ixgbe_dev_from_hw(struct ixgbe_hw *hw)
{
	return ((struct adapter *)hw->back)->dev;
}

u16
ixgbe_read_pci_cfg(struct ixgbe_hw *hw, u32 reg)
{
	pci_chipset_tag_t  pc = hw->back->osdep.pc;
	pcitag_t           tag = hw->back->osdep.tag;

	switch (reg % 4) {
	case 0:
		return pci_conf_read(pc, tag, reg) & __BITS(15, 0);
	case 2:
		return __SHIFTOUT(pci_conf_read(pc, tag, reg - 2),
		    __BITS(31, 16));
	default:
		panic("%s: invalid register (%" PRIx32, __func__, reg); 
		break;
	}
}

void
ixgbe_write_pci_cfg(struct ixgbe_hw *hw, u32 reg, u16 value)
{
	pci_chipset_tag_t  pc = hw->back->osdep.pc;
	pcitag_t           tag = hw->back->osdep.tag;
	pcireg_t old;

	switch (reg % 4) {
	case 0:
		old = pci_conf_read(pc, tag, reg) & __BITS(31, 16);
		pci_conf_write(pc, tag, reg, value | old);
		break;
	case 2:
		old = pci_conf_read(pc, tag, reg - 2) & __BITS(15, 0);
		pci_conf_write(pc, tag, reg - 2,
		    __SHIFTIN(value, __BITS(31, 16)) | old);
		break;
	default:
		panic("%s: invalid register (%" PRIx32, __func__, reg); 
		break;
	}

	return;
}

inline u32
ixgbe_read_reg(struct ixgbe_hw *hw, u32 reg)
{
	return bus_space_read_4(((struct adapter *)hw->back)->osdep.mem_bus_space_tag,
	    ((struct adapter *)hw->back)->osdep.mem_bus_space_handle, reg);
}

inline void
ixgbe_write_reg(struct ixgbe_hw *hw, u32 reg, u32 val)
{
	bus_space_write_4(((struct adapter *)hw->back)->osdep.mem_bus_space_tag,
	    ((struct adapter *)hw->back)->osdep.mem_bus_space_handle,
	    reg, val);
}

inline u32
ixgbe_read_reg_array(struct ixgbe_hw *hw, u32 reg, u32 offset)
{
	return bus_space_read_4(((struct adapter *)hw->back)->osdep.mem_bus_space_tag,
	    ((struct adapter *)hw->back)->osdep.mem_bus_space_handle,
	    reg + (offset << 2));
}

inline void
ixgbe_write_reg_array(struct ixgbe_hw *hw, u32 reg, u32 offset, u32 val)
{
	bus_space_write_4(((struct adapter *)hw->back)->osdep.mem_bus_space_tag,
	    ((struct adapter *)hw->back)->osdep.mem_bus_space_handle,
	    reg + (offset << 2), val);
}
