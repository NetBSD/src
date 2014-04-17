/******************************************************************************

  Copyright (c) 2001-2010, Intel Corporation 
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
/*$FreeBSD: src/sys/dev/ixgbe/ixgbe_common.c,v 1.12 2011/01/19 19:36:27 jfv Exp $*/
/*$NetBSD: ixgbe_common.c,v 1.2 2014/04/17 15:33:40 christos Exp $*/

#include "ixgbe_common.h"
#include "ixgbe_phy.h"
#include "ixgbe_api.h"

static s32 ixgbe_acquire_eeprom(struct ixgbe_hw *hw);
static s32 ixgbe_get_eeprom_semaphore(struct ixgbe_hw *hw);
static void ixgbe_release_eeprom_semaphore(struct ixgbe_hw *hw);
static s32 ixgbe_ready_eeprom(struct ixgbe_hw *hw);
static void ixgbe_standby_eeprom(struct ixgbe_hw *hw);
static void ixgbe_shift_out_eeprom_bits(struct ixgbe_hw *hw, u16 data,
                                        u16 count);
static u16 ixgbe_shift_in_eeprom_bits(struct ixgbe_hw *hw, u16 count);
static void ixgbe_raise_eeprom_clk(struct ixgbe_hw *hw, u32 *eec);
static void ixgbe_lower_eeprom_clk(struct ixgbe_hw *hw, u32 *eec);
static void ixgbe_release_eeprom(struct ixgbe_hw *hw);

static s32 ixgbe_mta_vector(struct ixgbe_hw *hw, u8 *mc_addr);
static s32 ixgbe_get_san_mac_addr_offset(struct ixgbe_hw *hw,
                                        u16 *san_mac_offset);
static s32 ixgbe_fc_autoneg_fiber(struct ixgbe_hw *hw);
static s32 ixgbe_fc_autoneg_backplane(struct ixgbe_hw *hw);
static s32 ixgbe_fc_autoneg_copper(struct ixgbe_hw *hw);
static s32 ixgbe_device_supports_autoneg_fc(struct ixgbe_hw *hw);
static s32 ixgbe_negotiate_fc(struct ixgbe_hw *hw, u32 adv_reg, u32 lp_reg,
			      u32 adv_sym, u32 adv_asm, u32 lp_sym, u32 lp_asm);

s32 ixgbe_find_vlvf_slot(struct ixgbe_hw *hw, u32 vlan);

/**
 *  ixgbe_init_ops_generic - Inits function ptrs
 *  @hw: pointer to the hardware structure
 *
 *  Initialize the function pointers.
 **/
s32 ixgbe_init_ops_generic(struct ixgbe_hw *hw)
{
	struct ixgbe_eeprom_info *eeprom = &hw->eeprom;
	struct ixgbe_mac_info *mac = &hw->mac;
	u32 eec = IXGBE_READ_REG(hw, IXGBE_EEC);

	DEBUGFUNC("ixgbe_init_ops_generic");

	/* EEPROM */
	eeprom->ops.init_params = &ixgbe_init_eeprom_params_generic;
	/* If EEPROM is valid (bit 8 = 1), use EERD otherwise use bit bang */
	if (eec & (1 << 8))
		eeprom->ops.read = &ixgbe_read_eerd_generic;
	else
		eeprom->ops.read = &ixgbe_read_eeprom_bit_bang_generic;
	eeprom->ops.write = &ixgbe_write_eeprom_generic;
	eeprom->ops.validate_checksum =
	                              &ixgbe_validate_eeprom_checksum_generic;
	eeprom->ops.update_checksum = &ixgbe_update_eeprom_checksum_generic;
	eeprom->ops.calc_checksum = &ixgbe_calc_eeprom_checksum_generic;

	/* MAC */
	mac->ops.init_hw = &ixgbe_init_hw_generic;
	mac->ops.reset_hw = NULL;
	mac->ops.start_hw = &ixgbe_start_hw_generic;
	mac->ops.clear_hw_cntrs = &ixgbe_clear_hw_cntrs_generic;
	mac->ops.get_media_type = NULL;
	mac->ops.get_supported_physical_layer = NULL;
	mac->ops.enable_rx_dma = &ixgbe_enable_rx_dma_generic;
	mac->ops.get_mac_addr = &ixgbe_get_mac_addr_generic;
	mac->ops.stop_adapter = &ixgbe_stop_adapter_generic;
	mac->ops.get_bus_info = &ixgbe_get_bus_info_generic;
	mac->ops.set_lan_id = &ixgbe_set_lan_id_multi_port_pcie;
	mac->ops.acquire_swfw_sync = &ixgbe_acquire_swfw_sync;
	mac->ops.release_swfw_sync = &ixgbe_release_swfw_sync;

	/* LEDs */
	mac->ops.led_on = &ixgbe_led_on_generic;
	mac->ops.led_off = &ixgbe_led_off_generic;
	mac->ops.blink_led_start = &ixgbe_blink_led_start_generic;
	mac->ops.blink_led_stop = &ixgbe_blink_led_stop_generic;

	/* RAR, Multicast, VLAN */
	mac->ops.set_rar = &ixgbe_set_rar_generic;
	mac->ops.clear_rar = &ixgbe_clear_rar_generic;
	mac->ops.insert_mac_addr = NULL;
	mac->ops.set_vmdq = NULL;
	mac->ops.clear_vmdq = NULL;
	mac->ops.init_rx_addrs = &ixgbe_init_rx_addrs_generic;
	mac->ops.update_uc_addr_list = &ixgbe_update_uc_addr_list_generic;
	mac->ops.update_mc_addr_list = &ixgbe_update_mc_addr_list_generic;
	mac->ops.enable_mc = &ixgbe_enable_mc_generic;
	mac->ops.disable_mc = &ixgbe_disable_mc_generic;
	mac->ops.clear_vfta = NULL;
	mac->ops.set_vfta = NULL;
	mac->ops.init_uta_tables = NULL;

	/* Flow Control */
	mac->ops.fc_enable = &ixgbe_fc_enable_generic;

	/* Link */
	mac->ops.get_link_capabilities = NULL;
	mac->ops.setup_link = NULL;
	mac->ops.check_link = NULL;

	return IXGBE_SUCCESS;
}

/**
 *  ixgbe_start_hw_generic - Prepare hardware for Tx/Rx
 *  @hw: pointer to hardware structure
 *
 *  Starts the hardware by filling the bus info structure and media type, clears
 *  all on chip counters, initializes receive address registers, multicast
 *  table, VLAN filter table, calls routine to set up link and flow control
 *  settings, and leaves transmit and receive units disabled and uninitialized
 **/
s32 ixgbe_start_hw_generic(struct ixgbe_hw *hw)
{
	u32 ctrl_ext;

	DEBUGFUNC("ixgbe_start_hw_generic");

	/* Set the media type */
	hw->phy.media_type = hw->mac.ops.get_media_type(hw);

	/* PHY ops initialization must be done in reset_hw() */

	/* Clear the VLAN filter table */
	hw->mac.ops.clear_vfta(hw);

	/* Clear statistics registers */
	hw->mac.ops.clear_hw_cntrs(hw);

	/* Set No Snoop Disable */
	ctrl_ext = IXGBE_READ_REG(hw, IXGBE_CTRL_EXT);
	ctrl_ext |= IXGBE_CTRL_EXT_NS_DIS;
	IXGBE_WRITE_REG(hw, IXGBE_CTRL_EXT, ctrl_ext);
	IXGBE_WRITE_FLUSH(hw);

	/* Setup flow control */
	ixgbe_setup_fc(hw, 0);

	/* Clear adapter stopped flag */
	hw->adapter_stopped = FALSE;

	return IXGBE_SUCCESS;
}

/**
 *  ixgbe_start_hw_gen2 - Init sequence for common device family
 *  @hw: pointer to hw structure
 *
 * Performs the init sequence common to the second generation
 * of 10 GbE devices.
 * Devices in the second generation:
 *     82599
 *     X540
 **/
s32 ixgbe_start_hw_gen2(struct ixgbe_hw *hw)
{
	u32 i;
	u32 regval;

	/* Clear the rate limiters */
	for (i = 0; i < hw->mac.max_tx_queues; i++) {
		IXGBE_WRITE_REG(hw, IXGBE_RTTDQSEL, i);
		IXGBE_WRITE_REG(hw, IXGBE_RTTBCNRC, 0);
	}
	IXGBE_WRITE_FLUSH(hw);

	/* Disable relaxed ordering */
	for (i = 0; i < hw->mac.max_tx_queues; i++) {
		regval = IXGBE_READ_REG(hw, IXGBE_DCA_TXCTRL_82599(i));
		regval &= ~IXGBE_DCA_TXCTRL_TX_WB_RO_EN;
		IXGBE_WRITE_REG(hw, IXGBE_DCA_TXCTRL_82599(i), regval);
	}

	for (i = 0; i < hw->mac.max_rx_queues; i++) {
		regval = IXGBE_READ_REG(hw, IXGBE_DCA_RXCTRL(i));
		regval &= ~(IXGBE_DCA_RXCTRL_DESC_WRO_EN |
					IXGBE_DCA_RXCTRL_DESC_HSRO_EN);
		IXGBE_WRITE_REG(hw, IXGBE_DCA_RXCTRL(i), regval);
	}

	return IXGBE_SUCCESS;
}

/**
 *  ixgbe_init_hw_generic - Generic hardware initialization
 *  @hw: pointer to hardware structure
 *
 *  Initialize the hardware by resetting the hardware, filling the bus info
 *  structure and media type, clears all on chip counters, initializes receive
 *  address registers, multicast table, VLAN filter table, calls routine to set
 *  up link and flow control settings, and leaves transmit and receive units
 *  disabled and uninitialized
 **/
s32 ixgbe_init_hw_generic(struct ixgbe_hw *hw)
{
	s32 status;

	DEBUGFUNC("ixgbe_init_hw_generic");

	/* Reset the hardware */
	status = hw->mac.ops.reset_hw(hw);

	if (status == IXGBE_SUCCESS) {
		/* Start the HW */
		status = hw->mac.ops.start_hw(hw);
	}

	return status;
}

/**
 *  ixgbe_clear_hw_cntrs_generic - Generic clear hardware counters
 *  @hw: pointer to hardware structure
 *
 *  Clears all hardware statistics counters by reading them from the hardware
 *  Statistics counters are clear on read.
 **/
s32 ixgbe_clear_hw_cntrs_generic(struct ixgbe_hw *hw)
{
	u16 i = 0;

	DEBUGFUNC("ixgbe_clear_hw_cntrs_generic");

	IXGBE_READ_REG(hw, IXGBE_CRCERRS);
	IXGBE_READ_REG(hw, IXGBE_ILLERRC);
	IXGBE_READ_REG(hw, IXGBE_ERRBC);
	IXGBE_READ_REG(hw, IXGBE_MSPDC);
	for (i = 0; i < 8; i++)
		IXGBE_READ_REG(hw, IXGBE_MPC(i));

	IXGBE_READ_REG(hw, IXGBE_MLFC);
	IXGBE_READ_REG(hw, IXGBE_MRFC);
	IXGBE_READ_REG(hw, IXGBE_RLEC);
	IXGBE_READ_REG(hw, IXGBE_LXONTXC);
	IXGBE_READ_REG(hw, IXGBE_LXOFFTXC);
	if (hw->mac.type >= ixgbe_mac_82599EB) {
		IXGBE_READ_REG(hw, IXGBE_LXONRXCNT);
		IXGBE_READ_REG(hw, IXGBE_LXOFFRXCNT);
	} else {
		IXGBE_READ_REG(hw, IXGBE_LXONRXC);
		IXGBE_READ_REG(hw, IXGBE_LXOFFRXC);
	}

	for (i = 0; i < 8; i++) {
		IXGBE_READ_REG(hw, IXGBE_PXONTXC(i));
		IXGBE_READ_REG(hw, IXGBE_PXOFFTXC(i));
		if (hw->mac.type >= ixgbe_mac_82599EB) {
			IXGBE_READ_REG(hw, IXGBE_PXONRXCNT(i));
			IXGBE_READ_REG(hw, IXGBE_PXOFFRXCNT(i));
		} else {
			IXGBE_READ_REG(hw, IXGBE_PXONRXC(i));
			IXGBE_READ_REG(hw, IXGBE_PXOFFRXC(i));
		}
	}
	if (hw->mac.type >= ixgbe_mac_82599EB)
		for (i = 0; i < 8; i++)
			IXGBE_READ_REG(hw, IXGBE_PXON2OFFCNT(i));
	IXGBE_READ_REG(hw, IXGBE_PRC64);
	IXGBE_READ_REG(hw, IXGBE_PRC127);
	IXGBE_READ_REG(hw, IXGBE_PRC255);
	IXGBE_READ_REG(hw, IXGBE_PRC511);
	IXGBE_READ_REG(hw, IXGBE_PRC1023);
	IXGBE_READ_REG(hw, IXGBE_PRC1522);
	IXGBE_READ_REG(hw, IXGBE_GPRC);
	IXGBE_READ_REG(hw, IXGBE_BPRC);
	IXGBE_READ_REG(hw, IXGBE_MPRC);
	IXGBE_READ_REG(hw, IXGBE_GPTC);
	IXGBE_READ_REG(hw, IXGBE_GORCL);
	IXGBE_READ_REG(hw, IXGBE_GORCH);
	IXGBE_READ_REG(hw, IXGBE_GOTCL);
	IXGBE_READ_REG(hw, IXGBE_GOTCH);
	for (i = 0; i < 8; i++)
		IXGBE_READ_REG(hw, IXGBE_RNBC(i));
	IXGBE_READ_REG(hw, IXGBE_RUC);
	IXGBE_READ_REG(hw, IXGBE_RFC);
	IXGBE_READ_REG(hw, IXGBE_ROC);
	IXGBE_READ_REG(hw, IXGBE_RJC);
	IXGBE_READ_REG(hw, IXGBE_MNGPRC);
	IXGBE_READ_REG(hw, IXGBE_MNGPDC);
	IXGBE_READ_REG(hw, IXGBE_MNGPTC);
	IXGBE_READ_REG(hw, IXGBE_TORL);
	IXGBE_READ_REG(hw, IXGBE_TORH);
	IXGBE_READ_REG(hw, IXGBE_TPR);
	IXGBE_READ_REG(hw, IXGBE_TPT);
	IXGBE_READ_REG(hw, IXGBE_PTC64);
	IXGBE_READ_REG(hw, IXGBE_PTC127);
	IXGBE_READ_REG(hw, IXGBE_PTC255);
	IXGBE_READ_REG(hw, IXGBE_PTC511);
	IXGBE_READ_REG(hw, IXGBE_PTC1023);
	IXGBE_READ_REG(hw, IXGBE_PTC1522);
	IXGBE_READ_REG(hw, IXGBE_MPTC);
	IXGBE_READ_REG(hw, IXGBE_BPTC);
	for (i = 0; i < 16; i++) {
		IXGBE_READ_REG(hw, IXGBE_QPRC(i));
		IXGBE_READ_REG(hw, IXGBE_QPTC(i));
		if (hw->mac.type >= ixgbe_mac_82599EB) {
			IXGBE_READ_REG(hw, IXGBE_QBRC_L(i));
			IXGBE_READ_REG(hw, IXGBE_QBRC_H(i));
			IXGBE_READ_REG(hw, IXGBE_QBTC_L(i));
			IXGBE_READ_REG(hw, IXGBE_QBTC_H(i));
			IXGBE_READ_REG(hw, IXGBE_QPRDC(i));
		} else {
			IXGBE_READ_REG(hw, IXGBE_QBRC(i));
			IXGBE_READ_REG(hw, IXGBE_QBTC(i));
		}
	}

	return IXGBE_SUCCESS;
}

/**
 *  ixgbe_read_pba_string_generic - Reads part number string from EEPROM
 *  @hw: pointer to hardware structure
 *  @pba_num: stores the part number string from the EEPROM
 *  @pba_num_size: part number string buffer length
 *
 *  Reads the part number string from the EEPROM.
 **/
