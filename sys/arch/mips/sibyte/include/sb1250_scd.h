/*  *********************************************************************
    *  SB1250 Board Support Package
    *
    *  SCD Constants and Macros			File: sb1250_scd.h
    *
    *  This module contains constants and macros useful for
    *  manipulating the System Control and Debug module on the 1250.
    *
    *  SB1250 specification level:  0.2  plus errata as of 11/7/2000
    *
    *  Author:  Mitch Lichtenberg (mitch@sibyte.com)
    *
    *********************************************************************
    *
    *  Copyright 2000,2001
    *  Broadcom Corporation. All rights reserved.
    *
    *  This software is furnished under license and may be used and
    *  copied only in accordance with the following terms and
    *  conditions.  Subject to these conditions, you may download,
    *  copy, install, use, modify and distribute modified or unmodified
    *  copies of this software in source and/or binary form.  No title
    *  or ownership is transferred hereby.
    *
    *  1) Any source code used, modified or distributed must reproduce
    *     and retain this copyright notice and list of conditions as
    *     they appear in the source file.
    *
    *  2) No right is granted to use any trade name, trademark, or
    *     logo of Broadcom Corporation. Neither the "Broadcom
    *     Corporation" name nor any trademark or logo of Broadcom
    *     Corporation may be used to endorse or promote products
    *     derived from this software without the prior written
    *     permission of Broadcom Corporation.
    *
    *  3) THIS SOFTWARE IS PROVIDED "AS-IS" AND ANY EXPRESS OR
    *     IMPLIED WARRANTIES, INCLUDING BUT NOT LIMITED TO, ANY IMPLIED
    *     WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
    *     PURPOSE, OR NON-INFRINGEMENT ARE DISCLAIMED. IN NO EVENT
    *     SHALL BROADCOM BE LIABLE FOR ANY DAMAGES WHATSOEVER, AND IN
    *     PARTICULAR, BROADCOM SHALL NOT BE LIABLE FOR DIRECT, INDIRECT,
    *     INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    *     (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
    *     GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
    *     BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
    *     OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
    *     TORT (INCLUDING NEGLIGENCE OR OTHERWISE), EVEN IF ADVISED OF
    *     THE POSSIBILITY OF SUCH DAMAGE.
    ********************************************************************* */

#ifndef _SB1250_SCD_H
#define	_SB1250_SCD_H

#include "sb1250_defs.h"

/*  *********************************************************************
    *  System control/debug registers
    ********************************************************************* */

/*
 * System Revision Register (Table 4-1)
 */

#define	M_SYS_RESERVED		    _SB_MAKEMASK(8,0)

#define	S_SYS_REVISION              _SB_MAKE64(8)
#define	M_SYS_REVISION              _SB_MAKEMASK(8,S_SYS_REVISION)
#define	V_SYS_REVISION(x)           _SB_MAKEVALUE(x,S_SYS_REVISION)
#define	G_SYS_REVISION(x)           _SB_GETVALUE(x,S_SYS_REVISION,M_SYS_REVISION)

#define	K_SYS_REVISION_PASS1	    1
#define	K_SYS_REVISION_PASS2	    3
#define	K_SYS_REVISION_PASS3	    4 /* XXX Unknown */

#define	S_SYS_PART                  _SB_MAKE64(16)
#define	M_SYS_PART                  _SB_MAKEMASK(16,S_SYS_PART)
#define	V_SYS_PART(x)               _SB_MAKEVALUE(x,S_SYS_PART)
#define	G_SYS_PART(x)               _SB_GETVALUE(x,S_SYS_PART,M_SYS_PART)

#define	K_SYS_PART_SB1250           0x1250
#define	K_SYS_PART_SB1125           0x1125

#define	S_SYS_WID                   _SB_MAKE64(32)
#define	M_SYS_WID                   _SB_MAKEMASK(32,S_SYS_WID)
#define	V_SYS_WID(x)                _SB_MAKEVALUE(x,S_SYS_WID)
#define	G_SYS_WID(x)                _SB_GETVALUE(x,S_SYS_WID,M_SYS_WID)

/*
 * System Config Register (Table 4-2)
 * Register: SCD_SYSTEM_CFG
 */

