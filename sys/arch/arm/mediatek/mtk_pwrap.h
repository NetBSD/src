/*-
* Copyright (c) 2017 Henry Chen <henryc.chen@mediatek.com>
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
* THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
* IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
* OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
* IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
* BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
* AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
* OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
* SUCH DAMAGE.
*/

#ifndef ARM_MEDIATEK_MTK_PMIC_WRAP_H
#define ARM_MEDIATEK_MTK_PMIC_WRAP_H

#include <arm/mediatek/mercury_reg.h>
#include <dev/pwrap/pwrapvar.h>

#define PWRAP_CG_AP		(1 << 20)
#define PWRAP_CG_MD		(1 << 27)
#define PWRAP_CG_CONN	(1 << 28)
#define PWRAP_CG_26M	(1 << 29)

#define PMIC_B_DCM_EN	(1 << 1)
#define PMIC_SPI_DCM_EN	(1 << 2)

#define CLK_SPI_CK_26M		0x1

enum {
	WACS2 = 1 << 3
};

/* timeout setting */
enum {
	TIMEOUT_READ_US        = 255,
	TIMEOUT_WAIT_IDLE_US   = 255
};

/* PMIC_WRAP registers */
#define PWRAP_MUX_SEL		0x0
#define PWRAP_WRAP_EN		0x4
#define PWRAP_DIO_EN		0x8
#define PWRAP_SIDLY		0xc
#define PWRAP_RDDMY		0x10
#define PWRAP_SI_CK_CON		0x14
#define PWRAP_CSHEXT_WRITE	0x18
#define PWRAP_CSHEXT_READ	0x1c
#define PWRAP_CSLEXT_START	0x20
#define PWRAP_CSLEXT_END	0x24
#define PWRAP_STAUPD_PRD	0x28
#define PWRAP_STAUPD_GRPEN	0x2c
#define PWRAP_STAUPD_MAN_TRIG	0x40
#define PWRAP_STAUPD_STA	0x44
#define PWRAP_WRAP_STA		0x48
#define PWRAP_HARB_INIT		0x4c
#define PWRAP_HARB_HPRIO	0x50
#define PWRAP_HIPRIO_ARB_EN	0x54
#define PWRAP_HARB_STA0		0x58
#define PWRAP_HARB_STA1		0x5c
#define PWRAP_MAN_EN		0x60
#define PWRAP_MAN_CMD		0x64
#define PWRAP_MAN_RDATA		0x68
#define PWRAP_MAN_VLDCLR	0x6c
#define PWRAP_WACS0_EN		0x70
#define PWRAP_INIT_DONE0	0x74
#define PWRAP_WACS0_CMD		0x78
#define PWRAP_WACS0_RDATA	0x7c
#define PWRAP_WACS0_VLDCLR	0x80
#define PWRAP_WACS1_EN		0x84
#define PWRAP_INIT_DONE1	0x88
#define PWRAP_WACS1_CMD		0x8c
#define PWRAP_WACS1_RDATA	0x90
#define PWRAP_WACS1_VLDCLR	0x94
#define PWRAP_WACS2_EN		0x98
#define PWRAP_INIT_DONE2	0x9c
#define PWRAP_WACS2_CMD		0xa0
#define PWRAP_WACS2_RDATA	0xa4
#define PWRAP_WACS2_VLDCLR	0xa8
#define PWRAP_INT_EN		0xac
#define PWRAP_INT_FLG_RAW	0xb0
#define PWRAP_INT_FLG		0xb4
#define PWRAP_INT_CLR		0xb8
#define PWRAP_SIG_ADR		0xbc
#define PWRAP_SIG_MODE		0xc0
#define PWRAP_SIG_VALUE		0xc4
#define PWRAP_SIG_ERRVAL	0xc8
#define PWRAP_CRC_EN		0xcc
#define PWRAP_TIMER_EN		0xd0
#define PWRAP_TIMER_STA		0xd4
#define PWRAP_WDT_UNIT		0xd8
#define PWRAP_WDT_SRC_EN	0xdc
#define PWRAP_WDT_FLG		0xe0
#define PWRAP_DEBUG_INT_SEL	0xe4
#define PWRAP_DVFS_ADR0		0xe8
#define PWRAP_DVFS_WDATA0	0xec
#define PWRAP_DVFS_ADR1		0xf0
#define PWRAP_DVFS_WDATA1	0xf4
#define PWRAP_DVFS_ADR2		0xf8
#define PWRAP_DVFS_WDATA2	0xfc
#define PWRAP_DVFS_ADR3		0x100
#define PWRAP_DVFS_WDATA3	0x104
#define PWRAP_DVFS_ADR4		0x108
#define PWRAP_DVFS_WDATA4	0x10c
#define PWRAP_DVFS_ADR5		0x110
#define PWRAP_DVFS_WDATA5	0x114
#define PWRAP_DVFS_ADR6		0x118
#define PWRAP_DVFS_WDATA6	0x11c
#define PWRAP_DVFS_ADR7		0x120
#define PWRAP_DVFS_WDATA7	0x124
#define PWRAP_SPMINF_STA	0x128
#define PWRAP_CIPHER_KEY_SEL	0x12c
#define PWRAP_CIPHER_IV_SEL	0x130
#define PWRAP_CIPHER_EN		0x134
#define PWRAP_CIPHER_RDY	0x138
#define PWRAP_CIPHER_MODE	0x13c
#define PWRAP_CIPHER_SWRST	0x140
#define PWRAP_DCM_EN		0x144
#define PWRAP_DCM_DBC_PRD	0x148
#define PWRAP_SW_RST		0x168
#define PWRAP_OP_TYPE		0x16c
#define PWRAP_MSB_FIRST		0x170