s32 ixgbe_read_pba_string_generic(struct ixgbe_hw *hw, u8 *pba_num,
                                  u32 pba_num_size)
{
	s32 ret_val;
	u16 data;
	u16 pba_ptr;
	u16 offset;
	u16 length;

	DEBUGFUNC("ixgbe_read_pba_string_generic");

	if (pba_num == NULL) {
		DEBUGOUT("PBA string buffer was null\n");
		return IXGBE_ERR_INVALID_ARGUMENT;
	}

	ret_val = hw->eeprom.ops.read(hw, IXGBE_PBANUM0_PTR, &data);
	if (ret_val) {
		DEBUGOUT("NVM Read Error\n");
		return ret_val;
	}

	ret_val = hw->eeprom.ops.read(hw, IXGBE_PBANUM1_PTR, &pba_ptr);
	if (ret_val) {
		DEBUGOUT("NVM Read Error\n");
		return ret_val;
	}

	/*
	 * if data is not ptr guard the PBA must be in legacy format which
	 * means pba_ptr is actually our second data word for the PBA number
	 * and we can decode it into an ascii string
	 */
	if (data != IXGBE_PBANUM_PTR_GUARD) {
		DEBUGOUT("NVM PBA number is not stored as string\n");

		/* we will need 11 characters to store the PBA */
		if (pba_num_size < 11) {
			DEBUGOUT("PBA string buffer too small\n");
			return IXGBE_ERR_NO_SPACE;
		}

		/* extract hex string from data and pba_ptr */
		pba_num[0] = (data >> 12) & 0xF;
		pba_num[1] = (data >> 8) & 0xF;
		pba_num[2] = (data >> 4) & 0xF;
		pba_num[3] = data & 0xF;
		pba_num[4] = (pba_ptr >> 12) & 0xF;
		pba_num[5] = (pba_ptr >> 8) & 0xF;
		pba_num[6] = '-';
		pba_num[7] = 0;
		pba_num[8] = (pba_ptr >> 4) & 0xF;
		pba_num[9] = pba_ptr & 0xF;

		/* put a null character on the end of our string */
		pba_num[10] = '\0';

		/* switch all the data but the '-' to hex char */
		for (offset = 0; offset < 10; offset++) {
			if (pba_num[offset] < 0xA)
				pba_num[offset] += '0';
			else if (pba_num[offset] < 0x10)
				pba_num[offset] += 'A' - 0xA;
		}

		return IXGBE_SUCCESS;
	}

	ret_val = hw->eeprom.ops.read(hw, pba_ptr, &length);
	if (ret_val) {
		DEBUGOUT("NVM Read Error\n");
		return ret_val;
	}

	if (length == 0xFFFF || length == 0) {
		DEBUGOUT("NVM PBA number section invalid length\n");
		return IXGBE_ERR_PBA_SECTION;
	}

	/* check if pba_num buffer is big enough */
	if (pba_num_size  < (((u32)length * 2) - 1)) {
		DEBUGOUT("PBA string buffer too small\n");
		return IXGBE_ERR_NO_SPACE;
	}

	/* trim pba length from start of string */
	pba_ptr++;
	length--;

	for (offset = 0; offset < length; offset++) {
		ret_val = hw->eeprom.ops.read(hw, pba_ptr + offset, &data);
		if (ret_val) {
			DEBUGOUT("NVM Read Error\n");
			return ret_val;
		}
		pba_num[offset * 2] = (u8)(data >> 8);
		pba_num[(offset * 2) + 1] = (u8)(data & 0xFF);
	}
	pba_num[offset * 2] = '\0';

	return IXGBE_SUCCESS;
}

/**
 *  ixgbe_read_pba_length_generic - Reads part number length from EEPROM
 *  @hw: pointer to hardware structure
 *  @pba_num_size: part number string buffer length
 *
 *  Reads the part number length from the EEPROM.
 *  Returns expected buffer size in pba_num_size
 **/
s32 ixgbe_read_pba_length_generic(struct ixgbe_hw *hw, u32 *pba_num_size)
{
	s32 ret_val;
	u16 data;
	u16 pba_ptr;
	u16 length;

	DEBUGFUNC("ixgbe_read_pba_length_generic");

	if (pba_num_size == NULL) {
		DEBUGOUT("PBA buffer size was null\n");
		return IXGBE_ERR_INVALID_ARGUMENT;
	}

	ret_val = hw->eeprom.ops.read(hw, IXGBE_PBANUM0_PTR, &data);
	if (ret_val) {
		DEBUGOUT("NVM Read Error\n");
		return ret_val;
	}

	ret_val = hw->eeprom.ops.read(hw, IXGBE_PBANUM1_PTR, &pba_ptr);
	if (ret_val) {
		DEBUGOUT("NVM Read Error\n");
		return ret_val;
	}

	 /* if data is not ptr guard the PBA must be in legacy format */
	if (data != IXGBE_PBANUM_PTR_GUARD) {
		*pba_num_size = 11;
		return IXGBE_SUCCESS;
	}

	ret_val = hw->eeprom.ops.read(hw, pba_ptr, &length);
	if (ret_val) {
		DEBUGOUT("NVM Read Error\n");
		return ret_val;
	}

	if (length == 0xFFFF || length == 0) {
		DEBUGOUT("NVM PBA number section invalid length\n");
		return IXGBE_ERR_PBA_SECTION;
	}

	/*
	 * Convert from length in u16 values to u8 chars, add 1 for NULL,
	 * and subtract 2 because length field is included in length.
	 */
	*pba_num_size = ((u32)length * 2) - 1;

	return IXGBE_SUCCESS;
}

/**
 *  ixgbe_read_pba_num_generic - Reads part number from EEPROM
 *  @hw: pointer to hardware structure
 *  @pba_num: stores the part number from the EEPROM
 *
 *  Reads the part number from the EEPROM.
 **/
s32 ixgbe_read_pba_num_generic(struct ixgbe_hw *hw, u32 *pba_num)
{
	s32 ret_val;
	u16 data;

	DEBUGFUNC("ixgbe_read_pba_num_generic");

	ret_val = hw->eeprom.ops.read(hw, IXGBE_PBANUM0_PTR, &data);
	if (ret_val) {
		DEBUGOUT("NVM Read Error\n");
		return ret_val;
	} else if (data == IXGBE_PBANUM_PTR_GUARD) {
		DEBUGOUT("NVM Not supported\n");
		return IXGBE_NOT_IMPLEMENTED;
	}
	*pba_num = (u32)(data << 16);

	ret_val = hw->eeprom.ops.read(hw, IXGBE_PBANUM1_PTR, &data);
	if (ret_val) {
		DEBUGOUT("NVM Read Error\n");
		return ret_val;
	}
	*pba_num |= data;

	return IXGBE_SUCCESS;
}

/**
 *  ixgbe_get_mac_addr_generic - Generic get MAC address
 *  @hw: pointer to hardware structure
 *  @mac_addr: Adapter MAC address
 *
 *  Reads the adapter's MAC address from first Receive Address Register (RAR0)
 *  A reset of the adapter must be performed prior to calling this function
 *  in order for the MAC address to have been loaded from the EEPROM into RAR0
 **/
s32 ixgbe_get_mac_addr_generic(struct ixgbe_hw *hw, u8 *mac_addr)
{
	u32 rar_high;
	u32 rar_low;
	u16 i;

	DEBUGFUNC("ixgbe_get_mac_addr_generic");

	rar_high = IXGBE_READ_REG(hw, IXGBE_RAH(0));
	rar_low = IXGBE_READ_REG(hw, IXGBE_RAL(0));

	for (i = 0; i < 4; i++)
		mac_addr[i] = (u8)(rar_low >> (i*8));

	for (i = 0; i < 2; i++)
		mac_addr[i+4] = (u8)(rar_high >> (i*8));

	return IXGBE_SUCCESS;
}

/**
 *  ixgbe_get_bus_info_generic - Generic set PCI bus info
 *  @hw: pointer to hardware structure
 *
 *  Sets the PCI bus info (speed, width, type) within the ixgbe_hw structure
 **/
s32 ixgbe_get_bus_info_generic(struct ixgbe_hw *hw)
{
	struct ixgbe_mac_info *mac = &hw->mac;
	u16 link_status;

	DEBUGFUNC("ixgbe_get_bus_info_generic");

	hw->bus.type = ixgbe_bus_type_pci_express;

	/* Get the negotiated link width and speed from PCI config space */
	link_status = IXGBE_READ_PCIE_WORD(hw, IXGBE_PCI_LINK_STATUS);

	switch (link_status & IXGBE_PCI_LINK_WIDTH) {
	case IXGBE_PCI_LINK_WIDTH_1:
		hw->bus.width = ixgbe_bus_width_pcie_x1;
		break;
	case IXGBE_PCI_LINK_WIDTH_2:
		hw->bus.width = ixgbe_bus_width_pcie_x2;
		break;
	case IXGBE_PCI_LINK_WIDTH_4:
		hw->bus.width = ixgbe_bus_width_pcie_x4;
		break;
	case IXGBE_PCI_LINK_WIDTH_8:
		hw->bus.width = ixgbe_bus_width_pcie_x8;
		break;
	default:
		hw->bus.width = ixgbe_bus_width_unknown;
		break;
	}

	switch (link_status & IXGBE_PCI_LINK_SPEED) {
	case IXGBE_PCI_LINK_SPEED_2500:
		hw->bus.speed = ixgbe_bus_speed_2500;
		break;
	case IXGBE_PCI_LINK_SPEED_5000:
		hw->bus.speed = ixgbe_bus_speed_5000;
		break;
	default:
		hw->bus.speed = ixgbe_bus_speed_unknown;
		break;
	}

	mac->ops.set_lan_id(hw);

	return IXGBE_SUCCESS;
}

/**
 *  ixgbe_set_lan_id_multi_port_pcie - Set LAN id for PCIe multiple port devices
 *  @hw: pointer to the HW structure
 *
 *  Determines the LAN function id by reading memory-mapped registers
 *  and swaps the port value if requested.
 **/
void ixgbe_set_lan_id_multi_port_pcie(struct ixgbe_hw *hw)
{
	struct ixgbe_bus_info *bus = &hw->bus;
	u32 reg;

	DEBUGFUNC("ixgbe_set_lan_id_multi_port_pcie");

	reg = IXGBE_READ_REG(hw, IXGBE_STATUS);
	bus->func = (reg & IXGBE_STATUS_LAN_ID) >> IXGBE_STATUS_LAN_ID_SHIFT;
	bus->lan_id = bus->func;

	/* check for a port swap */
	reg = IXGBE_READ_REG(hw, IXGBE_FACTPS);
	if (reg & IXGBE_FACTPS_LFS)
		bus->func ^= 0x1;
}

/**
 *  ixgbe_stop_adapter_generic - Generic stop Tx/Rx units
 *  @hw: pointer to hardware structure
 *
 *  Sets the adapter_stopped flag within ixgbe_hw struct. Clears interrupts,
 *  disables transmit and receive units. The adapter_stopped flag is used by
 *  the shared code and drivers to determine if the adapter is in a stopped
 *  state and should not touch the hardware.
 **/
s32 ixgbe_stop_adapter_generic(struct ixgbe_hw *hw)
{
	u32 number_of_queues;
	u32 reg_val;
	u16 i;

	DEBUGFUNC("ixgbe_stop_adapter_generic");

	/*
	 * Set the adapter_stopped flag so other driver functions stop touching
	 * the hardware
	 */
	hw->adapter_stopped = TRUE;

	/* Disable the receive unit */
	reg_val = IXGBE_READ_REG(hw, IXGBE_RXCTRL);
	reg_val &= ~(IXGBE_RXCTRL_RXEN);
	IXGBE_WRITE_REG(hw, IXGBE_RXCTRL, reg_val);
	IXGBE_WRITE_FLUSH(hw);
	msec_delay(2);

	/* Clear interrupt mask to stop from interrupts being generated */
	IXGBE_WRITE_REG(hw, IXGBE_EIMC, IXGBE_IRQ_CLEAR_MASK);

	/* Clear any pending interrupts */
	IXGBE_READ_REG(hw, IXGBE_EICR);

	/* Disable the transmit unit.  Each queue must be disabled. */
	number_of_queues = hw->mac.max_tx_queues;
	for (i = 0; i < number_of_queues; i++) {
		reg_val = IXGBE_READ_REG(hw, IXGBE_TXDCTL(i));
		if (reg_val & IXGBE_TXDCTL_ENABLE) {
			reg_val &= ~IXGBE_TXDCTL_ENABLE;
			IXGBE_WRITE_REG(hw, IXGBE_TXDCTL(i), reg_val);
		}
	}

	/*
	 * Prevent the PCI-E bus from from hanging by disabling PCI-E master
	 * access and verify no pending requests
	 */
	ixgbe_disable_pcie_master(hw);

	return IXGBE_SUCCESS;
}

/**
 *  ixgbe_led_on_generic - Turns on the software controllable LEDs.
 *  @hw: pointer to hardware structure
 *  @index: led number to turn on
 **/
s32 ixgbe_led_on_generic(struct ixgbe_hw *hw, u32 index)
{
	u32 led_reg = IXGBE_READ_REG(hw, IXGBE_LEDCTL);

	DEBUGFUNC("ixgbe_led_on_generic");

	/* To turn on the LED, set mode to ON. */
	led_reg &= ~IXGBE_LED_MODE_MASK(index);
	led_reg |= IXGBE_LED_ON << IXGBE_LED_MODE_SHIFT(index);
	IXGBE_WRITE_REG(hw, IXGBE_LEDCTL, led_reg);
	IXGBE_WRITE_FLUSH(hw);

	return IXGBE_SUCCESS;
}

/**
 *  ixgbe_led_off_generic - Turns off the software controllable LEDs.
 *  @hw: pointer to hardware structure
 *  @index: led number to turn off
 **/
s32 ixgbe_led_off_generic(struct ixgbe_hw *hw, u32 index)
{
	u32 led_reg = IXGBE_READ_REG(hw, IXGBE_LEDCTL);

	DEBUGFUNC("ixgbe_led_off_generic");

	/* To turn off the LED, set mode to OFF. */
	led_reg &= ~IXGBE_LED_MODE_MASK(index);
	led_reg |= IXGBE_LED_OFF << IXGBE_LED_MODE_SHIFT(index);
	IXGBE_WRITE_REG(hw, IXGBE_LEDCTL, led_reg);
	IXGBE_WRITE_FLUSH(hw);

	return IXGBE_SUCCESS;
}

/**
 *  ixgbe_init_eeprom_params_generic - Initialize EEPROM params
 *  @hw: pointer to hardware structure
 *
 *  Initializes the EEPROM parameters ixgbe_eeprom_info within the
 *  ixgbe_hw struct in order to set up EEPROM access.
 **/
s32 ixgbe_init_eeprom_params_generic(struct ixgbe_hw *hw)
{
	struct ixgbe_eeprom_info *eeprom = &hw->eeprom;
	u32 eec;
	u16 eeprom_size;

	DEBUGFUNC("ixgbe_init_eeprom_params_generic");

	if (eeprom->type == ixgbe_eeprom_uninitialized) {
		eeprom->type = ixgbe_eeprom_none;
		/* Set default semaphore delay to 10ms which is a well
		 * tested value */
		eeprom->semaphore_delay = 10;

		/*
		 * Check for EEPROM present first.
		 * If not present leave as none
		 */
		eec = IXGBE_READ_REG(hw, IXGBE_EEC);
		if (eec & IXGBE_EEC_PRES) {
			eeprom->type = ixgbe_eeprom_spi;

			/*
			 * SPI EEPROM is assumed here.  This code would need to
			 * change if a future EEPROM is not SPI.
			 */
			eeprom_size = (u16)((eec & IXGBE_EEC_SIZE) >>
			                    IXGBE_EEC_SIZE_SHIFT);
			eeprom->word_size = 1 << (eeprom_size +
			                     IXGBE_EEPROM_WORD_SIZE_BASE_SHIFT);
		}

		if (eec & IXGBE_EEC_ADDR_SIZE)
			eeprom->address_bits = 16;
		else
			eeprom->address_bits = 8;
		DEBUGOUT3("Eeprom params: type = %d, size = %d, address bits: "
		          "%d\n", eeprom->type, eeprom->word_size,
		          eeprom->address_bits);
	}

	return IXGBE_SUCCESS;
}

/**
 *  ixgbe_write_eeprom_generic - Writes 16 bit value to EEPROM
 *  @hw: pointer to hardware structure
 *  @offset: offset within the EEPROM to be written to
 *  @data: 16 bit word to be written to the EEPROM
 *
 *  If ixgbe_eeprom_update_checksum is not called after this function, the
 *  EEPROM will most likely contain an invalid checksum.
 **/
s32 ixgbe_write_eeprom_generic(struct ixgbe_hw *hw, u16 offset, u16 data)
{
	s32 status;
	u8 write_opcode = IXGBE_EEPROM_WRITE_OPCODE_SPI;

	DEBUGFUNC("ixgbe_write_eeprom_generic");

	hw->eeprom.ops.init_params(hw);

	if (offset >= hw->eeprom.word_size) {
		status = IXGBE_ERR_EEPROM;
		goto out;
	}

	/* Prepare the EEPROM for writing  */
	status = ixgbe_acquire_eeprom(hw);

	if (status == IXGBE_SUCCESS) {
		if (ixgbe_ready_eeprom(hw) != IXGBE_SUCCESS) {
			ixgbe_release_eeprom(hw);
			status = IXGBE_ERR_EEPROM;
		}
	}

	if (status == IXGBE_SUCCESS) {
		ixgbe_standby_eeprom(hw);

		/*  Send the WRITE ENABLE command (8 bit opcode )  */
		ixgbe_shift_out_eeprom_bits(hw, IXGBE_EEPROM_WREN_OPCODE_SPI,
		                            IXGBE_EEPROM_OPCODE_BITS);

		ixgbe_standby_eeprom(hw);

		/*
		 * Some SPI eeproms use the 8th address bit embedded in the
		 * opcode
		 */
		if ((hw->eeprom.address_bits == 8) && (offset >= 128))
			write_opcode |= IXGBE_EEPROM_A8_OPCODE_SPI;

		/* Send the Write command (8-bit opcode + addr) */
		ixgbe_shift_out_eeprom_bits(hw, write_opcode,
		                            IXGBE_EEPROM_OPCODE_BITS);
		ixgbe_shift_out_eeprom_bits(hw, (u16)(offset*2),
		                            hw->eeprom.address_bits);

		/* Send the data */
		data = (data >> 8) | (data << 8);
		ixgbe_shift_out_eeprom_bits(hw, data, 16);
		ixgbe_standby_eeprom(hw);

		/* Done with writing - release the EEPROM */
		ixgbe_release_eeprom(hw);
	}

out:
	return status;
}