#define	M_SYS_LDT_PLL_BYP           _SB_MAKEMASK1(3)
#define	M_SYS_PCI_SYNC_TEST_MODE    _SB_MAKEMASK1(4)
#define	M_SYS_IOB0_DIV              _SB_MAKEMASK1(5)
#define	M_SYS_IOB1_DIV              _SB_MAKEMASK1(6)

#define	S_SYS_PLL_DIV               _SB_MAKE64(7)
#define	M_SYS_PLL_DIV               _SB_MAKEMASK(5,S_SYS_PLL_DIV)
#define	V_SYS_PLL_DIV(x)            _SB_MAKEVALUE(x,S_SYS_PLL_DIV)
#define	G_SYS_PLL_DIV(x)            _SB_GETVALUE(x,S_SYS_PLL_DIV,M_SYS_PLL_DIV)

#define	M_SYS_SER0_ENABLE           _SB_MAKEMASK1(12)
#define	M_SYS_SER0_RSTB_EN          _SB_MAKEMASK1(13)
#define	M_SYS_SER1_ENABLE           _SB_MAKEMASK1(14)
#define	M_SYS_SER1_RSTB_EN          _SB_MAKEMASK1(15)
#define	M_SYS_PCMCIA_ENABLE         _SB_MAKEMASK1(16)

#define	S_SYS_BOOT_MODE             _SB_MAKE64(17)
#define	M_SYS_BOOT_MODE             _SB_MAKEMASK(2,S_SYS_BOOT_MODE)
#define	V_SYS_BOOT_MODE(x)          _SB_MAKEVALUE(x,S_SYS_BOOT_MODE)
#define	G_SYS_BOOT_MODE(x)          _SB_GETVALUE(x,S_SYS_BOOT_MODE,M_SYS_BOOT_MODE)
#define	K_SYS_BOOT_MODE_ROM32       0
#define	K_SYS_BOOT_MODE_ROM8        1
#define	K_SYS_BOOT_MODE_SMBUS_SMALL 2
#define	K_SYS_BOOT_MODE_SMBUS_BIG   3

#define	M_SYS_PCI_HOST              _SB_MAKEMASK1(19)
#define	M_SYS_PCI_ARBITER           _SB_MAKEMASK1(20)
#define	M_SYS_SOUTH_ON_LDT          _SB_MAKEMASK1(21)
#define	M_SYS_BIG_ENDIAN            _SB_MAKEMASK1(22)
#define	M_SYS_GENCLK_EN             _SB_MAKEMASK1(23)
#define	M_SYS_LDT_TEST_EN           _SB_MAKEMASK1(24)
#define	M_SYS_GEN_PARITY_EN         _SB_MAKEMASK1(25)

#define	S_SYS_CONFIG                26
#define	M_SYS_CONFIG                _SB_MAKEMASK(6,S_SYS_CONFIG)
#define	V_SYS_CONFIG(x)             _SB_MAKEVALUE(x,S_SYS_CONFIG)
#define	G_SYS_CONFIG(x)             _SB_GETVALUE(x,S_SYS_CONFIG,M_SYS_CONFIG)

/* The following bits are writeable by JTAG only. */

#define	M_SYS_CLKSTOP               _SB_MAKEMASK1(32)
#define	M_SYS_CLKSTEP               _SB_MAKEMASK1(33)

#define	S_SYS_CLKCOUNT              34
#define	M_SYS_CLKCOUNT              _SB_MAKEMASK(8,S_SYS_CLKCOUNT)
#define	V_SYS_CLKCOUNT(x)           _SB_MAKEVALUE(x,S_SYS_CLKCOUNT)
#define	G_SYS_CLKCOUNT(x)           _SB_GETVALUE(x,S_SYS_CLKCOUNT,M_SYS_CLKCOUNT)

#define	M_SYS_PLL_BYPASS            _SB_MAKEMASK1(42)

#define	S_SYS_PLL_IREF		    43
#define	M_SYS_PLL_IREF		    _SB_MAKEMASK(2,S_SYS_PLL_IREF)

#define	S_SYS_PLL_VCO		    45
#define	M_SYS_PLL_VCO		    _SB_MAKEMASK(2,S_SYS_PLL_VCO)