enum {
	RDATA_WACS_RDATA_SHIFT = 0,
	RDATA_WACS_FSM_SHIFT   = 16,
	RDATA_WACS_REQ_SHIFT   = 19,
	RDATA_SYNC_IDLE_SHIFT,
	RDATA_INIT_DONE_SHIFT,
	RDATA_SYS_IDLE_SHIFT,
};

enum {
	RDATA_WACS_RDATA_MASK = 0xffff,
	RDATA_WACS_FSM_MASK   = 0x7,
	RDATA_WACS_REQ_MASK   = 0x1,
	RDATA_SYNC_IDLE_MASK  = 0x1,
	RDATA_INIT_DONE_MASK  = 0x1,
	RDATA_SYS_IDLE_MASK   = 0x1,
};

/* WACS_FSM */
enum {
	WACS_FSM_IDLE     = 0x00,
	WACS_FSM_REQ      = 0x02,
	WACS_FSM_WFDLE    = 0x04, /* wait for dle, wait for read data done */
	WACS_FSM_WFVLDCLR = 0x06, /* finish read data, wait for valid flag
				   * clearing */
	WACS_INIT_DONE    = 0x01,
	WACS_SYNC_IDLE    = 0x01,
	WACS_SYNC_BUSY    = 0x00
};

/* dewrapper defaule value */
enum {
	DEFAULT_VALUE_READ_TEST  = 0x5aa5,
	WRITE_TEST_VALUE         = 0xa55a
};

enum pmic_regck {
	REG_CLOCK_18MHZ,
	REG_CLOCK_26MHZ,
	REG_CLOCK_SAFE_MODE
};

/* manual commnd */
enum {
	OP_WR    = 0x1,
	OP_CSH   = 0x0,
	OP_CSL   = 0x1,
	OP_OUTS  = 0x8,
	OP_OUTD  = 0x9,
	OP_INS   = 0xC,
	OP_IND   = 0xE
};

/* error information flag */
enum {
	E_PWR_INVALID_ARG             = 1,
	E_PWR_INVALID_RW              = 2,
	E_PWR_INVALID_ADDR            = 3,
	E_PWR_INVALID_WDAT            = 4,
	E_PWR_INVALID_OP_MANUAL       = 5,
	E_PWR_NOT_IDLE_STATE          = 6,
	E_PWR_NOT_INIT_DONE           = 7,
	E_PWR_NOT_INIT_DONE_READ      = 8,
	E_PWR_WAIT_IDLE_TIMEOUT       = 9,
	E_PWR_WAIT_IDLE_TIMEOUT_READ  = 10,
	E_PWR_INIT_SIDLY_FAIL         = 11,
	E_PWR_RESET_TIMEOUT           = 12,
	E_PWR_TIMEOUT                 = 13,
	E_PWR_INIT_RESET_SPI          = 20,
	E_PWR_INIT_SIDLY              = 21,
	E_PWR_INIT_REG_CLOCK          = 22,
	E_PWR_INIT_ENABLE_PMIC        = 23,
	E_PWR_INIT_DIO                = 24,
	E_PWR_INIT_CIPHER             = 25,
	E_PWR_INIT_WRITE_TEST         = 26,
	E_PWR_INIT_ENABLE_CRC         = 27,
	E_PWR_INIT_ENABLE_DEWRAP      = 28,
	E_PWR_INIT_ENABLE_EVENT       = 29,
	E_PWR_READ_TEST_FAIL          = 30,
	E_PWR_WRITE_TEST_FAIL         = 31,
	E_PWR_SWITCH_DIO              = 32,
	E_PWR_INVALID_DATA            = 33
};

#endif /* ARM_MEDIATEK_MTK_PMIC_WRAP_H */