/**
 *  ixgbe_read_eeprom_bit_bang_generic - Read EEPROM word using bit-bang
 *  @hw: pointer to hardware structure
 *  @offset: offset within the EEPROM to be read
 *  @data: read 16 bit value from EEPROM
 *
 *  Reads 16 bit value from EEPROM through bit-bang method
 **/
s32 ixgbe_read_eeprom_bit_bang_generic(struct ixgbe_hw *hw, u16 offset,
                                       u16 *data)
{
	s32 status;
	u16 word_in;
	u8 read_opcode = IXGBE_EEPROM_READ_OPCODE_SPI;

	DEBUGFUNC("ixgbe_read_eeprom_bit_bang_generic");

	hw->eeprom.ops.init_params(hw);

	if (offset >= hw->eeprom.word_size) {
		status = IXGBE_ERR_EEPROM;
		goto out;
	}

	/* Prepare the EEPROM for reading  */
	status = ixgbe_acquire_eeprom(hw);

	if (status == IXGBE_SUCCESS) {
		if (ixgbe_ready_eeprom(hw) != IXGBE_SUCCESS) {
			ixgbe_release_eeprom(hw);
			status = IXGBE_ERR_EEPROM;
		}
	}

	if (status == IXGBE_SUCCESS) {
		ixgbe_standby_eeprom(hw);

		/*
		 * Some SPI eeproms use the 8th address bit embedded in the
		 * opcode
		 */
		if ((hw->eeprom.address_bits == 8) && (offset >= 128))
			read_opcode |= IXGBE_EEPROM_A8_OPCODE_SPI;

		/* Send the READ command (opcode + addr) */
		ixgbe_shift_out_eeprom_bits(hw, read_opcode,
		                            IXGBE_EEPROM_OPCODE_BITS);
		ixgbe_shift_out_eeprom_bits(hw, (u16)(offset*2),
		                            hw->eeprom.address_bits);

		/* Read the data. */
		word_in = ixgbe_shift_in_eeprom_bits(hw, 16);
		*data = (word_in >> 8) | (word_in << 8);

		/* End this read operation */
		ixgbe_release_eeprom(hw);
	}

out:
	return status;
}

/**
 *  ixgbe_read_eerd_generic - Read EEPROM word using EERD
 *  @hw: pointer to hardware structure
 *  @offset: offset of  word in the EEPROM to read
 *  @data: word read from the EEPROM
 *
 *  Reads a 16 bit word from the EEPROM using the EERD register.
 **/
s32 ixgbe_read_eerd_generic(struct ixgbe_hw *hw, u16 offset, u16 *data)
{
	u32 eerd;
	s32 status;

	DEBUGFUNC("ixgbe_read_eerd_generic");

	hw->eeprom.ops.init_params(hw);

	if (offset >= hw->eeprom.word_size) {
		status = IXGBE_ERR_EEPROM;
		goto out;
	}

	eerd = (offset << IXGBE_EEPROM_RW_ADDR_SHIFT) +
	       IXGBE_EEPROM_RW_REG_START;

	IXGBE_WRITE_REG(hw, IXGBE_EERD, eerd);
	status = ixgbe_poll_eerd_eewr_done(hw, IXGBE_NVM_POLL_READ);

	if (status == IXGBE_SUCCESS)
		*data = (IXGBE_READ_REG(hw, IXGBE_EERD) >>
		         IXGBE_EEPROM_RW_REG_DATA);
	else
		DEBUGOUT("Eeprom read timed out\n");

out:
	return status;
}

/**
 *  ixgbe_write_eewr_generic - Write EEPROM word using EEWR
 *  @hw: pointer to hardware structure
 *  @offset: offset of  word in the EEPROM to write
 *  @data: word write to the EEPROM
 *
 *  Write a 16 bit word to the EEPROM using the EEWR register.
 **/
s32 ixgbe_write_eewr_generic(struct ixgbe_hw *hw, u16 offset, u16 data)
{
	u32 eewr;
	s32 status;

	DEBUGFUNC("ixgbe_write_eewr_generic");

	hw->eeprom.ops.init_params(hw);

	if (offset >= hw->eeprom.word_size) {
		status = IXGBE_ERR_EEPROM;
		goto out;
	}

	eewr = (offset << IXGBE_EEPROM_RW_ADDR_SHIFT) |
	       (data << IXGBE_EEPROM_RW_REG_DATA) | IXGBE_EEPROM_RW_REG_START;

	status = ixgbe_poll_eerd_eewr_done(hw, IXGBE_NVM_POLL_WRITE);
	if (status != IXGBE_SUCCESS) {
		DEBUGOUT("Eeprom write EEWR timed out\n");
		goto out;
	}

	IXGBE_WRITE_REG(hw, IXGBE_EEWR, eewr);

	status = ixgbe_poll_eerd_eewr_done(hw, IXGBE_NVM_POLL_WRITE);
	if (status != IXGBE_SUCCESS) {
		DEBUGOUT("Eeprom write EEWR timed out\n");
		goto out;
	}

out:
	return status;
}

/**
 *  ixgbe_poll_eerd_eewr_done - Poll EERD read or EEWR write status
 *  @hw: pointer to hardware structure
 *  @ee_reg: EEPROM flag for polling
 *
 *  Polls the status bit (bit 1) of the EERD or EEWR to determine when the
 *  read or write is done respectively.
 **/
s32 ixgbe_poll_eerd_eewr_done(struct ixgbe_hw *hw, u32 ee_reg)
{
	u32 i;
	u32 reg;
	s32 status = IXGBE_ERR_EEPROM;

	DEBUGFUNC("ixgbe_poll_eerd_eewr_done");

	for (i = 0; i < IXGBE_EERD_EEWR_ATTEMPTS; i++) {
		if (ee_reg == IXGBE_NVM_POLL_READ)
			reg = IXGBE_READ_REG(hw, IXGBE_EERD);
		else
			reg = IXGBE_READ_REG(hw, IXGBE_EEWR);

		if (reg & IXGBE_EEPROM_RW_REG_DONE) {
			status = IXGBE_SUCCESS;
			break;
		}
		usec_delay(5);
	}
	return status;
}

/**
 *  ixgbe_acquire_eeprom - Acquire EEPROM using bit-bang
 *  @hw: pointer to hardware structure
 *
 *  Prepares EEPROM for access using bit-bang method. This function should
 *  be called before issuing a command to the EEPROM.
 **/
static s32 ixgbe_acquire_eeprom(struct ixgbe_hw *hw)
{
	s32 status = IXGBE_SUCCESS;
	u32 eec;
	u32 i;

	DEBUGFUNC("ixgbe_acquire_eeprom");

	if (ixgbe_acquire_swfw_sync(hw, IXGBE_GSSR_EEP_SM) != IXGBE_SUCCESS)
		status = IXGBE_ERR_SWFW_SYNC;

	if (status == IXGBE_SUCCESS) {
		eec = IXGBE_READ_REG(hw, IXGBE_EEC);

		/* Request EEPROM Access */
		eec |= IXGBE_EEC_REQ;
		IXGBE_WRITE_REG(hw, IXGBE_EEC, eec);

		for (i = 0; i < IXGBE_EEPROM_GRANT_ATTEMPTS; i++) {
			eec = IXGBE_READ_REG(hw, IXGBE_EEC);
			if (eec & IXGBE_EEC_GNT)
				break;
			usec_delay(5);
		}

		/* Release if grant not acquired */
		if (!(eec & IXGBE_EEC_GNT)) {
			eec &= ~IXGBE_EEC_REQ;
			IXGBE_WRITE_REG(hw, IXGBE_EEC, eec);
			DEBUGOUT("Could not acquire EEPROM grant\n");

			ixgbe_release_swfw_sync(hw, IXGBE_GSSR_EEP_SM);
			status = IXGBE_ERR_EEPROM;
		}

		/* Setup EEPROM for Read/Write */
		if (status == IXGBE_SUCCESS) {
			/* Clear CS and SK */
			eec &= ~(IXGBE_EEC_CS | IXGBE_EEC_SK);
			IXGBE_WRITE_REG(hw, IXGBE_EEC, eec);
			IXGBE_WRITE_FLUSH(hw);
			usec_delay(1);
		}
	}
	return status;
}

/**
 *  ixgbe_get_eeprom_semaphore - Get hardware semaphore
 *  @hw: pointer to hardware structure
 *
 *  Sets the hardware semaphores so EEPROM access can occur for bit-bang method
 **/
static s32 ixgbe_get_eeprom_semaphore(struct ixgbe_hw *hw)
{
	s32 status = IXGBE_ERR_EEPROM;
	u32 timeout = 2000;
	u32 i;
	u32 swsm;

	DEBUGFUNC("ixgbe_get_eeprom_semaphore");


	/* Get SMBI software semaphore between device drivers first */
	for (i = 0; i < timeout; i++) {
		/*
		 * If the SMBI bit is 0 when we read it, then the bit will be
		 * set and we have the semaphore
		 */
		swsm = IXGBE_READ_REG(hw, IXGBE_SWSM);
		if (!(swsm & IXGBE_SWSM_SMBI)) {
			status = IXGBE_SUCCESS;
			break;
		}
		usec_delay(50);
	}

	/* Now get the semaphore between SW/FW through the SWESMBI bit */
	if (status == IXGBE_SUCCESS) {
		for (i = 0; i < timeout; i++) {
			swsm = IXGBE_READ_REG(hw, IXGBE_SWSM);

			/* Set the SW EEPROM semaphore bit to request access */
			swsm |= IXGBE_SWSM_SWESMBI;
			IXGBE_WRITE_REG(hw, IXGBE_SWSM, swsm);

			/*
			 * If we set the bit successfully then we got the
			 * semaphore.
			 */
			swsm = IXGBE_READ_REG(hw, IXGBE_SWSM);
			if (swsm & IXGBE_SWSM_SWESMBI)
				break;

			usec_delay(50);
		}

		/*
		 * Release semaphores and return error if SW EEPROM semaphore
		 * was not granted because we don't have access to the EEPROM
		 */
		if (i >= timeout) {
			DEBUGOUT("SWESMBI Software EEPROM semaphore "
			         "not granted.\n");
			ixgbe_release_eeprom_semaphore(hw);
			status = IXGBE_ERR_EEPROM;
		}
	} else {
		DEBUGOUT("Software semaphore SMBI between device drivers "
		         "not granted.\n");
	}

	return status;
}

/**
 *  ixgbe_release_eeprom_semaphore - Release hardware semaphore
 *  @hw: pointer to hardware structure
 *
 *  This function clears hardware semaphore bits.
 **/
static void ixgbe_release_eeprom_semaphore(struct ixgbe_hw *hw)
{
	u32 swsm;

	DEBUGFUNC("ixgbe_release_eeprom_semaphore");

	swsm = IXGBE_READ_REG(hw, IXGBE_SWSM);

	/* Release both semaphores by writing 0 to the bits SWESMBI and SMBI */
	swsm &= ~(IXGBE_SWSM_SWESMBI | IXGBE_SWSM_SMBI);
	IXGBE_WRITE_REG(hw, IXGBE_SWSM, swsm);
	IXGBE_WRITE_FLUSH(hw);
}

/**
 *  ixgbe_ready_eeprom - Polls for EEPROM ready
 *  @hw: pointer to hardware structure
 **/
static s32 ixgbe_ready_eeprom(struct ixgbe_hw *hw)
{
	s32 status = IXGBE_SUCCESS;
	u16 i;
	u8 spi_stat_reg;

	DEBUGFUNC("ixgbe_ready_eeprom");

	/*
	 * Read "Status Register" repeatedly until the LSB is cleared.  The
	 * EEPROM will signal that the command has been completed by clearing
	 * bit 0 of the internal status register.  If it's not cleared within
	 * 5 milliseconds, then error out.
	 */
	for (i = 0; i < IXGBE_EEPROM_MAX_RETRY_SPI; i += 5) {
		ixgbe_shift_out_eeprom_bits(hw, IXGBE_EEPROM_RDSR_OPCODE_SPI,
		                            IXGBE_EEPROM_OPCODE_BITS);
		spi_stat_reg = (u8)ixgbe_shift_in_eeprom_bits(hw, 8);
		if (!(spi_stat_reg & IXGBE_EEPROM_STATUS_RDY_SPI))
			break;

		usec_delay(5);
		ixgbe_standby_eeprom(hw);
	};

	/*
	 * On some parts, SPI write time could vary from 0-20mSec on 3.3V
	 * devices (and only 0-5mSec on 5V devices)
	 */
	if (i >= IXGBE_EEPROM_MAX_RETRY_SPI) {
		DEBUGOUT("SPI EEPROM Status error\n");
		status = IXGBE_ERR_EEPROM;
	}

	return status;
}

/**
 *  ixgbe_standby_eeprom - Returns EEPROM to a "standby" state
 *  @hw: pointer to hardware structure
 **/
static void ixgbe_standby_eeprom(struct ixgbe_hw *hw)
{
	u32 eec;

	DEBUGFUNC("ixgbe_standby_eeprom");

	eec = IXGBE_READ_REG(hw, IXGBE_EEC);

	/* Toggle CS to flush commands */
	eec |= IXGBE_EEC_CS;
	IXGBE_WRITE_REG(hw, IXGBE_EEC, eec);
	IXGBE_WRITE_FLUSH(hw);
	usec_delay(1);
	eec &= ~IXGBE_EEC_CS;
	IXGBE_WRITE_REG(hw, IXGBE_EEC, eec);
	IXGBE_WRITE_FLUSH(hw);
	usec_delay(1);
}

/**
 *  ixgbe_shift_out_eeprom_bits - Shift data bits out to the EEPROM.
 *  @hw: pointer to hardware structure
 *  @data: data to send to the EEPROM
 *  @count: number of bits to shift out
 **/
static void ixgbe_shift_out_eeprom_bits(struct ixgbe_hw *hw, u16 data,
                                        u16 count)
{
	u32 eec;
	u32 mask;
	u32 i;

	DEBUGFUNC("ixgbe_shift_out_eeprom_bits");

	eec = IXGBE_READ_REG(hw, IXGBE_EEC);

	/*
	 * Mask is used to shift "count" bits of "data" out to the EEPROM
	 * one bit at a time.  Determine the starting bit based on count
	 */
	mask = 0x01 << (count - 1);

	for (i = 0; i < count; i++) {
		/*
		 * A "1" is shifted out to the EEPROM by setting bit "DI" to a
		 * "1", and then raising and then lowering the clock (the SK
		 * bit controls the clock input to the EEPROM).  A "0" is
		 * shifted out to the EEPROM by setting "DI" to "0" and then
		 * raising and then lowering the clock.
		 */
		if (data & mask)
			eec |= IXGBE_EEC_DI;
		else
			eec &= ~IXGBE_EEC_DI;

		IXGBE_WRITE_REG(hw, IXGBE_EEC, eec);
		IXGBE_WRITE_FLUSH(hw);

		usec_delay(1);

		ixgbe_raise_eeprom_clk(hw, &eec);
		ixgbe_lower_eeprom_clk(hw, &eec);

		/*
		 * Shift mask to signify next bit of data to shift in to the
		 * EEPROM
		 */
		mask = mask >> 1;
	};

	/* We leave the "DI" bit set to "0" when we leave this routine. */
	eec &= ~IXGBE_EEC_DI;
	IXGBE_WRITE_REG(hw, IXGBE_EEC, eec);
	IXGBE_WRITE_FLUSH(hw);
}

/**
 *  ixgbe_shift_in_eeprom_bits - Shift data bits in from the EEPROM
 *  @hw: pointer to hardware structure
 **/
static u16 ixgbe_shift_in_eeprom_bits(struct ixgbe_hw *hw, u16 count)
{
	u32 eec;
	u32 i;
	u16 data = 0;

	DEBUGFUNC("ixgbe_shift_in_eeprom_bits");

	/*
	 * In order to read a register from the EEPROM, we need to shift
	 * 'count' bits in from the EEPROM. Bits are "shifted in" by raising
	 * the clock input to the EEPROM (setting the SK bit), and then reading
	 * the value of the "DO" bit.  During this "shifting in" process the
	 * "DI" bit should always be clear.
	 */
	eec = IXGBE_READ_REG(hw, IXGBE_EEC);

	eec &= ~(IXGBE_EEC_DO | IXGBE_EEC_DI);

	for (i = 0; i < count; i++) {
		data = data << 1;
		ixgbe_raise_eeprom_clk(hw, &eec);

		eec = IXGBE_READ_REG(hw, IXGBE_EEC);

		eec &= ~(IXGBE_EEC_DI);
		if (eec & IXGBE_EEC_DO)
			data |= 1;

		ixgbe_lower_eeprom_clk(hw, &eec);
	}

	return data;
}

/**
 *  ixgbe_raise_eeprom_clk - Raises the EEPROM's clock input.
 *  @hw: pointer to hardware structure
 *  @eec: EEC register's current value
 **/
static void ixgbe_raise_eeprom_clk(struct ixgbe_hw *hw, u32 *eec)
{
	DEBUGFUNC("ixgbe_raise_eeprom_clk");

	/*
	 * Raise the clock input to the EEPROM
	 * (setting the SK bit), then delay
	 */
	*eec = *eec | IXGBE_EEC_SK;
	IXGBE_WRITE_REG(hw, IXGBE_EEC, *eec);
	IXGBE_WRITE_FLUSH(hw);
	usec_delay(1);
}