#define	S_SYS_PLL_VREG		    47
#define	M_SYS_PLL_VREG		    _SB_MAKEMASK(2,S_SYS_PLL_VREG)

#define	M_SYS_MEM_RESET             _SB_MAKEMASK1(49)
#define	M_SYS_L2C_RESET             _SB_MAKEMASK1(50)
#define	M_SYS_IO_RESET_0            _SB_MAKEMASK1(51)
#define	M_SYS_IO_RESET_1            _SB_MAKEMASK1(52)
#define	M_SYS_SCD_RESET             _SB_MAKEMASK1(53)

/* End of bits writable by JTAG only. */

#define	M_SYS_CPU_RESET_0           _SB_MAKEMASK1(54)
#define	M_SYS_CPU_RESET_1           _SB_MAKEMASK1(55)

#define	M_SYS_UNICPU0               _SB_MAKEMASK1(56)
#define	M_SYS_UNICPU1               _SB_MAKEMASK1(57)

#define	M_SYS_SB_SOFTRES            _SB_MAKEMASK1(58)
#define	M_SYS_EXT_RESET             _SB_MAKEMASK1(59)
#define	M_SYS_SYSTEM_RESET          _SB_MAKEMASK1(60)

#define	M_SYS_MISR_MODE             _SB_MAKEMASK1(61)
#define	M_SYS_MISR_RESET            _SB_MAKEMASK1(62)

/*
 * Mailbox Registers (Table 4-3)
 * Registers: SCD_MBOX_CPU_x
 */

#define	S_MBOX_INT_3                0
#define	M_MBOX_INT_3                _SB_MAKEMASK(16,S_MBOX_INT_3)
#define	S_MBOX_INT_2                16
#define	M_MBOX_INT_2                _SB_MAKEMASK(16,S_MBOX_INT_2)
#define	S_MBOX_INT_1                32
#define	M_MBOX_INT_1                _SB_MAKEMASK(16,S_MBOX_INT_1)
#define	S_MBOX_INT_0                48
#define	M_MBOX_INT_0                _SB_MAKEMASK(16,S_MBOX_INT_0)

/*
 * Watchdog Registers (Table 4-8) (Table 4-9) (Table 4-10)
 * Registers: SCD_WDOG_INIT_CNT_x
 */

#define	V_SCD_WDOG_FREQ             1000000

#define	S_SCD_WDOG_INIT             0
#define	M_SCD_WDOG_INIT             _SB_MAKEMASK(13,S_SCD_WDOG_INIT)

#define	S_SCD_WDOG_CNT              0
#define	M_SCD_WDOG_CNT              _SB_MAKEMASK(13,S_SCD_WDOG_CNT)

#define	M_SCD_WDOG_ENABLE           _SB_MAKEMASK1(0)

/*
 * Timer Registers (Table 4-11) (Table 4-12) (Table 4-13)
 */

#define	V_SCD_TIMER_FREQ            1000000

#define	S_SCD_TIMER_INIT            0
#define	M_SCD_TIMER_INIT            _SB_MAKEMASK(20,S_SCD_TIMER_INIT)
#define	V_SCD_TIMER_INIT(x)         _SB_MAKEVALUE(x,S_SCD_TIMER_INIT)
#define	G_SCD_TIMER_INIT(x)         _SB_GETVALUE(x,S_SCD_TIMER_INIT,M_SCD_TIMER_INIT)

#define	S_SCD_TIMER_CNT             0
#define	M_SCD_TIMER_CNT             _SB_MAKEMASK(20,S_SCD_TIMER_CNT)
#define	V_SCD_TIMER_CNT(x)         _SB_MAKEVALUE(x,S_SCD_TIMER_CNT)
#define	G_SCD_TIMER_CNT(x)         _SB_GETVALUE(x,S_SCD_TIMER_CNT,M_SCD_TIMER_CNT)

#define	M_SCD_TIMER_ENABLE          _SB_MAKEMASK1(0)
#define	M_SCD_TIMER_MODE            _SB_MAKEMASK1(1)
#define	M_SCD_TIMER_MODE_CONTINUOUS M_SCD_TIMER_MODE

