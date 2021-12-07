/*	$NetBSD: scmdctlconst.h,v 1.1 2021/12/07 17:39:55 brad Exp $	*/

/*
 * Copyright (c) 2021 Brad Spencer <brad@anduin.eldar.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _SCMDCTLCONST_H_
#define _SCMDCTLCONST_H_

static const struct scmdcmd scmdcmds[] = {
	{
		.cmd = "identify",
		.id = SCMD_IDENTIFY,
		.helpargs = "[module]"
	},
	{
		.cmd = "diagnostics",
		.id = SCMD_DIAG,
		.helpargs = "[module]"
	},
	{
		.cmd = "motor",
		.id = SCMD_MOTOR,
		.helpargs = "get|set|invert|bridge|enable|disable module([get]|set|invert|bridge) A|B(set|invert) value(set)"
	},
	{
		.cmd = "read_register",
		.id = SCMD_READ,
		.helpargs = "module register [register_end]"
	},
	{
		.cmd = "write_register",
		.id = SCMD_WRITE,
		.helpargs = "module register value"
	},
	{
		.cmd = "restart",
		.id = SCMD_RESTART,
		.helpargs = ""
	},
	{
		.cmd = "re-enumerate",
		.id = SCMD_ENUMERATE,
		.helpargs = ""
	},
	{
		.cmd = "update_rate",
		.id = SCMD_UPDATERATE,
		.helpargs = "get|set|force rate(set)"
	},
	{
		.cmd = "expansion_bus",
		.id = SCMD_EBUS,
		.helpargs = "get|set 50kHz|100kHz|400kHz(set)"
	},
	{
		.cmd = "lock",
		.id = SCMD_LOCK,
		.helpargs = "get|lock|unlock local_user|local_master|global_user|global_master"
	},
	{
		.cmd = "spi_read_one",
		.id = SCMD_SPIREADONE,
		.helpargs = ""
	}
};

static const struct scmdcmd motorsubcmds[] = {
	{
		.cmd = "get",
		.id = SCMD_SUBMOTORGET,
		.helpargs = ""
	},
	{
		.cmd = "set",
		.id = SCMD_SUBMOTORSET,
		.helpargs = ""
	},
	{
		.cmd = "invert",
		.id = SCMD_SUBMOTORINVERT,
		.helpargs = ""
	},
	{
		.cmd = "bridge",
		.id = SCMD_SUBMOTORBRIDGE,
		.helpargs = ""
	},
	{
		.cmd = "disable",
		.id = SCMD_SUBMOTORDISABLE,
		.helpargs = ""
	},
	{
		.cmd = "enable",
		.id = SCMD_SUBMOTORENABLE,
		.helpargs = ""
	}
};

static const struct scmdcmd updateratesubcmds[] = {
	{
		.cmd = "get",
		.id = SCMD_SUBURGET,
		.helpargs = ""
	},
	{
		.cmd = "set",
		.id = SCMD_SUBURSET,
		.helpargs = ""
	},
	{
		.cmd = "force",
		.id = SCMD_SUBURFORCE,
		.helpargs = ""
	}
};

static const struct scmdcmd ebussubcmds[] = {
	{
		.cmd = "get",
		.id = SCMD_SUBEBUSGET,
		.helpargs = ""
	},
	{
		.cmd = "set",
		.id = SCMD_SUBEBUSSET,
		.helpargs = ""
	}
};

static const struct scmdcmd locksubcmds[] = {
	{
		.cmd = "get",
		.id = SCMD_SUBLOCKGET,
		.helpargs = ""
	},
	{
		.cmd = "lock",
		.id = SCMD_SUBLOCKLOCK,
		.helpargs = ""
	},
	{
		.cmd = "unlock",
		.id = SCMD_SUBLOCKUNLOCK,
		.helpargs = ""
	}
};

static const struct scmdcmd lockcmdtypes[] = {
	{
		.cmd = "local_user",
		.id = SCMD_LOCAL_USER_LOCK,
		.helpargs = ""
	},
	{
		.cmd = "local_master",
		.id = SCMD_LOCAL_MASTER_LOCK,
		.helpargs = ""
	},
	{
		.cmd = "global_user",
		.id = SCMD_GLOBAL_USER_LOCK,
		.helpargs = ""
	},
	{
		.cmd = "global_master",
		.id = SCMD_GLOBAL_MASTER_LOCK,
		.helpargs = ""
	}
};

static const char *ebus_speeds[] = {
	"50kHz","100kHz","400kHz"
};

static const char *scmdregisternames[] = {
	"FID",
	"ID",
	"SLAVE_ADDR",
	"CONFIG_BITS",
	"U_I2C_RD_ERR",
	"U_I2C_WR_ERR",
	"U_BUF_DUMPED",
	"E_I2C_RD_ERR",
	"E_I2C_WR_ERR",
	"LOOP_TIME",
	"SLV_POLL_CNT",
	"SLV_TOP_ADDR",
	"MST_E_ERR",
	"MST_E_STATUS",
	"FSAFE_FAULTS",
	"REG_OOR_CNT",
	"REG_RO_WRITE_CNT",
	"GEN_TEST_WORD",
	"MOTOR_A_INVERT",
	"MOTOR_B_INVERT",
	"BRIDGE",
	"LOCAL_MASTER_LOCK",
	"LOCAL_USER_LOCK",
	"MST_E_IN_FN",
	"U_PORT_CLKDIV_U",
	"U_PORT_CLKDIV_L",
	"U_PORT_CLKDIV_CTRL",
	"E_PORT_CLKDIV_U",
	"E_PORT_CLKDIV_L",
	"E_PORT_CLKDIV_CTRL",
	"U_BUS_UART_BAUD",
	"FSAFE_CTRL",
	"MA_DRIVE",
	"MB_DRIVE",
	"S1A_DRIVE",
	"S1B_DRIVE",
	"S2A_DRIVE",
	"S2B_DRIVE",
	"S3A_DRIVE",
	"S3B_DRIVE",
	"S4A_DRIVE",
	"S4B_DRIVE",
	"S5A_DRIVE",
	"S5B_DRIVE",
	"S6A_DRIVE",
	"S6B_DRIVE",
	"S7A_DRIVE",
	"S7B_DRIVE",
	"S8A_DRIVE",
	"S8B_DRIVE",
	"S9A_DRIVE",
	"S9B_DRIVE",
	"S10A_DRIVE",
	"S10B_DRIVE",
	"S11A_DRIVE",
	"S11B_DRIVE",
	"S12A_DRIVE",
	"S12B_DRIVE",
	"S13A_DRIVE",
	"S13B_DRIVE",
	"S14A_DRIVE",
	"S14B_DRIVE",
	"S15A_DRIVE",
	"S15B_DRIVE",
	"S16A_DRIVE",
	"S16B_DRIVE",
	"HOLE_1_42",
	"HOLE_1_43",
	"HOLE_1_44",
	"HOLE_1_45",
	"HOLE_1_46",
	"HOLE_1_47",
	"HOLE_1_48",
	"HOLE_1_49",
	"HOLE_1_4A",
	"HOLE_1_4B",
	"HOLE_1_4C",
	"HOLE_1_4D",
	"HOLE_1_4E",
	"HOLE_1_4F",
	"INV_2_9",
	"INV_10_17",
	"INV_18_25",
	"INV_26_33",
	"BRIDGE_SLV_L",
	"BRIDGE_SLV_H",
	"HOLE_2_56",
	"HOLE_2_57",
	"HOLE_2_58",
	"HOLE_2_59",
	"HOLE_2_5A",
	"HOLE_2_5B",
	"HOLE_2_5C",
	"HOLE_2_5D",
	"HOLE_2_5E",
	"HOLE_2_5F",
	"HOLE_2_60",
	"HOLE_2_61",
	"HOLE_2_62",
	"HOLE_2_63",
	"HOLE_2_64",
	"HOLE_2_65",
	"HOLE_2_66",
	"HOLE_2_67",
	"HOLE_2_68",
	"HOLE_2_69",
	"HOLE_2_6A",
	"HOLE_2_6B",
	"HOLE_2_6C",
	"HOLE_2_6D",
	"HOLE_2_6E",
	"PAGE_SELECT",
	"DRIVER_ENABLE",
	"UPDATE_RATE",
	"FORCE_UPDATE",
	"E_BUS_SPEED",
	"MASTER_LOCK",
	"USER_LOCK",
	"FSAFE_TIME",
	"STATUS_1",
	"CONTROL_1",
	"REM_ADDR",
	"REM_OFFSET",
	"REM_DATA_WR",
	"REM_DATA_RD",
	"REM_WRITE",
	"REM_READ"
};

#endif