/**
 *  ixgbe_lower_eeprom_clk - Lowers the EEPROM's clock input.
 *  @hw: pointer to hardware structure
 *  @eecd: EECD's current value
 **/
static void ixgbe_lower_eeprom_clk(struct ixgbe_hw *hw, u32 *eec)
{
	DEBUGFUNC("ixgbe_lower_eeprom_clk");

	/*
	 * Lower the clock input to the EEPROM (clearing the SK bit), then
	 * delay
	 */
	*eec = *eec & ~IXGBE_EEC_SK;
	IXGBE_WRITE_REG(hw, IXGBE_EEC, *eec);
	IXGBE_WRITE_FLUSH(hw);
	usec_delay(1);
}

/**
 *  ixgbe_release_eeprom - Release EEPROM, release semaphores
 *  @hw: pointer to hardware structure
 **/
static void ixgbe_release_eeprom(struct ixgbe_hw *hw)
{
	u32 eec;

	DEBUGFUNC("ixgbe_release_eeprom");

	eec = IXGBE_READ_REG(hw, IXGBE_EEC);

	eec |= IXGBE_EEC_CS;  /* Pull CS high */
	eec &= ~IXGBE_EEC_SK; /* Lower SCK */

	IXGBE_WRITE_REG(hw, IXGBE_EEC, eec);
	IXGBE_WRITE_FLUSH(hw);

	usec_delay(1);

	/* Stop requesting EEPROM access */
	eec &= ~IXGBE_EEC_REQ;
	IXGBE_WRITE_REG(hw, IXGBE_EEC, eec);

	ixgbe_release_swfw_sync(hw, IXGBE_GSSR_EEP_SM);

	/* Delay before attempt to obtain semaphore again to allow FW access */
	msec_delay(hw->eeprom.semaphore_delay);
}

/**
 *  ixgbe_calc_eeprom_checksum_generic - Calculates and returns the checksum
 *  @hw: pointer to hardware structure
 **/
u16 ixgbe_calc_eeprom_checksum_generic(struct ixgbe_hw *hw)
{
	u16 i;
	u16 j;
	u16 checksum = 0;
	u16 length = 0;
	u16 pointer = 0;
	u16 word = 0;

	DEBUGFUNC("ixgbe_calc_eeprom_checksum_generic");

	/* Include 0x0-0x3F in the checksum */
	for (i = 0; i < IXGBE_EEPROM_CHECKSUM; i++) {
		if (hw->eeprom.ops.read(hw, i, &word) != IXGBE_SUCCESS) {
			DEBUGOUT("EEPROM read failed\n");
			break;
		}
		checksum += word;
	}

	/* Include all data from pointers except for the fw pointer */
	for (i = IXGBE_PCIE_ANALOG_PTR; i < IXGBE_FW_PTR; i++) {
		hw->eeprom.ops.read(hw, i, &pointer);

		/* Make sure the pointer seems valid */
		if (pointer != 0xFFFF && pointer != 0) {
			hw->eeprom.ops.read(hw, pointer, &length);

			if (length != 0xFFFF && length != 0) {
				for (j = pointer+1; j <= pointer+length; j++) {
					hw->eeprom.ops.read(hw, j, &word);
					checksum += word;
				}
			}
		}
	}

	checksum = (u16)IXGBE_EEPROM_SUM - checksum;

	return checksum;
}

/**
 *  ixgbe_validate_eeprom_checksum_generic - Validate EEPROM checksum
 *  @hw: pointer to hardware structure
 *  @checksum_val: calculated checksum
 *
 *  Performs checksum calculation and validates the EEPROM checksum.  If the
 *  caller does not need checksum_val, the value can be NULL.
 **/
s32 ixgbe_validate_eeprom_checksum_generic(struct ixgbe_hw *hw,
                                           u16 *checksum_val)
{
	s32 status;
	u16 checksum;
	u16 read_checksum = 0;

	DEBUGFUNC("ixgbe_validate_eeprom_checksum_generic");

	/*
	 * Read the first word from the EEPROM. If this times out or fails, do
	 * not continue or we could be in for a very long wait while every
	 * EEPROM read fails
	 */
	status = hw->eeprom.ops.read(hw, 0, &checksum);

	if (status == IXGBE_SUCCESS) {
		checksum = hw->eeprom.ops.calc_checksum(hw);

		hw->eeprom.ops.read(hw, IXGBE_EEPROM_CHECKSUM, &read_checksum);

		/*
		 * Verify read checksum from EEPROM is the same as
		 * calculated checksum
		 */
		if (read_checksum != checksum)
			status = IXGBE_ERR_EEPROM_CHECKSUM;

		/* If the user cares, return the calculated checksum */
		if (checksum_val)
			*checksum_val = checksum;
	} else {
		DEBUGOUT("EEPROM read failed\n");
	}

	return status;
}

/**
 *  ixgbe_update_eeprom_checksum_generic - Updates the EEPROM checksum
 *  @hw: pointer to hardware structure
 **/
s32 ixgbe_update_eeprom_checksum_generic(struct ixgbe_hw *hw)
{
	s32 status;
	u16 checksum;

	DEBUGFUNC("ixgbe_update_eeprom_checksum_generic");

	/*
	 * Read the first word from the EEPROM. If this times out or fails, do
	 * not continue or we could be in for a very long wait while every
	 * EEPROM read fails
	 */
	status = hw->eeprom.ops.read(hw, 0, &checksum);

	if (status == IXGBE_SUCCESS) {
		checksum = hw->eeprom.ops.calc_checksum(hw);
		status = hw->eeprom.ops.write(hw, IXGBE_EEPROM_CHECKSUM,
		                              checksum);
	} else {
		DEBUGOUT("EEPROM read failed\n");
	}

	return status;
}

/**
 *  ixgbe_validate_mac_addr - Validate MAC address
 *  @mac_addr: pointer to MAC address.
 *
 *  Tests a MAC address to ensure it is a valid Individual Address
 **/
s32 ixgbe_validate_mac_addr(u8 *mac_addr)
{
	s32 status = IXGBE_SUCCESS;

	DEBUGFUNC("ixgbe_validate_mac_addr");

	/* Make sure it is not a multicast address */
	if (IXGBE_IS_MULTICAST(mac_addr)) {
		DEBUGOUT("MAC address is multicast\n");
		status = IXGBE_ERR_INVALID_MAC_ADDR;
	/* Not a broadcast address */
	} else if (IXGBE_IS_BROADCAST(mac_addr)) {
		DEBUGOUT("MAC address is broadcast\n");
		status = IXGBE_ERR_INVALID_MAC_ADDR;
	/* Reject the zero address */
	} else if (mac_addr[0] == 0 && mac_addr[1] == 0 && mac_addr[2] == 0 &&
	           mac_addr[3] == 0 && mac_addr[4] == 0 && mac_addr[5] == 0) {
		DEBUGOUT("MAC address is all zeros\n");
		status = IXGBE_ERR_INVALID_MAC_ADDR;
	}
	return status;
}

/**
 *  ixgbe_set_rar_generic - Set Rx address register
 *  @hw: pointer to hardware structure
 *  @index: Receive address register to write
 *  @addr: Address to put into receive address register
 *  @vmdq: VMDq "set" or "pool" index
 *  @enable_addr: set flag that address is active
 *
 *  Puts an ethernet address into a receive address register.
 **/
s32 ixgbe_set_rar_generic(struct ixgbe_hw *hw, u32 index, u8 *addr, u32 vmdq,
                          u32 enable_addr)
{
	u32 rar_low, rar_high;
	u32 rar_entries = hw->mac.num_rar_entries;

	DEBUGFUNC("ixgbe_set_rar_generic");

	/* Make sure we are using a valid rar index range */
	if (index >= rar_entries) {
		DEBUGOUT1("RAR index %d is out of range.\n", index);
		return IXGBE_ERR_INVALID_ARGUMENT;
	}

	/* setup VMDq pool selection before this RAR gets enabled */
	hw->mac.ops.set_vmdq(hw, index, vmdq);

	/*
	 * HW expects these in little endian so we reverse the byte
	 * order from network order (big endian) to little endian
	 */
	rar_low = ((u32)addr[0] |
	           ((u32)addr[1] << 8) |
	           ((u32)addr[2] << 16) |
	           ((u32)addr[3] << 24));
	/*
	 * Some parts put the VMDq setting in the extra RAH bits,
	 * so save everything except the lower 16 bits that hold part
	 * of the address and the address valid bit.
	 */
	rar_high = IXGBE_READ_REG(hw, IXGBE_RAH(index));
	rar_high &= ~(0x0000FFFF | IXGBE_RAH_AV);
	rar_high |= ((u32)addr[4] | ((u32)addr[5] << 8));

	if (enable_addr != 0)
		rar_high |= IXGBE_RAH_AV;

	IXGBE_WRITE_REG(hw, IXGBE_RAL(index), rar_low);
	IXGBE_WRITE_REG(hw, IXGBE_RAH(index), rar_high);

	return IXGBE_SUCCESS;
}

/**
 *  ixgbe_clear_rar_generic - Remove Rx address register
 *  @hw: pointer to hardware structure
 *  @index: Receive address register to write
 *
 *  Clears an ethernet address from a receive address register.
 **/
s32 ixgbe_clear_rar_generic(struct ixgbe_hw *hw, u32 index)
{
	u32 rar_high;
	u32 rar_entries = hw->mac.num_rar_entries;

	DEBUGFUNC("ixgbe_clear_rar_generic");

	/* Make sure we are using a valid rar index range */
	if (index >= rar_entries) {
		DEBUGOUT1("RAR index %d is out of range.\n", index);
		return IXGBE_ERR_INVALID_ARGUMENT;
	}

	/*
	 * Some parts put the VMDq setting in the extra RAH bits,
	 * so save everything except the lower 16 bits that hold part
	 * of the address and the address valid bit.
	 */
	rar_high = IXGBE_READ_REG(hw, IXGBE_RAH(index));
	rar_high &= ~(0x0000FFFF | IXGBE_RAH_AV);

	IXGBE_WRITE_REG(hw, IXGBE_RAL(index), 0);
	IXGBE_WRITE_REG(hw, IXGBE_RAH(index), rar_high);

	/* clear VMDq pool/queue selection for this RAR */
	hw->mac.ops.clear_vmdq(hw, index, IXGBE_CLEAR_VMDQ_ALL);

	return IXGBE_SUCCESS;
}

/**
 *  ixgbe_init_rx_addrs_generic - Initializes receive address filters.
 *  @hw: pointer to hardware structure
 *
 *  Places the MAC address in receive address register 0 and clears the rest
 *  of the receive address registers. Clears the multicast table. Assumes
 *  the receiver is in reset when the routine is called.
 **/
s32 ixgbe_init_rx_addrs_generic(struct ixgbe_hw *hw)
{
	u32 i;
	u32 rar_entries = hw->mac.num_rar_entries;

	DEBUGFUNC("ixgbe_init_rx_addrs_generic");

	/*
	 * If the current mac address is valid, assume it is a software override
	 * to the permanent address.
	 * Otherwise, use the permanent address from the eeprom.
	 */
	if (ixgbe_validate_mac_addr(hw->mac.addr) ==
	    IXGBE_ERR_INVALID_MAC_ADDR) {
		/* Get the MAC address from the RAR0 for later reference */
		hw->mac.ops.get_mac_addr(hw, hw->mac.addr);

		DEBUGOUT3(" Keeping Current RAR0 Addr =%.2X %.2X %.2X ",
		          hw->mac.addr[0], hw->mac.addr[1],
		          hw->mac.addr[2]);
		DEBUGOUT3("%.2X %.2X %.2X\n", hw->mac.addr[3],
		          hw->mac.addr[4], hw->mac.addr[5]);
	} else {
		/* Setup the receive address. */
		DEBUGOUT("Overriding MAC Address in RAR[0]\n");
		DEBUGOUT3(" New MAC Addr =%.2X %.2X %.2X ",
		          hw->mac.addr[0], hw->mac.addr[1],
		          hw->mac.addr[2]);
		DEBUGOUT3("%.2X %.2X %.2X\n", hw->mac.addr[3],
		          hw->mac.addr[4], hw->mac.addr[5]);

		hw->mac.ops.set_rar(hw, 0, hw->mac.addr, 0, IXGBE_RAH_AV);

		/* clear VMDq pool/queue selection for RAR 0 */
		hw->mac.ops.clear_vmdq(hw, 0, IXGBE_CLEAR_VMDQ_ALL);
	}
	hw->addr_ctrl.overflow_promisc = 0;

	hw->addr_ctrl.rar_used_count = 1;

	/* Zero out the other receive addresses. */
	DEBUGOUT1("Clearing RAR[1-%d]\n", rar_entries - 1);
	for (i = 1; i < rar_entries; i++) {
		IXGBE_WRITE_REG(hw, IXGBE_RAL(i), 0);
		IXGBE_WRITE_REG(hw, IXGBE_RAH(i), 0);
	}

	/* Clear the MTA */
	hw->addr_ctrl.mta_in_use = 0;
	IXGBE_WRITE_REG(hw, IXGBE_MCSTCTRL, hw->mac.mc_filter_type);

	DEBUGOUT(" Clearing MTA\n");
	for (i = 0; i < hw->mac.mcft_size; i++)
		IXGBE_WRITE_REG(hw, IXGBE_MTA(i), 0);

	ixgbe_init_uta_tables(hw);

	return IXGBE_SUCCESS;
}

/**
 *  ixgbe_add_uc_addr - Adds a secondary unicast address.
 *  @hw: pointer to hardware structure
 *  @addr: new address
 *
 *  Adds it to unused receive address register or goes into promiscuous mode.
 **/
void ixgbe_add_uc_addr(struct ixgbe_hw *hw, u8 *addr, u32 vmdq)
{
	u32 rar_entries = hw->mac.num_rar_entries;
	u32 rar;

	DEBUGFUNC("ixgbe_add_uc_addr");

	DEBUGOUT6(" UC Addr = %.2X %.2X %.2X %.2X %.2X %.2X\n",
	          addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);

	/*
	 * Place this address in the RAR if there is room,
	 * else put the controller into promiscuous mode
	 */
	if (hw->addr_ctrl.rar_used_count < rar_entries) {
		rar = hw->addr_ctrl.rar_used_count;
		hw->mac.ops.set_rar(hw, rar, addr, vmdq, IXGBE_RAH_AV);
		DEBUGOUT1("Added a secondary address to RAR[%d]\n", rar);
		hw->addr_ctrl.rar_used_count++;
	} else {
		hw->addr_ctrl.overflow_promisc++;
	}

	DEBUGOUT("ixgbe_add_uc_addr Complete\n");
}

/**
 *  ixgbe_update_uc_addr_list_generic - Updates MAC list of secondary addresses
 *  @hw: pointer to hardware structure
 *  @addr_list: the list of new addresses
 *  @addr_count: number of addresses
 *  @next: iterator function to walk the address list
 *
 *  The given list replaces any existing list.  Clears the secondary addrs from
 *  receive address registers.  Uses unused receive address registers for the
 *  first secondary addresses, and falls back to promiscuous mode as needed.
 *
 *  Drivers using secondary unicast addresses must set user_set_promisc when
 *  manually putting the device into promiscuous mode.
 **/
s32 ixgbe_update_uc_addr_list_generic(struct ixgbe_hw *hw, u8 *addr_list,
                                      u32 addr_count, ixgbe_mc_addr_itr next)
{
	u8 *addr;
	u32 i;
	u32 old_promisc_setting = hw->addr_ctrl.overflow_promisc;
	u32 uc_addr_in_use;
	u32 fctrl;
	u32 vmdq;

	DEBUGFUNC("ixgbe_update_uc_addr_list_generic");

	/*
	 * Clear accounting of old secondary address list,
	 * don't count RAR[0]
	 */
	uc_addr_in_use = hw->addr_ctrl.rar_used_count - 1;
	hw->addr_ctrl.rar_used_count -= uc_addr_in_use;
	hw->addr_ctrl.overflow_promisc = 0;

	/* Zero out the other receive addresses */
	DEBUGOUT1("Clearing RAR[1-%d]\n", uc_addr_in_use+1);
	for (i = 0; i < uc_addr_in_use; i++) {
		IXGBE_WRITE_REG(hw, IXGBE_RAL(1+i), 0);
		IXGBE_WRITE_REG(hw, IXGBE_RAH(1+i), 0);
	}

	/* Add the new addresses */
	for (i = 0; i < addr_count; i++) {
		DEBUGOUT(" Adding the secondary addresses:\n");
		addr = next(hw, &addr_list, &vmdq);
		ixgbe_add_uc_addr(hw, addr, vmdq);
	}

	if (hw->addr_ctrl.overflow_promisc) {
		/* enable promisc if not already in overflow or set by user */
		if (!old_promisc_setting && !hw->addr_ctrl.user_set_promisc) {
			DEBUGOUT(" Entering address overflow promisc mode\n");
			fctrl = IXGBE_READ_REG(hw, IXGBE_FCTRL);
			fctrl |= IXGBE_FCTRL_UPE;
			IXGBE_WRITE_REG(hw, IXGBE_FCTRL, fctrl);
		}
	} else {
		/* only disable if set by overflow, not by user */
		if (old_promisc_setting && !hw->addr_ctrl.user_set_promisc) {
			DEBUGOUT(" Leaving address overflow promisc mode\n");
			fctrl = IXGBE_READ_REG(hw, IXGBE_FCTRL);
			fctrl &= ~IXGBE_FCTRL_UPE;
			IXGBE_WRITE_REG(hw, IXGBE_FCTRL, fctrl);
		}
	}

	DEBUGOUT("ixgbe_update_uc_addr_list_generic Complete\n");
	return IXGBE_SUCCESS;
}