/*
 * System Performance Counters
 */

#define	S_SPC_CFG_SRC0            0
#define	M_SPC_CFG_SRC0            _SB_MAKEMASK(8,S_SPC_CFG_SRC0)
#define	V_SPC_CFG_SRC0(x)         _SB_MAKEVALUE(x,S_SPC_CFG_SRC0)
#define	G_SPC_CFG_SRC0(x)         _SB_GETVALUE(x,S_SPC_CFG_SRC0,M_SPC_CFG_SRC0)

#define	S_SPC_CFG_SRC1            8
#define	M_SPC_CFG_SRC1            _SB_MAKEMASK(8,S_SPC_CFG_SRC1)
#define	V_SPC_CFG_SRC1(x)         _SB_MAKEVALUE(x,S_SPC_CFG_SRC1)
#define	G_SPC_CFG_SRC1(x)         _SB_GETVALUE(x,S_SPC_CFG_SRC1,M_SPC_CFG_SRC1)

#define	S_SPC_CFG_SRC2            16
#define	M_SPC_CFG_SRC2            _SB_MAKEMASK(8,S_SPC_CFG_SRC2)
#define	V_SPC_CFG_SRC2(x)         _SB_MAKEVALUE(x,S_SPC_CFG_SRC2)
#define	G_SPC_CFG_SRC2(x)         _SB_GETVALUE(x,S_SPC_CFG_SRC2,M_SPC_CFG_SRC2)

#define	S_SPC_CFG_SRC3            24
#define	M_SPC_CFG_SRC3            _SB_MAKEMASK(8,S_SPC_CFG_SRC3)
#define	V_SPC_CFG_SRC3(x)         _SB_MAKEVALUE(x,S_SPC_CFG_SRC3)
#define	G_SPC_CFG_SRC3(x)         _SB_GETVALUE(x,S_SPC_CFG_SRC3,M_SPC_CFG_SRC3)

#define	M_SPC_CFG_CLEAR		_SB_MAKEMASK1(32)
#define	M_SPC_CFG_ENABLE	_SB_MAKEMASK1(33)


/*
 * Bus Watcher
 */

#define	S_SCD_BERR_TID            8
#define	M_SCD_BERR_TID            _SB_MAKEMASK(10,S_SCD_BERR_TID)
#define	V_SCD_BERR_TID(x)         _SB_MAKEVALUE(x,S_SCD_BERR_TID)
#define	G_SCD_BERR_TID(x)         _SB_GETVALUE(x,S_SCD_BERR_TID,M_SCD_BERR_TID)

#define	S_SCD_BERR_RID            18
#define	M_SCD_BERR_RID            _SB_MAKEMASK(4,S_SCD_BERR_RID)
#define	V_SCD_BERR_RID(x)         _SB_MAKEVALUE(x,S_SCD_BERR_RID)
#define	G_SCD_BERR_RID(x)         _SB_GETVALUE(x,S_SCD_BERR_RID,M_SCD_BERR_RID)

#define	S_SCD_BERR_DCODE            22
#define	M_SCD_BERR_DCODE            _SB_MAKEMASK(3,S_SCD_BERR_DCODE)
#define	V_SCD_BERR_DCODE(x)         _SB_MAKEVALUE(x,S_SCD_BERR_DCODE)
#define	G_SCD_BERR_DCODE(x)         _SB_GETVALUE(x,S_SCD_BERR_DCODE,M_SCD_BERR_DCODE)

#define	M_SCD_BERR_MULTERRS	_SB_MAKEMASK1(30)


#define	S_SCD_L2ECC_CORR_D            0
#define	M_SCD_L2ECC_CORR_D            _SB_MAKEMASK(8,S_SCD_L2ECC_CORR_D)
#define	V_SCD_L2ECC_CORR_D(x)         _SB_MAKEVALUE(x,S_SCD_L2ECC_CORR_D)
#define	G_SCD_L2ECC_CORR_D(x)         _SB_GETVALUE(x,S_SCD_L2ECC_CORR_D,M_SCD_L2ECC_CORR_D)