/**
 *  ixgbe_mta_vector - Determines bit-vector in multicast table to set
 *  @hw: pointer to hardware structure
 *  @mc_addr: the multicast address
 *
 *  Extracts the 12 bits, from a multicast address, to determine which
 *  bit-vector to set in the multicast table. The hardware uses 12 bits, from
 *  incoming rx multicast addresses, to determine the bit-vector to check in
 *  the MTA. Which of the 4 combination, of 12-bits, the hardware uses is set
 *  by the MO field of the MCSTCTRL. The MO field is set during initialization
 *  to mc_filter_type.
 **/
static s32 ixgbe_mta_vector(struct ixgbe_hw *hw, u8 *mc_addr)
{
	u32 vector = 0;

	DEBUGFUNC("ixgbe_mta_vector");

	switch (hw->mac.mc_filter_type) {
	case 0:   /* use bits [47:36] of the address */
		vector = ((mc_addr[4] >> 4) | (((u16)mc_addr[5]) << 4));
		break;
	case 1:   /* use bits [46:35] of the address */
		vector = ((mc_addr[4] >> 3) | (((u16)mc_addr[5]) << 5));
		break;
	case 2:   /* use bits [45:34] of the address */
		vector = ((mc_addr[4] >> 2) | (((u16)mc_addr[5]) << 6));
		break;
	case 3:   /* use bits [43:32] of the address */
		vector = ((mc_addr[4]) | (((u16)mc_addr[5]) << 8));
		break;
	default:  /* Invalid mc_filter_type */
		DEBUGOUT("MC filter type param set incorrectly\n");
		ASSERT(0);
		break;
	}

	/* vector can only be 12-bits or boundary will be exceeded */
	vector &= 0xFFF;
	return vector;
}

/**
 *  ixgbe_set_mta - Set bit-vector in multicast table
 *  @hw: pointer to hardware structure
 *  @hash_value: Multicast address hash value
 *
 *  Sets the bit-vector in the multicast table.
 **/
void ixgbe_set_mta(struct ixgbe_hw *hw, u8 *mc_addr)
{
	u32 vector;
	u32 vector_bit;
	u32 vector_reg;

	DEBUGFUNC("ixgbe_set_mta");

	hw->addr_ctrl.mta_in_use++;

	vector = ixgbe_mta_vector(hw, mc_addr);
	DEBUGOUT1(" bit-vector = 0x%03X\n", vector);

	/*
	 * The MTA is a register array of 128 32-bit registers. It is treated
	 * like an array of 4096 bits.  We want to set bit
	 * BitArray[vector_value]. So we figure out what register the bit is
	 * in, read it, OR in the new bit, then write back the new value.  The
	 * register is determined by the upper 7 bits of the vector value and
	 * the bit within that register are determined by the lower 5 bits of
	 * the value.
	 */
	vector_reg = (vector >> 5) & 0x7F;
	vector_bit = vector & 0x1F;
	hw->mac.mta_shadow[vector_reg] |= (1 << vector_bit);
}

/**
 *  ixgbe_update_mc_addr_list_generic - Updates MAC list of multicast addresses
 *  @hw: pointer to hardware structure
 *  @mc_addr_list: the list of new multicast addresses
 *  @mc_addr_count: number of addresses
 *  @next: iterator function to walk the multicast address list
 *
 *  The given list replaces any existing list. Clears the MC addrs from receive
 *  address registers and the multicast table. Uses unused receive address
 *  registers for the first multicast addresses, and hashes the rest into the
 *  multicast table.
 **/
s32 ixgbe_update_mc_addr_list_generic(struct ixgbe_hw *hw, u8 *mc_addr_list,
                                      u32 mc_addr_count, ixgbe_mc_addr_itr next)
{
	u32 i;
	u32 vmdq;

	DEBUGFUNC("ixgbe_update_mc_addr_list_generic");

	/*
	 * Set the new number of MC addresses that we are being requested to
	 * use.
	 */
	hw->addr_ctrl.num_mc_addrs = mc_addr_count;
	hw->addr_ctrl.mta_in_use = 0;

	/* Clear mta_shadow */
	DEBUGOUT(" Clearing MTA\n");
	memset(&hw->mac.mta_shadow, 0, sizeof(hw->mac.mta_shadow));

	/* Update mta_shadow */
	for (i = 0; i < mc_addr_count; i++) {
		DEBUGOUT(" Adding the multicast addresses:\n");
		ixgbe_set_mta(hw, next(hw, &mc_addr_list, &vmdq));
	}

	/* Enable mta */
	for (i = 0; i < hw->mac.mcft_size; i++)
		IXGBE_WRITE_REG_ARRAY(hw, IXGBE_MTA(0), i,
				      hw->mac.mta_shadow[i]);

	if (hw->addr_ctrl.mta_in_use > 0)
		IXGBE_WRITE_REG(hw, IXGBE_MCSTCTRL,
		                IXGBE_MCSTCTRL_MFE | hw->mac.mc_filter_type);

	DEBUGOUT("ixgbe_update_mc_addr_list_generic Complete\n");
	return IXGBE_SUCCESS;
}

/**
 *  ixgbe_enable_mc_generic - Enable multicast address in RAR
 *  @hw: pointer to hardware structure
 *
 *  Enables multicast address in RAR and the use of the multicast hash table.
 **/
s32 ixgbe_enable_mc_generic(struct ixgbe_hw *hw)
{
	struct ixgbe_addr_filter_info *a = &hw->addr_ctrl;

	DEBUGFUNC("ixgbe_enable_mc_generic");

	if (a->mta_in_use > 0)
		IXGBE_WRITE_REG(hw, IXGBE_MCSTCTRL, IXGBE_MCSTCTRL_MFE |
		                hw->mac.mc_filter_type);

	return IXGBE_SUCCESS;
}

/**
 *  ixgbe_disable_mc_generic - Disable multicast address in RAR
 *  @hw: pointer to hardware structure
 *
 *  Disables multicast address in RAR and the use of the multicast hash table.
 **/
s32 ixgbe_disable_mc_generic(struct ixgbe_hw *hw)
{
	struct ixgbe_addr_filter_info *a = &hw->addr_ctrl;

	DEBUGFUNC("ixgbe_disable_mc_generic");

	if (a->mta_in_use > 0)
		IXGBE_WRITE_REG(hw, IXGBE_MCSTCTRL, hw->mac.mc_filter_type);

	return IXGBE_SUCCESS;
}

/**
 *  ixgbe_fc_enable_generic - Enable flow control
 *  @hw: pointer to hardware structure
 *  @packetbuf_num: packet buffer number (0-7)
 *
 *  Enable flow control according to the current settings.
 **/
s32 ixgbe_fc_enable_generic(struct ixgbe_hw *hw, s32 packetbuf_num)
{
	s32 ret_val = IXGBE_SUCCESS;
	u32 mflcn_reg, fccfg_reg;
	u32 reg;
	u32 rx_pba_size;
	u32 fcrtl, fcrth;

	DEBUGFUNC("ixgbe_fc_enable_generic");

	/* Negotiate the fc mode to use */
	ret_val = ixgbe_fc_autoneg(hw);
	if (ret_val == IXGBE_ERR_FLOW_CONTROL)
		goto out;

	/* Disable any previous flow control settings */
	mflcn_reg = IXGBE_READ_REG(hw, IXGBE_MFLCN);
	mflcn_reg &= ~(IXGBE_MFLCN_RFCE | IXGBE_MFLCN_RPFCE);

	fccfg_reg = IXGBE_READ_REG(hw, IXGBE_FCCFG);
	fccfg_reg &= ~(IXGBE_FCCFG_TFCE_802_3X | IXGBE_FCCFG_TFCE_PRIORITY);

	/*
	 * The possible values of fc.current_mode are:
	 * 0: Flow control is completely disabled
	 * 1: Rx flow control is enabled (we can receive pause frames,
	 *    but not send pause frames).
	 * 2: Tx flow control is enabled (we can send pause frames but
	 *    we do not support receiving pause frames).
	 * 3: Both Rx and Tx flow control (symmetric) are enabled.
	 * other: Invalid.
	 */
	switch (hw->fc.current_mode) {
	case ixgbe_fc_none:
		/*
		 * Flow control is disabled by software override or autoneg.
		 * The code below will actually disable it in the HW.
		 */
		break;
	case ixgbe_fc_rx_pause:
		/*
		 * Rx Flow control is enabled and Tx Flow control is
		 * disabled by software override. Since there really
		 * isn't a way to advertise that we are capable of RX
		 * Pause ONLY, we will advertise that we support both
		 * symmetric and asymmetric Rx PAUSE.  Later, we will
		 * disable the adapter's ability to send PAUSE frames.
		 */
		mflcn_reg |= IXGBE_MFLCN_RFCE;
		break;
	case ixgbe_fc_tx_pause:
		/*
		 * Tx Flow control is enabled, and Rx Flow control is
		 * disabled by software override.
		 */
		fccfg_reg |= IXGBE_FCCFG_TFCE_802_3X;
		break;
	case ixgbe_fc_full:
		/* Flow control (both Rx and Tx) is enabled by SW override. */
		mflcn_reg |= IXGBE_MFLCN_RFCE;
		fccfg_reg |= IXGBE_FCCFG_TFCE_802_3X;
		break;
	default:
		DEBUGOUT("Flow control param set incorrectly\n");
		ret_val = IXGBE_ERR_CONFIG;
		goto out;
		break;
	}

	/* Set 802.3x based flow control settings. */
	mflcn_reg |= IXGBE_MFLCN_DPF;
	IXGBE_WRITE_REG(hw, IXGBE_MFLCN, mflcn_reg);
	IXGBE_WRITE_REG(hw, IXGBE_FCCFG, fccfg_reg);

	rx_pba_size = IXGBE_READ_REG(hw, IXGBE_RXPBSIZE(packetbuf_num));
	rx_pba_size >>= IXGBE_RXPBSIZE_SHIFT;

	fcrth = (rx_pba_size - hw->fc.high_water) << 10;
	fcrtl = (rx_pba_size - hw->fc.low_water) << 10;

	if (hw->fc.current_mode & ixgbe_fc_tx_pause) {
		fcrth |= IXGBE_FCRTH_FCEN;
		if (hw->fc.send_xon)
			fcrtl |= IXGBE_FCRTL_XONE;
	}

	IXGBE_WRITE_REG(hw, IXGBE_FCRTH_82599(packetbuf_num), fcrth);
	IXGBE_WRITE_REG(hw, IXGBE_FCRTL_82599(packetbuf_num), fcrtl);

	/* Configure pause time (2 TCs per register) */
	reg = IXGBE_READ_REG(hw, IXGBE_FCTTV(packetbuf_num / 2));
	if ((packetbuf_num & 1) == 0)
		reg = (reg & 0xFFFF0000) | hw->fc.pause_time;
	else
		reg = (reg & 0x0000FFFF) | (hw->fc.pause_time << 16);
	IXGBE_WRITE_REG(hw, IXGBE_FCTTV(packetbuf_num / 2), reg);

	IXGBE_WRITE_REG(hw, IXGBE_FCRTV, (hw->fc.pause_time >> 1));

out:
	return ret_val;
}

/**
 *  ixgbe_fc_autoneg - Configure flow control
 *  @hw: pointer to hardware structure
 *
 *  Compares our advertised flow control capabilities to those advertised by
 *  our link partner, and determines the proper flow control mode to use.
 **/
s32 ixgbe_fc_autoneg(struct ixgbe_hw *hw)
{
	s32 ret_val = IXGBE_ERR_FC_NOT_NEGOTIATED;
	ixgbe_link_speed speed;
	bool link_up;

	DEBUGFUNC("ixgbe_fc_autoneg");

	if (hw->fc.disable_fc_autoneg)
		goto out;

	/*
	 * AN should have completed when the cable was plugged in.
	 * Look for reasons to bail out.  Bail out if:
	 * - FC autoneg is disabled, or if
	 * - link is not up.
	 *
	 * Since we're being called from an LSC, link is already known to be up.
	 * So use link_up_wait_to_complete=FALSE.
	 */
	hw->mac.ops.check_link(hw, &speed, &link_up, FALSE);
	if (!link_up) {
		ret_val = IXGBE_ERR_FLOW_CONTROL;
		goto out;
	}

	switch (hw->phy.media_type) {
	/* Autoneg flow control on fiber adapters */
	case ixgbe_media_type_fiber:
		if (speed == IXGBE_LINK_SPEED_1GB_FULL)
			ret_val = ixgbe_fc_autoneg_fiber(hw);
		break;

	/* Autoneg flow control on backplane adapters */
	case ixgbe_media_type_backplane:
		ret_val = ixgbe_fc_autoneg_backplane(hw);
		break;

	/* Autoneg flow control on copper adapters */
	case ixgbe_media_type_copper:
		if (ixgbe_device_supports_autoneg_fc(hw) == IXGBE_SUCCESS)
			ret_val = ixgbe_fc_autoneg_copper(hw);
		break;

	default:
		break;
	}

out:
	if (ret_val == IXGBE_SUCCESS) {
		hw->fc.fc_was_autonegged = TRUE;
	} else {
		hw->fc.fc_was_autonegged = FALSE;
		hw->fc.current_mode = hw->fc.requested_mode;
	}
	return ret_val;
}

/**
 *  ixgbe_fc_autoneg_fiber - Enable flow control on 1 gig fiber
 *  @hw: pointer to hardware structure
 *  @speed:
 *  @link_up
 *
 *  Enable flow control according on 1 gig fiber.
 **/
static s32 ixgbe_fc_autoneg_fiber(struct ixgbe_hw *hw)
{
	u32 pcs_anadv_reg, pcs_lpab_reg, linkstat;
	s32 ret_val;

	/*
	 * On multispeed fiber at 1g, bail out if
	 * - link is up but AN did not complete, or if
	 * - link is up and AN completed but timed out
	 */

	linkstat = IXGBE_READ_REG(hw, IXGBE_PCS1GLSTA);
	if (((linkstat & IXGBE_PCS1GLSTA_AN_COMPLETE) == 0) ||
	    ((linkstat & IXGBE_PCS1GLSTA_AN_TIMED_OUT) != 0)) {
		ret_val = IXGBE_ERR_FC_NOT_NEGOTIATED;
		goto out;
	}

	pcs_anadv_reg = IXGBE_READ_REG(hw, IXGBE_PCS1GANA);
	pcs_lpab_reg = IXGBE_READ_REG(hw, IXGBE_PCS1GANLP);

	ret_val =  ixgbe_negotiate_fc(hw, pcs_anadv_reg,
			       pcs_lpab_reg, IXGBE_PCS1GANA_SYM_PAUSE,
			       IXGBE_PCS1GANA_ASM_PAUSE,
			       IXGBE_PCS1GANA_SYM_PAUSE,
			       IXGBE_PCS1GANA_ASM_PAUSE);

out:
	return ret_val;
}

/**
 *  ixgbe_fc_autoneg_backplane - Enable flow control IEEE clause 37
 *  @hw: pointer to hardware structure
 *
 *  Enable flow control according to IEEE clause 37.
 **/
static s32 ixgbe_fc_autoneg_backplane(struct ixgbe_hw *hw)
{
	u32 links2, anlp1_reg, autoc_reg, links;
	s32 ret_val;

	/*
	 * On backplane, bail out if
	 * - backplane autoneg was not completed, or if
	 * - we are 82599 and link partner is not AN enabled
	 */
	links = IXGBE_READ_REG(hw, IXGBE_LINKS);
	if ((links & IXGBE_LINKS_KX_AN_COMP) == 0) {
		hw->fc.fc_was_autonegged = FALSE;
		hw->fc.current_mode = hw->fc.requested_mode;
		ret_val = IXGBE_ERR_FC_NOT_NEGOTIATED;
		goto out;
	}

	if (hw->mac.type == ixgbe_mac_82599EB) {
		links2 = IXGBE_READ_REG(hw, IXGBE_LINKS2);
		if ((links2 & IXGBE_LINKS2_AN_SUPPORTED) == 0) {
			hw->fc.fc_was_autonegged = FALSE;
			hw->fc.current_mode = hw->fc.requested_mode;
			ret_val = IXGBE_ERR_FC_NOT_NEGOTIATED;
			goto out;
		}
	}
	/*
	 * Read the 10g AN autoc and LP ability registers and resolve
	 * local flow control settings accordingly
	 */
	autoc_reg = IXGBE_READ_REG(hw, IXGBE_AUTOC);
	anlp1_reg = IXGBE_READ_REG(hw, IXGBE_ANLP1);

	ret_val = ixgbe_negotiate_fc(hw, autoc_reg,
		anlp1_reg, IXGBE_AUTOC_SYM_PAUSE, IXGBE_AUTOC_ASM_PAUSE,
		IXGBE_ANLP1_SYM_PAUSE, IXGBE_ANLP1_ASM_PAUSE);

out:
	return ret_val;
}

/**
 *  ixgbe_fc_autoneg_copper - Enable flow control IEEE clause 37
 *  @hw: pointer to hardware structure
 *
 *  Enable flow control according to IEEE clause 37.
 **/
static s32 ixgbe_fc_autoneg_copper(struct ixgbe_hw *hw)
{
	u16 technology_ability_reg = 0;
	u16 lp_technology_ability_reg = 0;

	hw->phy.ops.read_reg(hw, IXGBE_MDIO_AUTO_NEG_ADVT,
			     IXGBE_MDIO_AUTO_NEG_DEV_TYPE,
			     &technology_ability_reg);
	hw->phy.ops.read_reg(hw, IXGBE_MDIO_AUTO_NEG_LP,
			     IXGBE_MDIO_AUTO_NEG_DEV_TYPE,
			     &lp_technology_ability_reg);

	return ixgbe_negotiate_fc(hw, (u32)technology_ability_reg,
				  (u32)lp_technology_ability_reg,
				  IXGBE_TAF_SYM_PAUSE, IXGBE_TAF_ASM_PAUSE,
				  IXGBE_TAF_SYM_PAUSE, IXGBE_TAF_ASM_PAUSE);
}

/**
 *  ixgbe_negotiate_fc - Negotiate flow control
 *  @hw: pointer to hardware structure
 *  @adv_reg: flow control advertised settings
 *  @lp_reg: link partner's flow control settings
 *  @adv_sym: symmetric pause bit in advertisement
 *  @adv_asm: asymmetric pause bit in advertisement
 *  @lp_sym: symmetric pause bit in link partner advertisement
 *  @lp_asm: asymmetric pause bit in link partner advertisement
 *
 *  Find the intersection between advertised settings and link partner's
 *  advertised settings
 **/
static s32 ixgbe_negotiate_fc(struct ixgbe_hw *hw, u32 adv_reg, u32 lp_reg,
			      u32 adv_sym, u32 adv_asm, u32 lp_sym, u32 lp_asm)
{
	if ((!(adv_reg)) ||  (!(lp_reg)))
		return IXGBE_ERR_FC_NOT_NEGOTIATED;

	if ((adv_reg & adv_sym) && (lp_reg & lp_sym)) {
		/*
		 * Now we need to check if the user selected Rx ONLY
		 * of pause frames.  In this case, we had to advertise
		 * FULL flow control because we could not advertise RX
		 * ONLY. Hence, we must now check to see if we need to
		 * turn OFF the TRANSMISSION of PAUSE frames.
		 */
		if (hw->fc.requested_mode == ixgbe_fc_full) {
			hw->fc.current_mode = ixgbe_fc_full;
			DEBUGOUT("Flow Control = FULL.\n");
		} else {
			hw->fc.current_mode = ixgbe_fc_rx_pause;
			DEBUGOUT("Flow Control=RX PAUSE frames only\n");
		}
	} else if (!(adv_reg & adv_sym) && (adv_reg & adv_asm) &&
		   (lp_reg & lp_sym) && (lp_reg & lp_asm)) {
		hw->fc.current_mode = ixgbe_fc_tx_pause;
		DEBUGOUT("Flow Control = TX PAUSE frames only.\n");
	} else if ((adv_reg & adv_sym) && (adv_reg & adv_asm) &&
		   !(lp_reg & lp_sym) && (lp_reg & lp_asm)) {
		hw->fc.current_mode = ixgbe_fc_rx_pause;
		DEBUGOUT("Flow Control = RX PAUSE frames only.\n");
	} else {
		hw->fc.current_mode = ixgbe_fc_none;
		DEBUGOUT("Flow Control = NONE.\n");
	}
	return IXGBE_SUCCESS;
}

/**
 *  ixgbe_setup_fc - Set up flow control
 *  @hw: pointer to hardware structure
 *
 *  Called at init time to set up flow control.
 **/
s32 ixgbe_setup_fc(struct ixgbe_hw *hw, s32 packetbuf_num)
{
	s32 ret_val = IXGBE_SUCCESS;
	u32 reg = 0, reg_bp = 0;
	u16 reg_cu = 0;

	DEBUGFUNC("ixgbe_setup_fc");

	/* Validate the packetbuf configuration */
	if (packetbuf_num < 0 || packetbuf_num > 7) {
		DEBUGOUT1("Invalid packet buffer number [%d], expected range is"
		          " 0-7\n", packetbuf_num);
		ret_val = IXGBE_ERR_INVALID_LINK_SETTINGS;
		goto out;
	}

	/*
	 * Validate the water mark configuration.  Zero water marks are invalid
	 * because it causes the controller to just blast out fc packets.
	 */
	if (!hw->fc.low_water || !hw->fc.high_water || !hw->fc.pause_time) {
		DEBUGOUT("Invalid water mark configuration\n");
		ret_val = IXGBE_ERR_INVALID_LINK_SETTINGS;
		goto out;
	}

	/*
	 * Validate the requested mode.  Strict IEEE mode does not allow
	 * ixgbe_fc_rx_pause because it will cause us to fail at UNH.
	 */
	if (hw->fc.strict_ieee && hw->fc.requested_mode == ixgbe_fc_rx_pause) {
		DEBUGOUT("ixgbe_fc_rx_pause not valid in strict IEEE mode\n");
		ret_val = IXGBE_ERR_INVALID_LINK_SETTINGS;
		goto out;
	}

	/*
	 * 10gig parts do not have a word in the EEPROM to determine the
	 * default flow control setting, so we explicitly set it to full.
	 */
	if (hw->fc.requested_mode == ixgbe_fc_default)
		hw->fc.requested_mode = ixgbe_fc_full;

	/*
	 * Set up the 1G and 10G flow control advertisement registers so the
	 * HW will be able to do fc autoneg once the cable is plugged in.  If
	 * we link at 10G, the 1G advertisement is harmless and vice versa.
	 */

	switch (hw->phy.media_type) {
	case ixgbe_media_type_fiber:
	case ixgbe_media_type_backplane:
		reg = IXGBE_READ_REG(hw, IXGBE_PCS1GANA);
		reg_bp = IXGBE_READ_REG(hw, IXGBE_AUTOC);
		break;

	case ixgbe_media_type_copper:
		hw->phy.ops.read_reg(hw, IXGBE_MDIO_AUTO_NEG_ADVT,
					IXGBE_MDIO_AUTO_NEG_DEV_TYPE, &reg_cu);
		break;

	default:
		;
	}

	/*
	 * The possible values of fc.requested_mode are:
	 * 0: Flow control is completely disabled
	 * 1: Rx flow control is enabled (we can receive pause frames,
	 *    but not send pause frames).
	 * 2: Tx flow control is enabled (we can send pause frames but
	 *    we do not support receiving pause frames).
	 * 3: Both Rx and Tx flow control (symmetric) are enabled.
	 * other: Invalid.
	 */
	switch (hw->fc.requested_mode) {
	case ixgbe_fc_none:
		/* Flow control completely disabled by software override. */
		reg &= ~(IXGBE_PCS1GANA_SYM_PAUSE | IXGBE_PCS1GANA_ASM_PAUSE);
		if (hw->phy.media_type == ixgbe_media_type_backplane)
			reg_bp &= ~(IXGBE_AUTOC_SYM_PAUSE |
				    IXGBE_AUTOC_ASM_PAUSE);
		else if (hw->phy.media_type == ixgbe_media_type_copper)
			reg_cu &= ~(IXGBE_TAF_SYM_PAUSE | IXGBE_TAF_ASM_PAUSE);
		break;
	case ixgbe_fc_rx_pause:
		/*
		 * Rx Flow control is enabled and Tx Flow control is
		 * disabled by software override. Since there really
		 * isn't a way to advertise that we are capable of RX
		 * Pause ONLY, we will advertise that we support both
		 * symmetric and asymmetric Rx PAUSE.  Later, we will
		 * disable the adapter's ability to send PAUSE frames.
		 */
		reg |= (IXGBE_PCS1GANA_SYM_PAUSE | IXGBE_PCS1GANA_ASM_PAUSE);
		if (hw->phy.media_type == ixgbe_media_type_backplane)
			reg_bp |= (IXGBE_AUTOC_SYM_PAUSE |
				   IXGBE_AUTOC_ASM_PAUSE);
		else if (hw->phy.media_type == ixgbe_media_type_copper)
			reg_cu |= (IXGBE_TAF_SYM_PAUSE | IXGBE_TAF_ASM_PAUSE);
		break;
	case ixgbe_fc_tx_pause:
		/*
		 * Tx Flow control is enabled, and Rx Flow control is
		 * disabled by software override.
		 */
		reg |= (IXGBE_PCS1GANA_ASM_PAUSE);
		reg &= ~(IXGBE_PCS1GANA_SYM_PAUSE);
		if (hw->phy.media_type == ixgbe_media_type_backplane) {
			reg_bp |= (IXGBE_AUTOC_ASM_PAUSE);
			reg_bp &= ~(IXGBE_AUTOC_SYM_PAUSE);
		} else if (hw->phy.media_type == ixgbe_media_type_copper) {
			reg_cu |= (IXGBE_TAF_ASM_PAUSE);
			reg_cu &= ~(IXGBE_TAF_SYM_PAUSE);
		}
		break;
	case ixgbe_fc_full:
		/* Flow control (both Rx and Tx) is enabled by SW override. */
		reg |= (IXGBE_PCS1GANA_SYM_PAUSE | IXGBE_PCS1GANA_ASM_PAUSE);
		if (hw->phy.media_type == ixgbe_media_type_backplane)
			reg_bp |= (IXGBE_AUTOC_SYM_PAUSE |
				   IXGBE_AUTOC_ASM_PAUSE);
		else if (hw->phy.media_type == ixgbe_media_type_copper)
			reg_cu |= (IXGBE_TAF_SYM_PAUSE | IXGBE_TAF_ASM_PAUSE);
		break;
	default:
		DEBUGOUT("Flow control param set incorrectly\n");
		ret_val = IXGBE_ERR_CONFIG;
		goto out;
		break;
	}

	/*
	 * Enable auto-negotiation between the MAC & PHY;
	 * the MAC will advertise clause 37 flow control.
	 */
	IXGBE_WRITE_REG(hw, IXGBE_PCS1GANA, reg);
	reg = IXGBE_READ_REG(hw, IXGBE_PCS1GLCTL);

	/* Disable AN timeout */
	if (hw->fc.strict_ieee)
		reg &= ~IXGBE_PCS1GLCTL_AN_1G_TIMEOUT_EN;

	IXGBE_WRITE_REG(hw, IXGBE_PCS1GLCTL, reg);
	DEBUGOUT1("Set up FC; PCS1GLCTL = 0x%08X\n", reg);

	/*
	 * AUTOC restart handles negotiation of 1G and 10G on backplane
	 * and copper. There is no need to set the PCS1GCTL register.
	 *
	 */
	if (hw->phy.media_type == ixgbe_media_type_backplane) {
		reg_bp |= IXGBE_AUTOC_AN_RESTART;
		IXGBE_WRITE_REG(hw, IXGBE_AUTOC, reg_bp);
	} else if ((hw->phy.media_type == ixgbe_media_type_copper) &&
		    (ixgbe_device_supports_autoneg_fc(hw) == IXGBE_SUCCESS)) {
		hw->phy.ops.write_reg(hw, IXGBE_MDIO_AUTO_NEG_ADVT,
				      IXGBE_MDIO_AUTO_NEG_DEV_TYPE, reg_cu);
	}

	DEBUGOUT1("Set up FC; IXGBE_AUTOC = 0x%08X\n", reg);
out:
	return ret_val;
}

/**
 *  ixgbe_disable_pcie_master - Disable PCI-express master access
 *  @hw: pointer to hardware structure
 *
 *  Disables PCI-Express master access and verifies there are no pending
 *  requests. IXGBE_ERR_MASTER_REQUESTS_PENDING is returned if master disable
 *  bit hasn't caused the master requests to be disabled, else IXGBE_SUCCESS
 *  is returned signifying master requests disabled.
 **/
s32 ixgbe_disable_pcie_master(struct ixgbe_hw *hw)
{
	u32 i;
	u32 reg_val;
	u32 number_of_queues;
	s32 status = IXGBE_SUCCESS;

	DEBUGFUNC("ixgbe_disable_pcie_master");

	/* Just jump out if bus mastering is already disabled */
	if (!(IXGBE_READ_REG(hw, IXGBE_STATUS) & IXGBE_STATUS_GIO))
		goto out;

	/* Disable the receive unit by stopping each queue */
	number_of_queues = hw->mac.max_rx_queues;
	for (i = 0; i < number_of_queues; i++) {
		reg_val = IXGBE_READ_REG(hw, IXGBE_RXDCTL(i));
		if (reg_val & IXGBE_RXDCTL_ENABLE) {
			reg_val &= ~IXGBE_RXDCTL_ENABLE;
			IXGBE_WRITE_REG(hw, IXGBE_RXDCTL(i), reg_val);
		}
	}

	reg_val = IXGBE_READ_REG(hw, IXGBE_CTRL);
	reg_val |= IXGBE_CTRL_GIO_DIS;
	IXGBE_WRITE_REG(hw, IXGBE_CTRL, reg_val);

	for (i = 0; i < IXGBE_PCI_MASTER_DISABLE_TIMEOUT; i++) {
		if (!(IXGBE_READ_REG(hw, IXGBE_STATUS) & IXGBE_STATUS_GIO))
			goto check_device_status;
		usec_delay(100);
	}

	DEBUGOUT("GIO Master Disable bit didn't clear - requesting resets\n");
	status = IXGBE_ERR_MASTER_REQUESTS_PENDING;

	/*
	 * Before proceeding, make sure that the PCIe block does not have
	 * transactions pending.
	 */
check_device_status:
	for (i = 0; i < IXGBE_PCI_MASTER_DISABLE_TIMEOUT; i++) {
		if (!(IXGBE_READ_PCIE_WORD(hw, IXGBE_PCI_DEVICE_STATUS) &
			IXGBE_PCI_DEVICE_STATUS_TRANSACTION_PENDING))
			break;
		usec_delay(100);
	}

	if (i == IXGBE_PCI_MASTER_DISABLE_TIMEOUT)
		DEBUGOUT("PCIe transaction pending bit also did not clear.\n");
	else
		goto out;

	/*
	 * Two consecutive resets are required via CTRL.RST per datasheet
	 * 5.2.5.3.2 Master Disable.  We set a flag to inform the reset routine
	 * of this need.  The first reset prevents new master requests from
	 * being issued by our device.  We then must wait 1usec for any
	 * remaining completions from the PCIe bus to trickle in, and then reset
	 * again to clear out any effects they may have had on our device.
	 */
	 hw->mac.flags |= IXGBE_FLAGS_DOUBLE_RESET_REQUIRED;

out:
	return status;
}


/**
 *  ixgbe_acquire_swfw_sync - Acquire SWFW semaphore
 *  @hw: pointer to hardware structure
 *  @mask: Mask to specify which semaphore to acquire
 *
 *  Acquires the SWFW semaphore thought the GSSR register for the specified
 *  function (CSR, PHY0, PHY1, EEPROM, Flash)
 **/
s32 ixgbe_acquire_swfw_sync(struct ixgbe_hw *hw, u16 mask)
{
	u32 gssr;
	u32 swmask = mask;
	u32 fwmask = mask << 5;
	s32 timeout = 200;

	DEBUGFUNC("ixgbe_acquire_swfw_sync");

	while (timeout) {
		/*
		 * SW EEPROM semaphore bit is used for access to all
		 * SW_FW_SYNC/GSSR bits (not just EEPROM)
		 */
		if (ixgbe_get_eeprom_semaphore(hw))
			return IXGBE_ERR_SWFW_SYNC;

		gssr = IXGBE_READ_REG(hw, IXGBE_GSSR);
		if (!(gssr & (fwmask | swmask)))
			break;

		/*
		 * Firmware currently using resource (fwmask) or other software
		 * thread currently using resource (swmask)
		 */
		ixgbe_release_eeprom_semaphore(hw);
		msec_delay(5);
		timeout--;
	}

	if (!timeout) {
		DEBUGOUT("Driver can't access resource, SW_FW_SYNC timeout.\n");
		return IXGBE_ERR_SWFW_SYNC;
	}

	gssr |= swmask;
	IXGBE_WRITE_REG(hw, IXGBE_GSSR, gssr);

	ixgbe_release_eeprom_semaphore(hw);
	return IXGBE_SUCCESS;
}

/**
 *  ixgbe_release_swfw_sync - Release SWFW semaphore
 *  @hw: pointer to hardware structure
 *  @mask: Mask to specify which semaphore to release
 *
 *  Releases the SWFW semaphore thought the GSSR register for the specified
 *  function (CSR, PHY0, PHY1, EEPROM, Flash)
 **/
void ixgbe_release_swfw_sync(struct ixgbe_hw *hw, u16 mask)
{
	u32 gssr;
	u32 swmask = mask;

	DEBUGFUNC("ixgbe_release_swfw_sync");

	ixgbe_get_eeprom_semaphore(hw);

	gssr = IXGBE_READ_REG(hw, IXGBE_GSSR);
	gssr &= ~swmask;
	IXGBE_WRITE_REG(hw, IXGBE_GSSR, gssr);

	ixgbe_release_eeprom_semaphore(hw);
}