#define	S_SCD_L2ECC_BAD_D            8
#define	M_SCD_L2ECC_BAD_D            _SB_MAKEMASK(8,S_SCD_L2ECC_BAD_D)
#define	V_SCD_L2ECC_BAD_D(x)         _SB_MAKEVALUE(x,S_SCD_L2ECC_BAD_D)
#define	G_SCD_L2ECC_BAD_D(x)         _SB_GETVALUE(x,S_SCD_L2ECC_BAD_D,M_SCD_L2ECC_BAD_D)

#define	S_SCD_L2ECC_CORR_T            16
#define	M_SCD_L2ECC_CORR_T            _SB_MAKEMASK(8,S_SCD_L2ECC_CORR_T)
#define	V_SCD_L2ECC_CORR_T(x)         _SB_MAKEVALUE(x,S_SCD_L2ECC_CORR_T)
#define	G_SCD_L2ECC_CORR_T(x)         _SB_GETVALUE(x,S_SCD_L2ECC_CORR_T,M_SCD_L2ECC_CORR_T)

#define	S_SCD_L2ECC_BAD_T            24
#define	M_SCD_L2ECC_BAD_T            _SB_MAKEMASK(8,S_SCD_L2ECC_BAD_T)
#define	V_SCD_L2ECC_BAD_T(x)         _SB_MAKEVALUE(x,S_SCD_L2ECC_BAD_T)
#define	G_SCD_L2ECC_BAD_T(x)         _SB_GETVALUE(x,S_SCD_L2ECC_BAD_T,M_SCD_L2ECC_BAD_T)

#define	S_SCD_MEM_ECC_CORR            0
#define	M_SCD_MEM_ECC_CORR            _SB_MAKEMASK(8,S_SCD_MEM_ECC_CORR)
#define	V_SCD_MEM_ECC_CORR(x)         _SB_MAKEVALUE(x,S_SCD_MEM_ECC_CORR)
#define	G_SCD_MEM_ECC_CORR(x)         _SB_GETVALUE(x,S_SCD_MEM_ECC_CORR,M_SCD_MEM_ECC_CORR)

#define	S_SCD_MEM_ECC_BAD            16
#define	M_SCD_MEM_ECC_BAD            _SB_MAKEMASK(8,S_SCD_MEM_ECC_BAD)
#define	V_SCD_MEM_ECC_BAD(x)         _SB_MAKEVALUE(x,S_SCD_MEM_ECC_BAD)
#define	G_SCD_MEM_ECC_BAD(x)         _SB_GETVALUE(x,S_SCD_MEM_ECC_BAD,M_SCD_MEM_ECC_BAD)

#define	S_SCD_MEM_BUSERR            24
#define	M_SCD_MEM_BUSERR            _SB_MAKEMASK(8,S_SCD_MEM_BUSERR)
#define	V_SCD_MEM_BUSERR(x)         _SB_MAKEVALUE(x,S_SCD_MEM_BUSERR)
#define	G_SCD_MEM_BUSERR(x)         _SB_GETVALUE(x,S_SCD_MEM_BUSERR,M_SCD_MEM_BUSERR)


/*
 * Address Trap Registers
 */

#define	M_ATRAP_INDEX		  _SB_MAKEMASK(4,0)
#define	M_ATRAP_ADDRESS		  _SB_MAKEMASK(40,0)

#define	S_ATRAP_CFG_CNT            0
#define	M_ATRAP_CFG_CNT            _SB_MAKEMASK(3,S_ATRAP_CFG_CNT)
#define	V_ATRAP_CFG_CNT(x)         _SB_MAKEVALUE(x,S_ATRAP_CFG_CNT)
#define	G_ATRAP_CFG_CNT(x)         _SB_GETVALUE(x,S_ATRAP_CFG_CNT,M_ATRAP_CFG_CNT)

#define	M_ATRAP_CFG_WRITE	   _SB_MAKEMASK1(3)
#define	M_ATRAP_CFG_ALL	  	   _SB_MAKEMASK1(4)
#define	M_ATRAP_CFG_INV	   	   _SB_MAKEMASK1(5)
#define	M_ATRAP_CFG_USESRC	   _SB_MAKEMASK1(6)
#define	M_ATRAP_CFG_SRCINV	   _SB_MAKEMASK1(7)

#define	S_ATRAP_CFG_AGENTID     8
#define	M_ATRAP_CFG_AGENTID     _SB_MAKEMASK(4,S_ATRAP_CFG_AGENTID)
#define	V_ATRAP_CFG_AGENTID(x)  _SB_MAKEVALUE(x,S_ATRAP_CFG_AGENTID)
#define	G_ATRAP_CFG_AGENTID(x)  _SB_GETVALUE(x,S_ATRAP_CFG_AGENTID,M_ATRAP_CFG_AGENTID)

#define	K_BUS_AGENT_CPU0	0
#define	K_BUS_AGENT_CPU1	1
#define	K_BUS_AGENT_IOB0	2
#define	K_BUS_AGENT_IOB1	3
#define	K_BUS_AGENT_SCD	4
#define	K_BUS_AGENT_RESERVED	5
#define	K_BUS_AGENT_L2C	6
#define	K_BUS_AGENT_MC	7

#define	S_ATRAP_CFG_CATTR     12
#define	M_ATRAP_CFG_CATTR     _SB_MAKEMASK(3,S_ATRAP_CFG_CATTR)
#define	V_ATRAP_CFG_CATTR(x)  _SB_MAKEVALUE(x,S_ATRAP_CFG_CATTR)
#define	G_ATRAP_CFG_CATTR(x)  _SB_GETVALUE(x,S_ATRAP_CFG_CATTR,M_ATRAP_CFG_CATTR)

#define	K_ATRAP_CFG_CATTR_IGNORE	0
#define	K_ATRAP_CFG_CATTR_UNC    	1
#define	K_ATRAP_CFG_CATTR_CACHEABLE	2
#define	K_ATRAP_CFG_CATTR_NONCOH  	3
#define	K_ATRAP_CFG_CATTR_COHERENT	4
#define	K_ATRAP_CFG_CATTR_NOTUNC	5
#define	K_ATRAP_CFG_CATTR_NOTNONCOH	6
#define	K_ATRAP_CFG_CATTR_NOTCOHERENT   7

/*
 * Trace Buffer Config register
 */

#define	M_SCD_TRACE_CFG_RESET           _SB_MAKEMASK1(0)
#define	M_SCD_TRACE_CFG_START_READ      _SB_MAKEMASK1(1)
#define	M_SCD_TRACE_CFG_START           _SB_MAKEMASK1(2)
#define	M_SCD_TRACE_CFG_STOP            _SB_MAKEMASK1(3)
#define	M_SCD_TRACE_CFG_FREEZE          _SB_MAKEMASK1(4)
#define	M_SCD_TRACE_CFG_FREEZE_FULL     _SB_MAKEMASK1(5)
#define	M_SCD_TRACE_CFG_DEBUG_FULL      _SB_MAKEMASK1(6)
#define	M_SCD_TRACE_CFG_FULL            _SB_MAKEMASK1(7)

#define	S_SCD_TRACE_CFG_CUR_ADDR        10
#define	M_SCD_TRACE_CFG_CUR_ADDR        _SB_MAKEMASK(8,S_SCD_TRACE_CFG_CUR_ADDR)
#define	V_SCD_TRACE_CFG_CUR_ADDR(x)     _SB_MAKEVALUE(x,S_SCD_TRACE_CFG_CUR_ADDR)
#define	G_SCD_TRACE_CFG_CUR_ADDR(x)     _SB_GETVALUE(x,S_SCD_TRACE_CFG_CUR_ADDR,M_SCD_TRACE_CFG_CUR_ADDR)

/*
 * Trace Event registers
 */

#define	S_SCD_TREVT_ADDR_MATCH          0
#define	M_SCD_TREVT_ADDR_MATCH          _SB_MAKEMASK(4,S_SCD_TREVT_ADDR_MATCH)
#define	V_SCD_TREVT_ADDR_MATCH(x)       _SB_MAKEVALUE(x,S_SCD_TREVT_ADDR_MATCH)
#define	G_SCD_TREVT_ADDR_MATCH(x)       _SB_GETVALUE(x,S_SCD_TREVT_ADDR_MATCH,M_SCD_TREVT_ADDR_MATCH)