/**
 *  ixgbe_enable_rx_dma_generic - Enable the Rx DMA unit
 *  @hw: pointer to hardware structure
 *  @regval: register value to write to RXCTRL
 *
 *  Enables the Rx DMA unit
 **/
s32 ixgbe_enable_rx_dma_generic(struct ixgbe_hw *hw, u32 regval)
{
	DEBUGFUNC("ixgbe_enable_rx_dma_generic");

	IXGBE_WRITE_REG(hw, IXGBE_RXCTRL, regval);

	return IXGBE_SUCCESS;
}

/**
 *  ixgbe_blink_led_start_generic - Blink LED based on index.
 *  @hw: pointer to hardware structure
 *  @index: led number to blink
 **/
s32 ixgbe_blink_led_start_generic(struct ixgbe_hw *hw, u32 index)
{
	ixgbe_link_speed speed = 0;
	bool link_up = 0;
	u32 autoc_reg = IXGBE_READ_REG(hw, IXGBE_AUTOC);
	u32 led_reg = IXGBE_READ_REG(hw, IXGBE_LEDCTL);

	DEBUGFUNC("ixgbe_blink_led_start_generic");

	/*
	 * Link must be up to auto-blink the LEDs;
	 * Force it if link is down.
	 */
	hw->mac.ops.check_link(hw, &speed, &link_up, FALSE);

	if (!link_up) {
		autoc_reg |= IXGBE_AUTOC_AN_RESTART;
		autoc_reg |= IXGBE_AUTOC_FLU;
		IXGBE_WRITE_REG(hw, IXGBE_AUTOC, autoc_reg);
		msec_delay(10);
	}

	led_reg &= ~IXGBE_LED_MODE_MASK(index);
	led_reg |= IXGBE_LED_BLINK(index);
	IXGBE_WRITE_REG(hw, IXGBE_LEDCTL, led_reg);
	IXGBE_WRITE_FLUSH(hw);

	return IXGBE_SUCCESS;
}

/**
 *  ixgbe_blink_led_stop_generic - Stop blinking LED based on index.
 *  @hw: pointer to hardware structure
 *  @index: led number to stop blinking
 **/
s32 ixgbe_blink_led_stop_generic(struct ixgbe_hw *hw, u32 index)
{
	u32 autoc_reg = IXGBE_READ_REG(hw, IXGBE_AUTOC);
	u32 led_reg = IXGBE_READ_REG(hw, IXGBE_LEDCTL);

	DEBUGFUNC("ixgbe_blink_led_stop_generic");


	autoc_reg &= ~IXGBE_AUTOC_FLU;
	autoc_reg |= IXGBE_AUTOC_AN_RESTART;
	IXGBE_WRITE_REG(hw, IXGBE_AUTOC, autoc_reg);

	led_reg &= ~IXGBE_LED_MODE_MASK(index);
	led_reg &= ~IXGBE_LED_BLINK(index);
	led_reg |= IXGBE_LED_LINK_ACTIVE << IXGBE_LED_MODE_SHIFT(index);
	IXGBE_WRITE_REG(hw, IXGBE_LEDCTL, led_reg);
	IXGBE_WRITE_FLUSH(hw);

	return IXGBE_SUCCESS;
}

/**
 *  ixgbe_get_san_mac_addr_offset - Get SAN MAC address offset from the EEPROM
 *  @hw: pointer to hardware structure
 *  @san_mac_offset: SAN MAC address offset
 *
 *  This function will read the EEPROM location for the SAN MAC address
 *  pointer, and returns the value at that location.  This is used in both
 *  get and set mac_addr routines.
 **/
static s32 ixgbe_get_san_mac_addr_offset(struct ixgbe_hw *hw,
                                        u16 *san_mac_offset)
{
	DEBUGFUNC("ixgbe_get_san_mac_addr_offset");

	/*
	 * First read the EEPROM pointer to see if the MAC addresses are
	 * available.
	 */
	hw->eeprom.ops.read(hw, IXGBE_SAN_MAC_ADDR_PTR, san_mac_offset);

	return IXGBE_SUCCESS;
}

/**
 *  ixgbe_get_san_mac_addr_generic - SAN MAC address retrieval from the EEPROM
 *  @hw: pointer to hardware structure
 *  @san_mac_addr: SAN MAC address
 *
 *  Reads the SAN MAC address from the EEPROM, if it's available.  This is
 *  per-port, so set_lan_id() must be called before reading the addresses.
 *  set_lan_id() is called by identify_sfp(), but this cannot be relied
 *  upon for non-SFP connections, so we must call it here.
 **/
s32 ixgbe_get_san_mac_addr_generic(struct ixgbe_hw *hw, u8 *san_mac_addr)
{
	u16 san_mac_data, san_mac_offset;
	u8 i;

	DEBUGFUNC("ixgbe_get_san_mac_addr_generic");

	/*
	 * First read the EEPROM pointer to see if the MAC addresses are
	 * available.  If they're not, no point in calling set_lan_id() here.
	 */
	ixgbe_get_san_mac_addr_offset(hw, &san_mac_offset);

	if ((san_mac_offset == 0) || (san_mac_offset == 0xFFFF)) {
		/*
		 * No addresses available in this EEPROM.  It's not an
		 * error though, so just wipe the local address and return.
		 */
		for (i = 0; i < 6; i++)
			san_mac_addr[i] = 0xFF;

		goto san_mac_addr_out;
	}

	/* make sure we know which port we need to program */
	hw->mac.ops.set_lan_id(hw);
	/* apply the port offset to the address offset */
	(hw->bus.func) ? (san_mac_offset += IXGBE_SAN_MAC_ADDR_PORT1_OFFSET) :
	                 (san_mac_offset += IXGBE_SAN_MAC_ADDR_PORT0_OFFSET);
	for (i = 0; i < 3; i++) {
		hw->eeprom.ops.read(hw, san_mac_offset, &san_mac_data);
		san_mac_addr[i * 2] = (u8)(san_mac_data);
		san_mac_addr[i * 2 + 1] = (u8)(san_mac_data >> 8);
		san_mac_offset++;
	}

san_mac_addr_out:
	return IXGBE_SUCCESS;
}

/**
 *  ixgbe_set_san_mac_addr_generic - Write the SAN MAC address to the EEPROM
 *  @hw: pointer to hardware structure
 *  @san_mac_addr: SAN MAC address
 *
 *  Write a SAN MAC address to the EEPROM.
 **/
s32 ixgbe_set_san_mac_addr_generic(struct ixgbe_hw *hw, u8 *san_mac_addr)
{
	s32 status = IXGBE_SUCCESS;
	u16 san_mac_data, san_mac_offset;
	u8 i;

	DEBUGFUNC("ixgbe_set_san_mac_addr_generic");

	/* Look for SAN mac address pointer.  If not defined, return */
	ixgbe_get_san_mac_addr_offset(hw, &san_mac_offset);

	if ((san_mac_offset == 0) || (san_mac_offset == 0xFFFF)) {
		status = IXGBE_ERR_NO_SAN_ADDR_PTR;
		goto san_mac_addr_out;
	}

	/* Make sure we know which port we need to write */
	hw->mac.ops.set_lan_id(hw);
	/* Apply the port offset to the address offset */
	(hw->bus.func) ? (san_mac_offset += IXGBE_SAN_MAC_ADDR_PORT1_OFFSET) :
	                 (san_mac_offset += IXGBE_SAN_MAC_ADDR_PORT0_OFFSET);

	for (i = 0; i < 3; i++) {
		san_mac_data = (u16)((u16)(san_mac_addr[i * 2 + 1]) << 8);
		san_mac_data |= (u16)(san_mac_addr[i * 2]);
		hw->eeprom.ops.write(hw, san_mac_offset, san_mac_data);
		san_mac_offset++;
	}

san_mac_addr_out:
	return status;
}

/**
 *  ixgbe_get_pcie_msix_count_generic - Gets MSI-X vector count
 *  @hw: pointer to hardware structure
 *
 *  Read PCIe configuration space, and get the MSI-X vector count from
 *  the capabilities table.
 **/
u32 ixgbe_get_pcie_msix_count_generic(struct ixgbe_hw *hw)
{
	u32 msix_count = 64;

	DEBUGFUNC("ixgbe_get_pcie_msix_count_generic");
	if (hw->mac.msix_vectors_from_pcie) {
		msix_count = IXGBE_READ_PCIE_WORD(hw,
		                                  IXGBE_PCIE_MSIX_82599_CAPS);
		msix_count &= IXGBE_PCIE_MSIX_TBL_SZ_MASK;

		/* MSI-X count is zero-based in HW, so increment to give
		 * proper value */
		msix_count++;
	}

	return msix_count;
}

/**
 *  ixgbe_insert_mac_addr_generic - Find a RAR for this mac address
 *  @hw: pointer to hardware structure
 *  @addr: Address to put into receive address register
 *  @vmdq: VMDq pool to assign
 *
 *  Puts an ethernet address into a receive address register, or
 *  finds the rar that it is aleady in; adds to the pool list
 **/
s32 ixgbe_insert_mac_addr_generic(struct ixgbe_hw *hw, u8 *addr, u32 vmdq)
{
	static const u32 NO_EMPTY_RAR_FOUND = 0xFFFFFFFF;
	u32 first_empty_rar = NO_EMPTY_RAR_FOUND;
	u32 rar;
	u32 rar_low, rar_high;
	u32 addr_low, addr_high;

	DEBUGFUNC("ixgbe_insert_mac_addr_generic");

	/* swap bytes for HW little endian */
	addr_low  = addr[0] | (addr[1] << 8)
			    | (addr[2] << 16)
			    | (addr[3] << 24);
	addr_high = addr[4] | (addr[5] << 8);

	/*
	 * Either find the mac_id in rar or find the first empty space.
	 * rar_highwater points to just after the highest currently used
	 * rar in order to shorten the search.  It grows when we add a new
	 * rar to the top.
	 */
	for (rar = 0; rar < hw->mac.rar_highwater; rar++) {
		rar_high = IXGBE_READ_REG(hw, IXGBE_RAH(rar));

		if (((IXGBE_RAH_AV & rar_high) == 0)
		    && first_empty_rar == NO_EMPTY_RAR_FOUND) {
			first_empty_rar = rar;
		} else if ((rar_high & 0xFFFF) == addr_high) {
			rar_low = IXGBE_READ_REG(hw, IXGBE_RAL(rar));
			if (rar_low == addr_low)
				break;    /* found it already in the rars */
		}
	}

	if (rar < hw->mac.rar_highwater) {
		/* already there so just add to the pool bits */
		ixgbe_set_vmdq(hw, rar, vmdq);
	} else if (first_empty_rar != NO_EMPTY_RAR_FOUND) {
		/* stick it into first empty RAR slot we found */
		rar = first_empty_rar;
		ixgbe_set_rar(hw, rar, addr, vmdq, IXGBE_RAH_AV);
	} else if (rar == hw->mac.rar_highwater) {
		/* add it to the top of the list and inc the highwater mark */
		ixgbe_set_rar(hw, rar, addr, vmdq, IXGBE_RAH_AV);
		hw->mac.rar_highwater++;
	} else if (rar >= hw->mac.num_rar_entries) {
		return IXGBE_ERR_INVALID_MAC_ADDR;
	}

	/*
	 * If we found rar[0], make sure the default pool bit (we use pool 0)
	 * remains cleared to be sure default pool packets will get delivered
	 */
	if (rar == 0)
		ixgbe_clear_vmdq(hw, rar, 0);

	return rar;
}

/**
 *  ixgbe_clear_vmdq_generic - Disassociate a VMDq pool index from a rx address
 *  @hw: pointer to hardware struct
 *  @rar: receive address register index to disassociate
 *  @vmdq: VMDq pool index to remove from the rar
 **/
s32 ixgbe_clear_vmdq_generic(struct ixgbe_hw *hw, u32 rar, u32 vmdq)
{
	u32 mpsar_lo, mpsar_hi;
	u32 rar_entries = hw->mac.num_rar_entries;

	DEBUGFUNC("ixgbe_clear_vmdq_generic");

	/* Make sure we are using a valid rar index range */
	if (rar >= rar_entries) {
		DEBUGOUT1("RAR index %d is out of range.\n", rar);
		return IXGBE_ERR_INVALID_ARGUMENT;
	}

	mpsar_lo = IXGBE_READ_REG(hw, IXGBE_MPSAR_LO(rar));
	mpsar_hi = IXGBE_READ_REG(hw, IXGBE_MPSAR_HI(rar));

	if (!mpsar_lo && !mpsar_hi)
		goto done;

	if (vmdq == IXGBE_CLEAR_VMDQ_ALL) {
		if (mpsar_lo) {
			IXGBE_WRITE_REG(hw, IXGBE_MPSAR_LO(rar), 0);
			mpsar_lo = 0;
		}
		if (mpsar_hi) {
			IXGBE_WRITE_REG(hw, IXGBE_MPSAR_HI(rar), 0);
			mpsar_hi = 0;
		}
	} else if (vmdq < 32) {
		mpsar_lo &= ~(1 << vmdq);
		IXGBE_WRITE_REG(hw, IXGBE_MPSAR_LO(rar), mpsar_lo);
	} else {
		mpsar_hi &= ~(1 << (vmdq - 32));
		IXGBE_WRITE_REG(hw, IXGBE_MPSAR_HI(rar), mpsar_hi);
	}

	/* was that the last pool using this rar? */
	if (mpsar_lo == 0 && mpsar_hi == 0 && rar != 0)
		hw->mac.ops.clear_rar(hw, rar);
done:
	return IXGBE_SUCCESS;
}

/**
 *  ixgbe_set_vmdq_generic - Associate a VMDq pool index with a rx address
 *  @hw: pointer to hardware struct
 *  @rar: receive address register index to associate with a VMDq index
 *  @vmdq: VMDq pool index
 **/
s32 ixgbe_set_vmdq_generic(struct ixgbe_hw *hw, u32 rar, u32 vmdq)
{
	u32 mpsar;
	u32 rar_entries = hw->mac.num_rar_entries;

	DEBUGFUNC("ixgbe_set_vmdq_generic");

	/* Make sure we are using a valid rar index range */
	if (rar >= rar_entries) {
		DEBUGOUT1("RAR index %d is out of range.\n", rar);
		return IXGBE_ERR_INVALID_ARGUMENT;
	}

	if (vmdq < 32) {
		mpsar = IXGBE_READ_REG(hw, IXGBE_MPSAR_LO(rar));
		mpsar |= 1 << vmdq;
		IXGBE_WRITE_REG(hw, IXGBE_MPSAR_LO(rar), mpsar);
	} else {
		mpsar = IXGBE_READ_REG(hw, IXGBE_MPSAR_HI(rar));
		mpsar |= 1 << (vmdq - 32);
		IXGBE_WRITE_REG(hw, IXGBE_MPSAR_HI(rar), mpsar);
	}
	return IXGBE_SUCCESS;
}

/**
 *  ixgbe_init_uta_tables_generic - Initialize the Unicast Table Array
 *  @hw: pointer to hardware structure
 **/
s32 ixgbe_init_uta_tables_generic(struct ixgbe_hw *hw)
{
	int i;

	DEBUGFUNC("ixgbe_init_uta_tables_generic");
	DEBUGOUT(" Clearing UTA\n");

	for (i = 0; i < 128; i++)
		IXGBE_WRITE_REG(hw, IXGBE_UTA(i), 0);

	return IXGBE_SUCCESS;
}

/**
 *  ixgbe_find_vlvf_slot - find the vlanid or the first empty slot
 *  @hw: pointer to hardware structure
 *  @vlan: VLAN id to write to VLAN filter
 *
 *  return the VLVF index where this VLAN id should be placed
 *
 **/
s32 ixgbe_find_vlvf_slot(struct ixgbe_hw *hw, u32 vlan)
{
	u32 bits = 0;
	u32 first_empty_slot = 0;
	s32 regindex;

	/* short cut the special case */
	if (vlan == 0)
		return 0;

	/*
	  * Search for the vlan id in the VLVF entries. Save off the first empty
	  * slot found along the way
	  */
	for (regindex = 1; regindex < IXGBE_VLVF_ENTRIES; regindex++) {
		bits = IXGBE_READ_REG(hw, IXGBE_VLVF(regindex));
		if (!bits && !(first_empty_slot))
			first_empty_slot = regindex;
		else if ((bits & 0x0FFF) == vlan)
			break;
	}

	/*
	  * If regindex is less than IXGBE_VLVF_ENTRIES, then we found the vlan
	  * in the VLVF. Else use the first empty VLVF register for this
	  * vlan id.
	  */
	if (regindex >= IXGBE_VLVF_ENTRIES) {
		if (first_empty_slot)
			regindex = first_empty_slot;
		else {
			DEBUGOUT("No space in VLVF.\n");
			regindex = IXGBE_ERR_NO_SPACE;
		}
	}

	return regindex;
}

/**
 *  ixgbe_set_vfta_generic - Set VLAN filter table
 *  @hw: pointer to hardware structure
 *  @vlan: VLAN id to write to VLAN filter
 *  @vind: VMDq output index that maps queue to VLAN id in VFVFB
 *  @vlan_on: boolean flag to turn on/off VLAN in VFVF
 *
 *  Turn on/off specified VLAN in the VLAN filter table.
 **/