#define	M_SCD_TREVT_REQID_MATCH         _SB_MAKEMASK1(4)
#define	M_SCD_TREVT_DATAID_MATCH        _SB_MAKEMASK1(5)
#define	M_SCD_TREVT_RESPID_MATCH        _SB_MAKEMASK1(6)
#define	M_SCD_TREVT_INTERRUPT           _SB_MAKEMASK1(7)
#define	M_SCD_TREVT_DEBUG_PIN           _SB_MAKEMASK1(9)
#define	M_SCD_TREVT_WRITE               _SB_MAKEMASK1(10)
#define	M_SCD_TREVT_READ                _SB_MAKEMASK1(11)

#define	S_SCD_TREVT_REQID               12
#define	M_SCD_TREVT_REQID               _SB_MAKEMASK(4,S_SCD_TREVT_REQID)
#define	V_SCD_TREVT_REQID(x)            _SB_MAKEVALUE(x,S_SCD_TREVT_REQID)
#define	G_SCD_TREVT_REQID(x)            _SB_GETVALUE(x,S_SCD_TREVT_REQID,M_SCD_TREVT_REQID)

#define	S_SCD_TREVT_RESPID              16
#define	M_SCD_TREVT_RESPID              _SB_MAKEMASK(4,S_SCD_TREVT_RESPID)
#define	V_SCD_TREVT_RESPID(x)           _SB_MAKEVALUE(x,S_SCD_TREVT_RESPID)
#define	G_SCD_TREVT_RESPID(x)           _SB_GETVALUE(x,S_SCD_TREVT_RESPID,M_SCD_TREVT_RESPID)

#define	S_SCD_TREVT_DATAID              20
#define	M_SCD_TREVT_DATAID              _SB_MAKEMASK(4,S_SCD_TREVT_DATAID)
#define	V_SCD_TREVT_DATAID(x)           _SB_MAKEVALUE(x,S_SCD_TREVT_DATAID)
#define	G_SCD_TREVT_DATAID(x)           _SB_GETVALUE(x,S_SCD_TREVT_DATAID,M_SCD_TREVT_DATID)

#define	S_SCD_TREVT_COUNT               24
#define	M_SCD_TREVT_COUNT               _SB_MAKEMASK(8,S_SCD_TREVT_COUNT)
#define	V_SCD_TREVT_COUNT(x)            _SB_MAKEVALUE(x,S_SCD_TREVT_COUNT)
#define	G_SCD_TREVT_COUNT(x)            _SB_GETVALUE(x,S_SCD_TREVT_COUNT,M_SCD_TREVT_COUNT)

/*
 * Trace Sequence registers
 */

#define	S_SCD_TRSEQ_EVENT4              0
#define	M_SCD_TRSEQ_EVENT4              _SB_MAKEMASK(4,S_SCD_TRSEQ_EVENT4)
#define	V_SCD_TRSEQ_EVENT4(x)           _SB_MAKEVALUE(x,S_SCD_TRSEQ_EVENT4)
#define	G_SCD_TRSEQ_EVENT4(x)           _SB_GETVALUE(x,S_SCD_TRSEQ_EVENT4,M_SCD_TRSEQ_EVENT4)

#define	S_SCD_TRSEQ_EVENT3              4
#define	M_SCD_TRSEQ_EVENT3              _SB_MAKEMASK(4,S_SCD_TRSEQ_EVENT3)
#define	V_SCD_TRSEQ_EVENT3(x)           _SB_MAKEVALUE(x,S_SCD_TRSEQ_EVENT3)
#define	G_SCD_TRSEQ_EVENT3(x)           _SB_GETVALUE(x,S_SCD_TRSEQ_EVENT3,M_SCD_TRSEQ_EVENT3)

#define	S_SCD_TRSEQ_EVENT2              8
#define	M_SCD_TRSEQ_EVENT2              _SB_MAKEMASK(4,S_SCD_TRSEQ_EVENT2)
#define	V_SCD_TRSEQ_EVENT2(x)           _SB_MAKEVALUE(x,S_SCD_TRSEQ_EVENT2)
#define	G_SCD_TRSEQ_EVENT2(x)           _SB_GETVALUE(x,S_SCD_TRSEQ_EVENT2,M_SCD_TRSEQ_EVENT2)