s32 ixgbe_set_vfta_generic(struct ixgbe_hw *hw, u32 vlan, u32 vind,
                           bool vlan_on)
{
	s32 regindex;
	u32 bitindex;
	u32 vfta;
	u32 bits;
	u32 vt;
	u32 targetbit;
	bool vfta_changed = FALSE;

	DEBUGFUNC("ixgbe_set_vfta_generic");

	if (vlan > 4095)
		return IXGBE_ERR_PARAM;

	/*
	 * this is a 2 part operation - first the VFTA, then the
	 * VLVF and VLVFB if VT Mode is set
	 * We don't write the VFTA until we know the VLVF part succeeded.
	 */

	/* Part 1
	 * The VFTA is a bitstring made up of 128 32-bit registers
	 * that enable the particular VLAN id, much like the MTA:
	 *    bits[11-5]: which register
	 *    bits[4-0]:  which bit in the register
	 */
	regindex = (vlan >> 5) & 0x7F;
	bitindex = vlan & 0x1F;
	targetbit = (1 << bitindex);
	vfta = IXGBE_READ_REG(hw, IXGBE_VFTA(regindex));

	if (vlan_on) {
		if (!(vfta & targetbit)) {
			vfta |= targetbit;
			vfta_changed = TRUE;
		}
	} else {
		if ((vfta & targetbit)) {
			vfta &= ~targetbit;
			vfta_changed = TRUE;
		}
	}

	/* Part 2
	 * If VT Mode is set
	 *   Either vlan_on
	 *     make sure the vlan is in VLVF
	 *     set the vind bit in the matching VLVFB
	 *   Or !vlan_on
	 *     clear the pool bit and possibly the vind
	 */
	vt = IXGBE_READ_REG(hw, IXGBE_VT_CTL);
	if (vt & IXGBE_VT_CTL_VT_ENABLE) {
		s32 vlvf_index;

		vlvf_index = ixgbe_find_vlvf_slot(hw, vlan);
		if (vlvf_index < 0)
			return vlvf_index;

		if (vlan_on) {
			/* set the pool bit */
			if (vind < 32) {
				bits = IXGBE_READ_REG(hw,
						IXGBE_VLVFB(vlvf_index*2));
				bits |= (1 << vind);
				IXGBE_WRITE_REG(hw,
						IXGBE_VLVFB(vlvf_index*2),
						bits);
			} else {
				bits = IXGBE_READ_REG(hw,
						IXGBE_VLVFB((vlvf_index*2)+1));
				bits |= (1 << (vind-32));
				IXGBE_WRITE_REG(hw,
						IXGBE_VLVFB((vlvf_index*2)+1),
						bits);
			}
		} else {
			/* clear the pool bit */
			if (vind < 32) {
				bits = IXGBE_READ_REG(hw,
						IXGBE_VLVFB(vlvf_index*2));
				bits &= ~(1 << vind);
				IXGBE_WRITE_REG(hw,
						IXGBE_VLVFB(vlvf_index*2),
						bits);
				bits |= IXGBE_READ_REG(hw,
						IXGBE_VLVFB((vlvf_index*2)+1));
			} else {
				bits = IXGBE_READ_REG(hw,
						IXGBE_VLVFB((vlvf_index*2)+1));
				bits &= ~(1 << (vind-32));
				IXGBE_WRITE_REG(hw,
						IXGBE_VLVFB((vlvf_index*2)+1),
						bits);
				bits |= IXGBE_READ_REG(hw,
						IXGBE_VLVFB(vlvf_index*2));
			}
		}

		/*
		 * If there are still bits set in the VLVFB registers
		 * for the VLAN ID indicated we need to see if the
		 * caller is requesting that we clear the VFTA entry bit.
		 * If the caller has requested that we clear the VFTA
		 * entry bit but there are still pools/VFs using this VLAN
		 * ID entry then ignore the request.  We're not worried
		 * about the case where we're turning the VFTA VLAN ID
		 * entry bit on, only when requested to turn it off as
		 * there may be multiple pools and/or VFs using the
		 * VLAN ID entry.  In that case we cannot clear the
		 * VFTA bit until all pools/VFs using that VLAN ID have also
		 * been cleared.  This will be indicated by "bits" being
		 * zero.
		 */
		if (bits) {
			IXGBE_WRITE_REG(hw, IXGBE_VLVF(vlvf_index),
					(IXGBE_VLVF_VIEN | vlan));
			if (!vlan_on) {
				/* someone wants to clear the vfta entry
				 * but some pools/VFs are still using it.
				 * Ignore it. */
				vfta_changed = FALSE;
			}
		}
		else
			IXGBE_WRITE_REG(hw, IXGBE_VLVF(vlvf_index), 0);
	}

	if (vfta_changed)
		IXGBE_WRITE_REG(hw, IXGBE_VFTA(regindex), vfta);

	return IXGBE_SUCCESS;
}

/**
 *  ixgbe_clear_vfta_generic - Clear VLAN filter table
 *  @hw: pointer to hardware structure
 *
 *  Clears the VLAN filer table, and the VMDq index associated with the filter
 **/
s32 ixgbe_clear_vfta_generic(struct ixgbe_hw *hw)
{
	u32 offset;

	DEBUGFUNC("ixgbe_clear_vfta_generic");

	for (offset = 0; offset < hw->mac.vft_size; offset++)
		IXGBE_WRITE_REG(hw, IXGBE_VFTA(offset), 0);

	for (offset = 0; offset < IXGBE_VLVF_ENTRIES; offset++) {
		IXGBE_WRITE_REG(hw, IXGBE_VLVF(offset), 0);
		IXGBE_WRITE_REG(hw, IXGBE_VLVFB(offset*2), 0);
		IXGBE_WRITE_REG(hw, IXGBE_VLVFB((offset*2)+1), 0);
	}

	return IXGBE_SUCCESS;
}

/**
 *  ixgbe_check_mac_link_generic - Determine link and speed status
 *  @hw: pointer to hardware structure
 *  @speed: pointer to link speed
 *  @link_up: TRUE when link is up
 *  @link_up_wait_to_complete: bool used to wait for link up or not
 *
 *  Reads the links register to determine if link is up and the current speed
 **/
s32 ixgbe_check_mac_link_generic(struct ixgbe_hw *hw, ixgbe_link_speed *speed,
                                 bool *link_up, bool link_up_wait_to_complete)
{
	u32 links_reg, links_orig;
	u32 i;

	DEBUGFUNC("ixgbe_check_mac_link_generic");

	/* clear the old state */
	links_orig = IXGBE_READ_REG(hw, IXGBE_LINKS);

	links_reg = IXGBE_READ_REG(hw, IXGBE_LINKS);

	if (links_orig != links_reg) {
		DEBUGOUT2("LINKS changed from %08X to %08X\n",
		          links_orig, links_reg);
	}

	if (link_up_wait_to_complete) {
		for (i = 0; i < IXGBE_LINK_UP_TIME; i++) {
			if (links_reg & IXGBE_LINKS_UP) {
				*link_up = TRUE;
				break;
			} else {
				*link_up = FALSE;
			}
			msec_delay(100);
			links_reg = IXGBE_READ_REG(hw, IXGBE_LINKS);
		}
	} else {
		if (links_reg & IXGBE_LINKS_UP)
			*link_up = TRUE;
		else
			*link_up = FALSE;
	}

	if ((links_reg & IXGBE_LINKS_SPEED_82599) ==
	    IXGBE_LINKS_SPEED_10G_82599)
		*speed = IXGBE_LINK_SPEED_10GB_FULL;
	else if ((links_reg & IXGBE_LINKS_SPEED_82599) ==
	         IXGBE_LINKS_SPEED_1G_82599)
		*speed = IXGBE_LINK_SPEED_1GB_FULL;
	else if ((links_reg & IXGBE_LINKS_SPEED_82599) ==
	         IXGBE_LINKS_SPEED_100_82599)
		*speed = IXGBE_LINK_SPEED_100_FULL;
	else
		*speed = IXGBE_LINK_SPEED_UNKNOWN;

	/* if link is down, zero out the current_mode */
	if (*link_up == FALSE) {
		hw->fc.current_mode = ixgbe_fc_none;
		hw->fc.fc_was_autonegged = FALSE;
	}

	return IXGBE_SUCCESS;
}

/**
 *  ixgbe_get_wwn_prefix_generic - Get alternative WWNN/WWPN prefix from
 *  the EEPROM
 *  @hw: pointer to hardware structure
 *  @wwnn_prefix: the alternative WWNN prefix
 *  @wwpn_prefix: the alternative WWPN prefix
 *
 *  This function will read the EEPROM from the alternative SAN MAC address
 *  block to check the support for the alternative WWNN/WWPN prefix support.
 **/
s32 ixgbe_get_wwn_prefix_generic(struct ixgbe_hw *hw, u16 *wwnn_prefix,
                                 u16 *wwpn_prefix)
{
	u16 offset, caps;
	u16 alt_san_mac_blk_offset;

	DEBUGFUNC("ixgbe_get_wwn_prefix_generic");

	/* clear output first */
	*wwnn_prefix = 0xFFFF;
	*wwpn_prefix = 0xFFFF;

	/* check if alternative SAN MAC is supported */
	hw->eeprom.ops.read(hw, IXGBE_ALT_SAN_MAC_ADDR_BLK_PTR,
	                    &alt_san_mac_blk_offset);

	if ((alt_san_mac_blk_offset == 0) ||
	    (alt_san_mac_blk_offset == 0xFFFF))
		goto wwn_prefix_out;

	/* check capability in alternative san mac address block */
	offset = alt_san_mac_blk_offset + IXGBE_ALT_SAN_MAC_ADDR_CAPS_OFFSET;
	hw->eeprom.ops.read(hw, offset, &caps);
	if (!(caps & IXGBE_ALT_SAN_MAC_ADDR_CAPS_ALTWWN))
		goto wwn_prefix_out;

	/* get the corresponding prefix for WWNN/WWPN */
	offset = alt_san_mac_blk_offset + IXGBE_ALT_SAN_MAC_ADDR_WWNN_OFFSET;
	hw->eeprom.ops.read(hw, offset, wwnn_prefix);

	offset = alt_san_mac_blk_offset + IXGBE_ALT_SAN_MAC_ADDR_WWPN_OFFSET;
	hw->eeprom.ops.read(hw, offset, wwpn_prefix);

wwn_prefix_out:
	return IXGBE_SUCCESS;
}

/**
 *  ixgbe_get_fcoe_boot_status_generic - Get FCOE boot status from EEPROM
 *  @hw: pointer to hardware structure
 *  @bs: the fcoe boot status
 *
 *  This function will read the FCOE boot status from the iSCSI FCOE block
 **/
s32 ixgbe_get_fcoe_boot_status_generic(struct ixgbe_hw *hw, u16 *bs)
{
	u16 offset, caps, flags;
	s32 status;

	DEBUGFUNC("ixgbe_get_fcoe_boot_status_generic");

	/* clear output first */
	*bs = ixgbe_fcoe_bootstatus_unavailable;

	/* check if FCOE IBA block is present */
	offset = IXGBE_FCOE_IBA_CAPS_BLK_PTR;
	status = hw->eeprom.ops.read(hw, offset, &caps);
	if (status != IXGBE_SUCCESS)
		goto out;

	if (!(caps & IXGBE_FCOE_IBA_CAPS_FCOE))
		goto out;

	/* check if iSCSI FCOE block is populated */
	status = hw->eeprom.ops.read(hw, IXGBE_ISCSI_FCOE_BLK_PTR, &offset);
	if (status != IXGBE_SUCCESS)
		goto out;

	if ((offset == 0) || (offset == 0xFFFF))
		goto out;

	/* read fcoe flags in iSCSI FCOE block */
	offset = offset + IXGBE_ISCSI_FCOE_FLAGS_OFFSET;
	status = hw->eeprom.ops.read(hw, offset, &flags);
	if (status != IXGBE_SUCCESS)
		goto out;

	if (flags & IXGBE_ISCSI_FCOE_FLAGS_ENABLE)
		*bs = ixgbe_fcoe_bootstatus_enabled;
	else
		*bs = ixgbe_fcoe_bootstatus_disabled;

out:
	return status;
}

/**
 *  ixgbe_device_supports_autoneg_fc - Check if phy supports autoneg flow
 *  control
 *  @hw: pointer to hardware structure
 *
 *  There are several phys that do not support autoneg flow control. This
 *  function check the device id to see if the associated phy supports
 *  autoneg flow control.
 **/
static s32 ixgbe_device_supports_autoneg_fc(struct ixgbe_hw *hw)
{

	DEBUGFUNC("ixgbe_device_supports_autoneg_fc");

	switch (hw->device_id) {
	case IXGBE_DEV_ID_82599_T3_LOM:
		return IXGBE_SUCCESS;
	default:
		return IXGBE_ERR_FC_NOT_SUPPORTED;
	}
}

/**
 *  ixgbe_set_mac_anti_spoofing - Enable/Disable MAC anti-spoofing
 *  @hw: pointer to hardware structure
 *  @enable: enable or disable switch for anti-spoofing
 *  @pf: Physical Function pool - do not enable anti-spoofing for the PF
 *
 **/
void ixgbe_set_mac_anti_spoofing(struct ixgbe_hw *hw, bool enable, int pf)
{
	int j;
	int pf_target_reg = pf >> 3;
	int pf_target_shift = pf % 8;
	u32 pfvfspoof = 0;

	if (hw->mac.type == ixgbe_mac_82598EB)
		return;

	if (enable)
		pfvfspoof = IXGBE_SPOOF_MACAS_MASK;

	/*
	 * PFVFSPOOF register array is size 8 with 8 bits assigned to
	 * MAC anti-spoof enables in each register array element.
	 */
	for (j = 0; j < IXGBE_PFVFSPOOF_REG_COUNT; j++)
		IXGBE_WRITE_REG(hw, IXGBE_PFVFSPOOF(j), pfvfspoof);

	/* If not enabling anti-spoofing then done */
	if (!enable)
		return;

	/*
	 * The PF should be allowed to spoof so that it can support
	 * emulation mode NICs.  Reset the bit assigned to the PF
	 */
	pfvfspoof = IXGBE_READ_REG(hw, IXGBE_PFVFSPOOF(pf_target_reg));
	pfvfspoof ^= (1 << pf_target_shift);
	IXGBE_WRITE_REG(hw, IXGBE_PFVFSPOOF(pf_target_reg), pfvfspoof);
}

/**
 *  ixgbe_set_vlan_anti_spoofing - Enable/Disable VLAN anti-spoofing
 *  @hw: pointer to hardware structure
 *  @enable: enable or disable switch for VLAN anti-spoofing
 *  @pf: Virtual Function pool - VF Pool to set for VLAN anti-spoofing
 *
 **/
void ixgbe_set_vlan_anti_spoofing(struct ixgbe_hw *hw, bool enable, int vf)
{
	int vf_target_reg = vf >> 3;
	int vf_target_shift = vf % 8 + IXGBE_SPOOF_VLANAS_SHIFT;
	u32 pfvfspoof;

	if (hw->mac.type == ixgbe_mac_82598EB)
		return;

	pfvfspoof = IXGBE_READ_REG(hw, IXGBE_PFVFSPOOF(vf_target_reg));
	if (enable)
		pfvfspoof |= (1 << vf_target_shift);
	else
		pfvfspoof &= ~(1 << vf_target_shift);
	IXGBE_WRITE_REG(hw, IXGBE_PFVFSPOOF(vf_target_reg), pfvfspoof);
}

/**
 *  ixgbe_get_device_caps_generic - Get additional device capabilities
 *  @hw: pointer to hardware structure
 *  @device_caps: the EEPROM word with the extra device capabilities
 *
 *  This function will read the EEPROM location for the device capabilities,
 *  and return the word through device_caps.
 **/
s32 ixgbe_get_device_caps_generic(struct ixgbe_hw *hw, u16 *device_caps)
{
	DEBUGFUNC("ixgbe_get_device_caps_generic");

	hw->eeprom.ops.read(hw, IXGBE_DEVICE_CAPS, device_caps);

	return IXGBE_SUCCESS;
}

/**
 *  ixgbe_enable_relaxed_ordering_gen2 - Enable relaxed ordering
 *  @hw: pointer to hardware structure
 *
 **/
void ixgbe_enable_relaxed_ordering_gen2(struct ixgbe_hw *hw)
{
	u32 regval;
	u32 i;

	DEBUGFUNC("ixgbe_enable_relaxed_ordering_gen2");

	/* Enable relaxed ordering */
	for (i = 0; i < hw->mac.max_tx_queues; i++) {
		regval = IXGBE_READ_REG(hw, IXGBE_DCA_TXCTRL_82599(i));
		regval |= IXGBE_DCA_TXCTRL_TX_WB_RO_EN;
		IXGBE_WRITE_REG(hw, IXGBE_DCA_TXCTRL_82599(i), regval);
	}

	for (i = 0; i < hw->mac.max_rx_queues; i++) {
		regval = IXGBE_READ_REG(hw, IXGBE_DCA_RXCTRL(i));
		regval |= (IXGBE_DCA_RXCTRL_DESC_WRO_EN |
		           IXGBE_DCA_RXCTRL_DESC_HSRO_EN);
		IXGBE_WRITE_REG(hw, IXGBE_DCA_RXCTRL(i), regval);
	}

}