#define	S_SCD_TRSEQ_EVENT1              12
#define	M_SCD_TRSEQ_EVENT1              _SB_MAKEMASK(4,S_SCD_TRSEQ_EVENT1)
#define	V_SCD_TRSEQ_EVENT1(x)           _SB_MAKEVALUE(x,S_SCD_TRSEQ_EVENT1)
#define	G_SCD_TRSEQ_EVENT1(x)           _SB_GETVALUE(x,S_SCD_TRSEQ_EVENT1,M_SCD_TRSEQ_EVENT1)

#define	K_SCD_TRSEQ_E0                  0
#define	K_SCD_TRSEQ_E1                  1
#define	K_SCD_TRSEQ_E2                  2
#define	K_SCD_TRSEQ_E3                  3
#define	K_SCD_TRSEQ_E0_E1               4
#define	K_SCD_TRSEQ_E1_E2               5
#define	K_SCD_TRSEQ_E2_E3               6
#define	K_SCD_TRSEQ_E0_E1_E2            7
#define	K_SCD_TRSEQ_E0_E1_E2_E3         8
#define	K_SCD_TRSEQ_E0E1                9
#define	K_SCD_TRSEQ_E0E1E2              10
#define	K_SCD_TRSEQ_E0E1E2E3            11
#define	K_SCD_TRSEQ_E0E1_E2             12
#define	K_SCD_TRSEQ_E0E1_E2E3           13
#define	K_SCD_TRSEQ_E0E1_E2_E3          14
#define	K_SCD_TRSEQ_IGNORED             15

#define	K_SCD_TRSEQ_TRIGGER_ALL         (V_SCD_TRSEQ_EVENT1(K_SCD_TRSEQ_IGNORED) | \
					 V_SCD_TRSEQ_EVENT2(K_SCD_TRSEQ_IGNORED) | \
					 V_SCD_TRSEQ_EVENT3(K_SCD_TRSEQ_IGNORED) | \
					 V_SCD_TRSEQ_EVENT4(K_SCD_TRSEQ_IGNORED))

#define	S_SCD_TRSEQ_FUNCTION            16
#define	M_SCD_TRSEQ_FUNCTION            _SB_MAKEMASK(4,S_SCD_TRSEQ_FUNCTION)
#define	V_SCD_TRSEQ_FUNCTION(x)         _SB_MAKEVALUE(x,S_SCD_TRSEQ_FUNCTION)
#define	G_SCD_TRSEQ_FUNCTION(x)         _SB_GETVALUE(x,S_SCD_TRSEQ_FUNCTION,M_SCD_TRSEQ_FUNCTION)

#define	K_SCD_TRSEQ_FUNC_NOP            0
#define	K_SCD_TRSEQ_FUNC_START          1
#define	K_SCD_TRSEQ_FUNC_STOP           2
#define	K_SCD_TRSEQ_FUNC_FREEZE         3

#define	V_SCD_TRSEQ_FUNC_NOP            V_SCD_TRSEQ_FUNCTION(K_SCD_TRSEQ_FUNC_NOP)
#define	V_SCD_TRSEQ_FUNC_START          V_SCD_TRSEQ_FUNCTION(K_SCD_TRSEQ_FUNC_START)
#define	V_SCD_TRSEQ_FUNC_STOP           V_SCD_TRSEQ_FUNCTION(K_SCD_TRSEQ_FUNC_STOP)
#define	V_SCD_TRSEQ_FUNC_FREEZE         V_SCD_TRSEQ_FUNCTION(K_SCD_TRSEQ_FUNC_FREEZE)

#define	M_SCD_TRSEQ_ASAMPLE             _SB_MAKEMASK1(18)
#define	M_SCD_TRSEQ_DSAMPLE             _SB_MAKEMASK1(19)
#define	M_SCD_TRSEQ_DEBUGPIN            _SB_MAKEMASK1(20)
#define	M_SCD_TRSEQ_DEBUGCPU            _SB_MAKEMASK1(21)
#define	M_SCD_TRSEQ_CLEARUSE            _SB_MAKEMASK1(22)

#endif
