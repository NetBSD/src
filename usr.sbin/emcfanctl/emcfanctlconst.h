/*	$NetBSD: emcfanctlconst.h,v 1.1 2025/03/11 13:56:48 brad Exp $	*/

/*
 * Copyright (c) 2025 Brad Spencer <brad@anduin.eldar.org>
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

#ifndef _EMCFANCTLCONST_H_
#define _EMCFANCTLCONST_H_

/* These structures describe the command line command structure */

static const struct emcfanctlcmd emcfanctlcmds[] = {
	{
		.cmd = "info",
		.id = EMCFANCTL_INFO,
		.helpargs = ""

	},
	{
		.cmd = "register",
		.id = EMCFANCTL_REGISTER,
		.helpargs = "list|read|write"

	},
	{
		.cmd = "fan",
		.id = EMCFANCTL_FAN,
		.helpargs = "<n> drive|divider|min_expected_rpm|edges|polarity read|write|inverted|non-inverted <write value>"
	},
	{
		.cmd = "apd",
		.id = EMCFANCTL_APD,
		.helpargs = "read|on|off"
	},
	{
		.cmd = "smbus_timeout",
		.id = EMCFANCTL_SMBUSTO,
		.helpargs = "read|on|off"
	}
};

static const struct emcfanctlcmd emcfanctlregistercmds[] = {
	{
		.cmd = "list",
		.id = EMCFANCTL_REGISTER_LIST,
		.helpargs = "list"

	},
	{
		.cmd = "read",
		.id = EMCFANCTL_REGISTER_READ,
		.helpargs = "read start_register [end_register]"

	},
	{
		.cmd = "write",
		.id = EMCFANCTL_REGISTER_WRITE,
		.helpargs = "write register value"

	}
};

static const struct emcfanctlcmd emcfanctlfancmds[] = {
	{
		.cmd = "status",
		.id = EMCFANCTL_FAN_STATUS,
		.helpargs = "<n> status"
	},
	{
		.cmd = "drive",
		.id = EMCFANCTL_FAN_DRIVE,
		.helpargs = "drive"
	},
	{
		.cmd = "divider",
		.id = EMCFANCTL_FAN_DIVIDER,
		.helpargs = "divider"
	},
	{
		.cmd = "min_expected_rpm",
		.id = EMCFANCTL_FAN_MINEXPECTED_RPM,
		.helpargs = "min_expected_rpm"
	},
	{
		.cmd = "edges",
		.id = EMCFANCTL_FAN_EDGES,
		.helpargs = "edges"
	},
	{
		.cmd = "polarity",
		.id = EMCFANCTL_FAN_POLARITY,
		.helpargs = "polarity read|inverted|non-inverted"
	},
	{
		.cmd = "pwm_base_frequency",
		.id = EMCFANCTL_FAN_PWM_BASEFREQ,
		.helpargs = "pwm_base_frequency read|write"
	},
	{
		.cmd = "pwm_output_type",
		.id = EMCFANCTL_FAN_PWM_OUTPUTTYPE,
		.helpargs = "pwm_output_type read|push-pull|open-drain"
	}
};

static const struct emcfanctlcmd emcfanctlddcmds[] = {
	{
		.cmd = "read",
		.id = EMCFANCTL_FAN_DD_READ,
		.helpargs = "read"
	},
	{
		.cmd = "write",
		.id = EMCFANCTL_FAN_DD_WRITE,
		.helpargs = "write"
	}
};

static const struct emcfanctlcmd emcfanctlpcmds[] = {
	{
		.cmd = "read",
		.id = EMCFANCTL_FAN_P_READ,
		.helpargs = "read"
	},
	{
		.cmd = "inverted",
		.id = EMCFANCTL_FAN_P_INVERTED,
		.helpargs = "inverted"
	},
	{
		.cmd = "non-inverted",
		.id = EMCFANCTL_FAN_P_NONINVERTED,
		.helpargs = "non-inverted"
	}
};

static const struct emcfanctlcmd emcfanctlotcmds[] = {
	{
		.cmd = "read",
		.id = EMCFANCTL_FAN_OT_READ,
		.helpargs = "read"
	},
	{
		.cmd = "push-pull",
		.id = EMCFANCTL_FAN_OT_PUSHPULL,
		.helpargs = "push-pull"
	},
	{
		.cmd = "open-drain",
		.id = EMCFANCTL_FAN_OT_OPENDRAIN,
		.helpargs = "open-drain"
	}
};

static const struct emcfanctlcmd emcfanctlapdsmtocmds[] = {
	{
		.cmd = "read",
		.id = EMCFANCTL_APD_READ,
		.helpargs = "read"
	},
	{
		.cmd = "on",
		.id = EMCFANCTL_APD_ON,
		.helpargs = "on"
	},
	{
		.cmd = "off",
		.id = EMCFANCTL_APD_OFF,
		.helpargs = "off"
	}
};

/* Name the registers for each of the chip types */

static const struct emcfan_registers emcfanctl_2101_registers[] = {
	{
		.name = "internal_temperature",
		.reg = 0x00
	},
	{
		.name = "external_diode_temperature_high_byte",
		.reg = 0x01
	},
	{
		.name = "status",
		.reg = 0x02
	},
	{
		.name = "configuration",
		.reg = 0x03
	},
	{
		.name = "conversion_rate",
		.reg = 0x04
	},
	{
		.name = "internal_temp_limit",
		.reg = 0x05
	},
	{
		.name = "external_temp_high_limit_high_byte",
		.reg = 0x07
	},
	{
		.name = "external_temp_low_limit_high_byte",
		.reg = 0x08
	},
	{
		.name = "configuration_alt",
		.reg = 0x09
	},
	{
		.name = "conversion_rate_alt",
		.reg = 0x0A
	},
	{
		.name = "internal_temp_limit_alt",
		.reg = 0x0B
	},
	{
		.name = "external_temp_high_limit_high_byte_alt",
		.reg = 0x0D
	},
	{
		.name = "external_temp_low_limit_high_byte_alt",
		.reg = 0x0E
	},
	{
		.name = "external_temperature_force",
		.reg = 0x0C
	},
	{
		.name = "one_shot",
		.reg = 0x0F
	},
	{
		.name = "external_diode_temperature_low_byte",
		.reg = 0x10
	},
	{
		.name = "scratchpad1",
		.reg = 0x11
	},
	{
		.name = "scratchpad2",
		.reg = 0x12
	},
	{
		.name = "external_diode_high_limit_low_byte",
		.reg = 0x13
	},
	{
		.name = "external_diode_low_limit_low_byte",
		.reg = 0x14
	},
	{
		.name = "alert_mask",
		.reg = 0x16
	},
	{
		.name = "external_diode_ideality_factor",
		.reg = 0x17
	},
	{
		.name = "beta_compensation_factor",
		.reg = 0x18
	},
	{
		.name = "tcrit_temp_limit",
		.reg = 0x19
	},
	{
		.name = "tcrit_hysteresis",
		.reg = 0x21
	},
	{
		.name = "tach_reading_low_byte",
		.reg = 0x46
	},
	{
		.name = "tach_reading_high_byte",
		.reg = 0x47
	},
	{
		.name = "tach_limit_low_byte",
		.reg = 0x48
	},
	{
		.name = "tach_limit_high_byte",
		.reg = 0x49
	},
	{
		.name = "fan_configuration",
		.reg = 0x4A
	},
	{
		.name = "fan_spin-up",
		.reg = 0x4B
	},
	{
		.name = "fan_setting",
		.reg = 0x4C
	},
	{
		.name = "pwm_frequency",
		.reg = 0x4D
	},
	{
		.name = "pwm_frequency_divide",
		.reg = 0x4E
	},
	{
		.name = "lookup_table_hysteresis",
		.reg = 0x4F
	},
	{
		.name = "lookup_table_temp_setting_1",
		.reg = 0x50
	},
	{
		.name = "lookup_table_fan_setting_1",
		.reg = 0x51
	},
	{
		.name = "lookup_table_temp_setting_2",
		.reg = 0x52
	},
	{
		.name = "lookup_table_fan_setting_2",
		.reg = 0x53
	},
	{
		.name = "lookup_table_temp_setting_3",
		.reg = 0x54
	},
	{
		.name = "lookup_table_fan_setting_3",
		.reg = 0x55
	},
	{
		.name = "lookup_table_temp_setting_4",
		.reg = 0x56
	},
	{
		.name = "lookup_table_fan_setting_4",
		.reg = 0x57
	},
	{
		.name = "lookup_table_temp_setting_5",
		.reg = 0x58
	},
	{
		.name = "lookup_table_fan_setting_5",
		.reg = 0x59
	},
	{
		.name = "lookup_table_temp_setting_6",
		.reg = 0x5A
	},
	{
		.name = "lookup_table_fan_setting_6",
		.reg = 0x5B
	},
	{
		.name = "lookup_table_temp_setting_7",
		.reg = 0x5C
	},
	{
		.name = "lookup_table_fan_setting_7",
		.reg = 0x5D
	},
	{
		.name = "lookup_table_temp_setting_8",
		.reg = 0x5E
	},
	{
		.name = "lookup_table_fan_setting_8",
		.reg = 0x5F
	},
	{
		.name = "averaging_filter",
		.reg = 0xBF
	},
	{
		.name = "product_id",
		.reg = 0xFD
	},
	{
		.name = "manufacturer_id",
		.reg = 0xFE
	},
	{
		.name = "revision_register",
		.reg = 0xFF
	}
};

static const struct emcfan_registers emcfanctl_2103_1_registers[] = {
	{
		.name = "internal_temp_reading_high_byte",
		.reg = 0x00
	},
	{
		.name = "internal_temp_reading_low_byte",
		.reg = 0x01
	},
	{
		.name = "external_diode_1_temp_reading_high_byte",
		.reg = 0x02
	},
	{
		.name = "external_diode_1_temp_reading_low_byte",
		.reg = 0x03
	},
	{
		.name = "critical/thermal_shutdown_temperature",
		.reg = 0x0A
	},
	{
		.name = "pushed_temperature_1",
		.reg = 0x0C
	},
	{
		.name = "pushed_temperature_2",
		.reg = 0x0D
	},
	{
		.name = "trip_set_voltage",
		.reg = 0x10
	},
	{
		.name = "external_diode_1_ideality_register",
		.reg = 0x11
	},
	{
		.name = "external_diode_1_beta_configuration",
		.reg = 0x14
	},
	{
		.name = "external_diode_rec_configuration",
		.reg = 0x17
	},
	{
		.name = "external_diode_1_tcrit_limit",
		.reg = 0x19
	},
	{
		.name = "internal_diode_tcrit_limit",
		.reg = 0x1D
	},
	{
		.name = "tcrit_status",
		.reg = 0x1F
	},
	{
		.name = "configuration",
		.reg = 0x20
	},
	{
		.name = "configuration_2",
		.reg = 0x21
	},
	{
		.name = "interrupt_status",
		.reg = 0x23
	},
	{
		.name = "high_limit_status",
		.reg = 0x24
	},
	{
		.name = "low_limit_status",
		.reg = 0x25
	},
	{
		.name = "diode_fault",
		.reg = 0x26
	},
	{
		.name = "fan_status",
		.reg = 0x27
	},
	{
		.name = "interrupt_enable_register",
		.reg = 0x28
	},
	{
		.name = "fan_interrupt_enable_register",
		.reg = 0x29
	},
	{
		.name = "pwm_config",
		.reg = 0x2A
	},
	{
		.name = "pwm_base_frequency",
		.reg = 0x2B
	},
	{
		.name = "external_diode_1_temp_high_limit",
		.reg = 0x30
	},
	{
		.name = "internal_diode_high_limit",
		.reg = 0x34
	},
	{
		.name = "external_diode_1_temp_low_limit",
		.reg = 0x38
	},
	{
		.name = "internal_diode_low_limit",
		.reg = 0x3C
	},
	{
		.name = "fan_setting",
		.reg = 0x40
	},
	{
		.name = "pwm_divide",
		.reg = 0x41
	},
	{
		.name = "fan_configuration_1",
		.reg = 0x42
	},
	{
		.name = "fan_configuration_2",
		.reg = 0x43
	},
	{
		.name = "gain",
		.reg = 0x45
	},
	{
		.name = "fan_spin_up_configuration",
		.reg = 0x46
	},
	{
		.name = "fan_step",
		.reg = 0x47
	},
	{
		.name = "fan_minimum_drive",
		.reg = 0x48
	},
	{
		.name = "fan_valid_tach_count",
		.reg = 0x49
	},
	{
		.name = "fan_drive_fail_band_low_byte",
		.reg = 0x4A
	},
	{
		.name = "fan_drive_fail_band_high_byte",
		.reg = 0x4B
	},
	{
		.name = "tach_target_low_byte",
		.reg = 0x4C
	},
	{
		.name = "tach_target_high_byte",
		.reg = 0x4D
	},
	{
		.name = "tach_reading_high_byte",
		.reg = 0x4E
	},
	{
		.name = "tach_reading_low_byte",
		.reg = 0x4F
	},
	{
		.name = "lut_configuration",
		.reg = 0x50
	},
	{
		.name = "lut_drive_1",
		.reg = 0x51
	},
	{
		.name = "lut_temp_1_setting_1",
		.reg = 0x52
	},
	{
		.name = "lut_temp_2_setting_1",
		.reg = 0x53
	},
	{
		.name = "lut_temp_3_setting_1",
		.reg = 0x54
	},
	{
		.name = "lut_temp_4_setting_1",
		.reg = 0x55
	},
	{
		.name = "lut_drive_2",
		.reg = 0x56
	},
	{
		.name = "lut_temp_1_setting_2",
		.reg = 0x57
	},
	{
		.name = "lut_temp_2_setting_2",
		.reg = 0x58
	},
	{
		.name = "lut_temp_3_setting_2",
		.reg = 0x59
	},
	{
		.name = "lut_temp_4_setting_2",
		.reg = 0x5A
	},
	{
		.name = "lut_drive_3",
		.reg = 0x5B
	},
	{
		.name = "lut_temp_1_setting_3",
		.reg = 0x5C
	},
	{
		.name = "lut_temp_2_setting_3",
		.reg = 0x5D
	},
	{
		.name = "lut_temp_3_setting_3",
		.reg = 0x5E
	},
	{
		.name = "lut_temp_4_setting_3",
		.reg = 0x5F
	},
	{
		.name = "lut_drive_4",
		.reg = 0x60
	},
	{
		.name = "lut_temp_1_setting_4",
		.reg = 0x61
	},
	{
		.name = "lut_temp_2_setting_4",
		.reg = 0x62
	},
	{
		.name = "lut_temp_3_setting_4",
		.reg = 0x63
	},
	{
		.name = "lut_temp_4_setting_4",
		.reg = 0x64
	},
	{
		.name = "lut_drive_5",
		.reg = 0x65
	},
	{
		.name = "lut_temp_1_setting_5",
		.reg = 0x66
	},
	{
		.name = "lut_temp_2_setting_5",
		.reg = 0x67
	},
	{
		.name = "lut_temp_3_setting_5",
		.reg = 0x68
	},
	{
		.name = "lut_temp_4_setting_5",
		.reg = 0x69
	},
	{
		.name = "lut_drive_6",
		.reg = 0x6A
	},
	{
		.name = "lut_temp_1_setting_6",
		.reg = 0x6B
	},
	{
		.name = "lut_temp_2_setting_6",
		.reg = 0x6C
	},
	{
		.name = "lut_temp_3_setting_6",
		.reg = 0x6D
	},
	{
		.name = "lut_temp_4_setting_6",
		.reg = 0x6E
	},
	{
		.name = "lut_drive_7",
		.reg = 0x6F
	},
	{
		.name = "lut_temp_1_setting_7",
		.reg = 0x70
	},
	{
		.name = "lut_temp_2_setting_7",
		.reg = 0x71
	},
	{
		.name = "lut_temp_3_setting_7",
		.reg = 0x72
	},
	{
		.name = "lut_temp_4_setting_7",
		.reg = 0x73
	},
	{
		.name = "lut_drive_8",
		.reg = 0x74
	},
	{
		.name = "lut_temp_1_setting_8",
		.reg = 0x75
	},
	{
		.name = "lut_temp_2_setting_8",
		.reg = 0x76
	},
	{
		.name = "lut_temp_3_setting_8",
		.reg = 0x77
	},
	{
		.name = "lut_temp_4_setting_8",
		.reg = 0x78
	},
	{
		.name = "lut_temp_hysteresis",
		.reg = 0x79
	},
	{
		.name = "software_lock",
		.reg = 0xEF
	},
	{
		.name = "product_features",
		.reg = 0xFC
	},
	{
		.name = "product_id",
		.reg = 0xFD
	},
	{
		.name = "manufacturer_id",
		.reg = 0xFE
	},
	{
		.name = "revision",
		.reg = 0xFF
	}
};

static const struct emcfan_registers emcfanctl_2103_24_registers[] = {
	{
		.name = "internal_temp_reading_high_byte",
		.reg = 0x00
	},
	{
		.name = "internal_temp_reading_low_byte",
		.reg = 0x01
	},
	{
		.name = "external_diode_1_temp_reading_high_byte",
		.reg = 0x02
	},
	{
		.name = "external_diode_1_temp_reading_low_byte",
		.reg = 0x03
	},
	{
		.name = "external_diode_2_temp_reading_high_byte",
		.reg = 0x04
	},
	{
		.name = "external_diode_2_temp_reading_low_byte",
		.reg = 0x05
	},
	{
		.name = "external_diode_3_temp_reading_high_byte",
		.reg = 0x06
	},
	{
		.name = "external_diode_3_temp_reading_low_byte",
		.reg = 0x07
	},
	{
		.name = "critical/thermal_shutdown_temperature",
		.reg = 0x0A
	},
	{
		.name = "pushed_temperature_1",
		.reg = 0x0C
	},
	{
		.name = "pushed_temperature_2",
		.reg = 0x0D
	},
	{
		.name = "trip_set_voltage",
		.reg = 0x10
	},
	{
		.name = "external_diode_1_ideality_register",
		.reg = 0x11
	},
	{
		.name = "external_diode_2_ideality_register",
		.reg = 0x12
	},
	{
		.name = "external_diode_1_beta_configuration",
		.reg = 0x14
	},
	{
		.name = "external_diode_2_beta_configuration",
		.reg = 0x15
	},
	{
		.name = "external_diode_rec_configuration",
		.reg = 0x17
	},
	{
		.name = "external_diode_1_tcrit_limit",
		.reg = 0x19
	},
	{
		.name = "external_diode_2_tcrit_limit",
		.reg = 0x1A
	},
	{
		.name = "external_diode_3_tcrit_limit",
		.reg = 0x1B
	},
	{
		.name = "internal_diode_tcrit_limit",
		.reg = 0x1D
	},
	{
		.name = "tcrit_status",
		.reg = 0x1F
	},
	{
		.name = "configuration",
		.reg = 0x20
	},
	{
		.name = "configuration_2",
		.reg = 0x21
	},
	{
		.name = "interrupt_status",
		.reg = 0x23
	},
	{
		.name = "high_limit_status",
		.reg = 0x24
	},
	{
		.name = "low_limit_status",
		.reg = 0x25
	},
	{
		.name = "diode_fault",
		.reg = 0x26
	},
	{
		.name = "fan_status",
		.reg = 0x27
	},
	{
		.name = "interrupt_enable_register",
		.reg = 0x28
	},
	{
		.name = "fan_interrupt_enable_register",
		.reg = 0x29
	},
	{
		.name = "pwm_config",
		.reg = 0x2A
	},
	{
		.name = "pwm_base_frequency",
		.reg = 0x2B
	},
	{
		.name = "external_diode_1_temp_high_limit",
		.reg = 0x30
	},
	{
		.name = "external_diode_2_temp_high_limit",
		.reg = 0x31
	},
	{
		.name = "external_diode_3_temp_high_limit",
		.reg = 0x32
	},
	{
		.name = "internal_diode_high_limit",
		.reg = 0x34
	},
	{
		.name = "external_diode_1_temp_low_limit",
		.reg = 0x38
	},
	{
		.name = "external_diode_2_temp_low_limit",
		.reg = 0x39
	},
	{
		.name = "external_diode_3_temp_low_limit",
		.reg = 0x3A
	},
	{
		.name = "internal_diode_low_limit",
		.reg = 0x3C
	},
	{
		.name = "fan_setting",
		.reg = 0x40
	},
	{
		.name = "pwm_divide",
		.reg = 0x41
	},
	{
		.name = "fan_configuration_1",
		.reg = 0x42
	},
	{
		.name = "fan_configuration_2",
		.reg = 0x43
	},
	{
		.name = "gain",
		.reg = 0x45
	},
	{
		.name = "fan_spin_up_configuration",
		.reg = 0x46
	},
	{
		.name = "fan_step",
		.reg = 0x47
	},
	{
		.name = "fan_minimum_drive",
		.reg = 0x48
	},
	{
		.name = "fan_valid_tach_count",
		.reg = 0x49
	},
	{
		.name = "fan_drive_fail_band_low_byte",
		.reg = 0x4A
	},
	{
		.name = "fan_drive_fail_band_high_byte",
		.reg = 0x4B
	},
	{
		.name = "tach_target_low_byte",
		.reg = 0x4C
	},
	{
		.name = "tach_target_high_byte",
		.reg = 0x4D
	},
	{
		.name = "tach_reading_high_byte",
		.reg = 0x4E
	},
	{
		.name = "tach_reading_low_byte",
		.reg = 0x4F
	},
	{
		.name = "lut_configuration",
		.reg = 0x50
	},
	{
		.name = "lut_drive_1",
		.reg = 0x51
	},
	{
		.name = "lut_temp_1_setting_1",
		.reg = 0x52
	},
	{
		.name = "lut_temp_2_setting_1",
		.reg = 0x53
	},
	{
		.name = "lut_temp_3_setting_1",
		.reg = 0x54
	},
	{
		.name = "lut_temp_4_setting_1",
		.reg = 0x55
	},
	{
		.name = "lut_drive_2",
		.reg = 0x56
	},
	{
		.name = "lut_temp_1_setting_2",
		.reg = 0x57
	},
	{
		.name = "lut_temp_2_setting_2",
		.reg = 0x58
	},
	{
		.name = "lut_temp_3_setting_2",
		.reg = 0x59
	},
	{
		.name = "lut_temp_4_setting_2",
		.reg = 0x5A
	},
	{
		.name = "lut_drive_3",
		.reg = 0x5B
	},
	{
		.name = "lut_temp_1_setting_3",
		.reg = 0x5C
	},
	{
		.name = "lut_temp_2_setting_3",
		.reg = 0x5D
	},
	{
		.name = "lut_temp_3_setting_3",
		.reg = 0x5E
	},
	{
		.name = "lut_temp_4_setting_3",
		.reg = 0x5F
	},
	{
		.name = "lut_drive_4",
		.reg = 0x60
	},
	{
		.name = "lut_temp_1_setting_4",
		.reg = 0x61
	},
	{
		.name = "lut_temp_2_setting_4",
		.reg = 0x62
	},
	{
		.name = "lut_temp_3_setting_4",
		.reg = 0x63
	},
	{
		.name = "lut_temp_4_setting_4",
		.reg = 0x64
	},
	{
		.name = "lut_drive_5",
		.reg = 0x65
	},
	{
		.name = "lut_temp_1_setting_5",
		.reg = 0x66
	},
	{
		.name = "lut_temp_2_setting_5",
		.reg = 0x67
	},
	{
		.name = "lut_temp_3_setting_5",
		.reg = 0x68
	},
	{
		.name = "lut_temp_4_setting_5",
		.reg = 0x69
	},
	{
		.name = "lut_drive_6",
		.reg = 0x6A
	},
	{
		.name = "lut_temp_1_setting_6",
		.reg = 0x6B
	},
	{
		.name = "lut_temp_2_setting_6",
		.reg = 0x6C
	},
	{
		.name = "lut_temp_3_setting_6",
		.reg = 0x6D
	},
	{
		.name = "lut_temp_4_setting_6",
		.reg = 0x6E
	},
	{
		.name = "lut_drive_7",
		.reg = 0x6F
	},
	{
		.name = "lut_temp_1_setting_7",
		.reg = 0x70
	},
	{
		.name = "lut_temp_2_setting_7",
		.reg = 0x71
	},
	{
		.name = "lut_temp_3_setting_7",
		.reg = 0x72
	},
	{
		.name = "lut_temp_4_setting_7",
		.reg = 0x73
	},
	{
		.name = "lut_drive_8",
		.reg = 0x74
	},
	{
		.name = "lut_temp_1_setting_8",
		.reg = 0x75
	},
	{
		.name = "lut_temp_2_setting_8",
		.reg = 0x76
	},
	{
		.name = "lut_temp_3_setting_8",
		.reg = 0x77
	},
	{
		.name = "lut_temp_4_setting_8",
		.reg = 0x78
	},
	{
		.name = "lut_temp_hysteresis",
		.reg = 0x79
	},
	{
		.name = "gpio_direction_register",
		.reg = 0xE1
	},
	{
		.name = "gpio_output_configuration_register",
		.reg = 0xE2
	},
	{
		.name = "gpio_input_register",
		.reg = 0xE3
	},
	{
		.name = "gpio_output_register",
		.reg = 0xE4
	},
	{
		.name = "gpio_interrupt_enable_register",
		.reg = 0xE5
	},
	{
		.name = "gpio_status",
		.reg = 0xE6
	},
	{
		.name = "software_lock",
		.reg = 0xEF
	},
	{
		.name = "product_features",
		.reg = 0xFC
	},
	{
		.name = "product_id",
		.reg = 0xFD
	},
	{
		.name = "manufacturer_id",
		.reg = 0xFE
	},
	{
		.name = "revision",
		.reg = 0xFF
	}
};

static const struct emcfan_registers emcfanctl_2104_registers[] = {
	{
		.name = "internal_temp_reading_high_byte",
		.reg = 0x00
	},
	{
		.name = "internal_temp_reading_low_byte",
		.reg = 0x01
	},
	{
		.name = "external_diode_1_temp_reading_high_byte",
		.reg = 0x02
	},
	{
		.name = "external_diode_1_temp_reading_low_byte",
		.reg = 0x03
	},
	{
		.name = "external_diode_2_temp_reading_high_byte",
		.reg = 0x04
	},
	{
		.name = "external_diode_2_temp_reading_low_byte",
		.reg = 0x05
	},
	{
		.name = "external_diode_3_temp_reading_high_byte",
		.reg = 0x06
	},
	{
		.name = "external_diode_3_temp_reading_low_byte",
		.reg = 0x07
	},
	{
		.name = "external_diode_4_temp_reading_high_byte",
		.reg = 0x08
	},
	{
		.name = "external_diode_4_temp_reading_low_byte",
		.reg = 0x09
	},
	{
		.name = "critical/thermal_shutdown_temperature",
		.reg = 0x0A
	},
	{
		.name = "pushed_temperature_1",
		.reg = 0x0C
	},
	{
		.name = "pushed_temperature_2",
		.reg = 0x0D
	},
	{
		.name = "pushed_temperature_3",
		.reg = 0x0E
	},
	{
		.name = "pushed_temperature_4",
		.reg = 0x0F
	},
	{
		.name = "trip_set_voltage",
		.reg = 0x10
	},
	{
		.name = "external_diode_1_beta_configuration",
		.reg = 0x14
	},
	{
		.name = "external_diode_2_beta_configuration",
		.reg = 0x15
	},
	{
		.name = "external_diode_3_beta_configuration",
		.reg = 0x16
	},
	{
		.name = "external_diode_rec_configuration",
		.reg = 0x17
	},
	{
		.name = "external_diode_1_tcrit_limit",
		.reg = 0x19
	},
	{
		.name = "external_diode_2_tcrit_limit",
		.reg = 0x1A
	},
	{
		.name = "external_diode_3_tcrit_limit",
		.reg = 0x1B
	},
	{
		.name = "external_diode_4_tcrit_limit",
		.reg = 0x1C
	},
	{
		.name = "internal_diode_tcrit_limit",
		.reg = 0x1D
	},
	{
		.name = "tcrit_limit_status",
		.reg = 0x1F
	},
	{
		.name = "configuration",
		.reg = 0x20
	},
	{
		.name = "configuration_2",
		.reg = 0x21
	},
	{
		.name = "configuration_3",
		.reg = 0x22
	},
	{
		.name = "interrupt_status",
		.reg = 0x23
	},
	{
		.name = "high_limit_status",
		.reg = 0x24
	},
	{
		.name = "low_limit_status",
		.reg = 0x25
	},
	{
		.name = "diode_fault",
		.reg = 0x26
	},
	{
		.name = "fan_status",
		.reg = 0x27
	},
	{
		.name = "interrupt_enable_register",
		.reg = 0x28
	},
	{
		.name = "fan_interrupt_enable_register",
		.reg = 0x29
	},
	{
		.name = "pwm_config",
		.reg = 0x2A
	},
	{
		.name = "pwm_base_frequency",
		.reg = 0x2B
	},
	{
		.name = "external_diode_1_temp_high_limit",
		.reg = 0x30
	},
	{
		.name = "external_diode_2_temp_high_limit",
		.reg = 0x31
	},
	{
		.name = "external_diode_3_temp_high_limit",
		.reg = 0x32
	},
	{
		.name = "external_diode_4_temp_high_limit",
		.reg = 0x33
	},
	{
		.name = "internal_diode_high_limit",
		.reg = 0x34
	},
	{
		.name = "voltage_4_high_limit",
		.reg = 0x35
	},
	{
		.name = "external_diode_1_temp_low_limit",
		.reg = 0x38
	},
	{
		.name = "external_diode_2_temp_low_limit",
		.reg = 0x39
	},
	{
		.name = "external_diode_3_temp_low_limit",
		.reg = 0x3A
	},
	{
		.name = "external_diode_4_temp_low_limit",
		.reg = 0x3B
	},
	{
		.name = "internal_diode_low_limit",
		.reg = 0x3C
	},
	{
		.name = "voltage_4_low_limit",
		.reg = 0x3D
	},
	{
		.name = "fan_1_setting",
		.reg = 0x40
	},
	{
		.name = "pwm_1_divide",
		.reg = 0x41
	},
	{
		.name = "fan_1_configuration_1",
		.reg = 0x42
	},
	{
		.name = "fan_1_configuration_2",
		.reg = 0x43
	},
	{
		.name = "gain_1",
		.reg = 0x45
	},
	{
		.name = "fan_1_spin_up_configuration",
		.reg = 0x46
	},
	{
		.name = "fan_1_step",
		.reg = 0x47
	},
	{
		.name = "fan_1_minimum_drive",
		.reg = 0x48
	},
	{
		.name = "fan_1_valid_tach_count",
		.reg = 0x49
	},
	{
		.name = "fan_1_drive_fail_band_low_byte",
		.reg = 0x4A
	},
	{
		.name = "fan_1_drive_fail_band_high_byte",
		.reg = 0x4B
	},
	{
		.name = "tach_1_target_low_byte",
		.reg = 0x4C
	},
	{
		.name = "tach_1_target_high_byte",
		.reg = 0x4D
	},
	{
		.name = "tach_1_readinghigh_byte",
		.reg = 0x4E
	},
	{
		.name = "tach_1_reading_low_byte",
		.reg = 0x4F
	},
	{
		.name = "lut_1_configuration",
		.reg = 0x50
	},
	{
		.name = "lut_1_drive_1",
		.reg = 0x51
	},
	{
		.name = "lut_1_temp_1_setting_1",
		.reg = 0x52
	},
	{
		.name = "lut_1_temp_2_setting_1",
		.reg = 0x53
	},
	{
		.name = "lut_1_temp_3_setting_1",
		.reg = 0x54
	},
	{
		.name = "lut_1_temp_4_setting_1",
		.reg = 0x55
	},
	{
		.name = "lut_1_drive_2",
		.reg = 0x56
	},
	{
		.name = "lut_1_temp_1_setting_2",
		.reg = 0x57
	},
	{
		.name = "lut_1_temp_2_setting_2",
		.reg = 0x58
	},
	{
		.name = "lut_1_temp_3_setting_2",
		.reg = 0x59
	},
	{
		.name = "lut_1_temp_4_setting_2",
		.reg = 0x5A
	},
	{
		.name = "lut_1_drive_3",
		.reg = 0x5B
	},
	{
		.name = "lut_1_temp_1_setting_3",
		.reg = 0x5C
	},
	{
		.name = "lut_1_temp_2_setting_3",
		.reg = 0x5D
	},
	{
		.name = "lut_1_temp_3_setting_3",
		.reg = 0x5E
	},
	{
		.name = "lut_1_temp_4_setting_3",
		.reg = 0x5F
	},
	{
		.name = "lut_1_drive_4",
		.reg = 0x60
	},
	{
		.name = "lut_1_temp_1_setting_4",
		.reg = 0x61
	},
	{
		.name = "lut_1_temp_2_setting_4",
		.reg = 0x62
	},
	{
		.name = "lut_1_temp_3_setting_4",
		.reg = 0x63
	},
	{
		.name = "lut_1_temp_4_setting_4",
		.reg = 0x64
	},
	{
		.name = "lut_1_drive_5",
		.reg = 0x65
	},
	{
		.name = "lut_1_temp_1_setting_5",
		.reg = 0x66
	},
	{
		.name = "lut_1_temp_2_setting_5",
		.reg = 0x67
	},
	{
		.name = "lut_1_temp_3_setting_5",
		.reg = 0x68
	},
	{
		.name = "lut_1_temp_4_setting_5",
		.reg = 0x69
	},
	{
		.name = "lut_1_drive_6",
		.reg = 0x6A
	},
	{
		.name = "lut_1_temp_1_setting_6",
		.reg = 0x6B
	},
	{
		.name = "lut_1_temp_2_setting_6",
		.reg = 0x6C
	},
	{
		.name = "lut_1_temp_3_setting_6",
		.reg = 0x6D
	},
	{
		.name = "lut_1_temp_4_setting_6",
		.reg = 0x6E
	},
	{
		.name = "lut_1_drive_7",
		.reg = 0x6F
	},
	{
		.name = "lut_1_temp_1_setting_7",
		.reg = 0x70
	},
	{
		.name = "lut_1_temp_2_setting_7",
		.reg = 0x71
	},
	{
		.name = "lut_1_temp_3_setting_7",
		.reg = 0x72
	},
	{
		.name = "lut_1_temp_4_setting_7",
		.reg = 0x73
	},
	{
		.name = "lut_1_drive_8",
		.reg = 0x74
	},
	{
		.name = "lut_1_temp_1_setting_8",
		.reg = 0x75
	},
	{
		.name = "lut_1_temp_2_setting_8",
		.reg = 0x76
	},
	{
		.name = "lut_1_temp_3_setting_8",
		.reg = 0x77
	},
	{
		.name = "lut_1_temp_4_setting_8",
		.reg = 0x78
	},
	{
		.name = "lut_1_temp_hysteresis",
		.reg = 0x79
	},
	{
		.name = "fan_2_setting",
		.reg = 0x80
	},
	{
		.name = "pwm2_divide",
		.reg = 0x81
	},
	{
		.name = "fan_2_configuration_1",
		.reg = 0x82
	},
	{
		.name = "fan_2_configuration_2",
		.reg = 0x83
	},
	{
		.name = "gain_2",
		.reg = 0x85
	},
	{
		.name = "fan_2_spin_up_configuration",
		.reg = 0x86
	},
	{
		.name = "fan_2_step",
		.reg = 0x87
	},
	{
		.name = "fan_2_minimum_drive",
		.reg = 0x88
	},
	{
		.name = "fan_2_valid_tach_count",
		.reg = 0x89
	},
	{
		.name = "fan_2_drive_fail_band_low_byte",
		.reg = 0x8A
	},
	{
		.name = "fan_2_drive_fail_band_high_byte",
		.reg = 0x8B
	},
	{
		.name = "tach_2_target_low_byte",
		.reg = 0x8C
	},
	{
		.name = "tach_2_target_high_byte",
		.reg = 0x8D
	},
	{
		.name = "tach_2_reading_high_byte",
		.reg = 0x8E
	},
	{
		.name = "tach_2_reading_low_byte",
		.reg = 0x8F
	},
	{
		.name = "lut_2_configuration",
		.reg = 0x90
	},
	{
		.name = "lut_2_drive_1",
		.reg = 0x91
	},
	{
		.name = "lut_2_temp_1_setting_1",
		.reg = 0x92
	},
	{
		.name = "lut_2_temp_2_setting_1",
		.reg = 0x93
	},
	{
		.name = "lut_2_temp_3_setting_1",
		.reg = 0x94
	},
	{
		.name = "lut_2_temp_4_setting_1",
		.reg = 0x95
	},
	{
		.name = "lut_2_drive_2",
		.reg = 0x96
	},
	{
		.name = "lut_2_temp_1_setting_2",
		.reg = 0x97
	},
	{
		.name = "lut_2_temp_2_setting_2",
		.reg = 0x98
	},
	{
		.name = "lut_2_temp_3_setting_2",
		.reg = 0x99
	},
	{
		.name = "lut_2_temp_4_setting_2",
		.reg = 0x9A
	},
	{
		.name = "lut_2_drive_3",
		.reg = 0x9B
	},
	{
		.name = "lut_2_temp_1_setting_3",
		.reg = 0x9C
	},
	{
		.name = "lut_2_temp_2_setting_3",
		.reg = 0x9D
	},
	{
		.name = "lut_2_temp_3_setting_3",
		.reg = 0x9E
	},
	{
		.name = "lut_2_temp_4_setting_3",
		.reg = 0x9F
	},
	{
		.name = "lut_2_drive_4",
		.reg = 0xA0
	},
	{
		.name = "lut_2_temp_1_setting_4",
		.reg = 0xA1
	},
	{
		.name = "lut_2_temp_2_setting_4",
		.reg = 0xA2
	},
	{
		.name = "lut_2_temp_3_setting_4",
		.reg = 0xA3
	},
	{
		.name = "lut_2_temp_4_setting_4",
		.reg = 0xA4
	},
	{
		.name = "lut_2_drive_5",
		.reg = 0xA5
	},
	{
		.name = "lut_2_temp_1_setting_5",
		.reg = 0xA6
	},
	{
		.name = "lut_2_temp_2_setting_5",
		.reg = 0xA7
	},
	{
		.name = "lut_2_temp_3_setting_5",
		.reg = 0xA8
	},
	{
		.name = "lut_2_temp_4_setting_5",
		.reg = 0xA9
	},
	{
		.name = "lut_2_drive_6",
		.reg = 0xAA
	},
	{
		.name = "lut_2_temp_1_setting_6",
		.reg = 0xAB
	},
	{
		.name = "lut_2_temp_2_setting_6",
		.reg = 0xAC
	},
	{
		.name = "lut_2_temp_3_setting_6",
		.reg = 0xAD
	},
	{
		.name = "lut_2_temp_4_setting_6",
		.reg = 0xAE
	},
	{
		.name = "lut_2_drive_7",
		.reg = 0xAF
	},
	{
		.name = "lut_2_temp_1_setting_6",
		.reg = 0xB0
	},
	{
		.name = "lut_2_temp_2_setting_6",
		.reg = 0xB1
	},
	{
		.name = "lut_2_temp_3_setting_6",
		.reg = 0xB2
	},
	{
		.name = "lut_2_temp_4_setting_6",
		.reg = 0xB3
	},
	{
		.name = "lut_2_drive_8",
		.reg = 0xB4
	},
	{
		.name = "lut_2_temp_1_setting_8",
		.reg = 0xB5
	},
	{
		.name = "lut_2_temp_2_setting_8",
		.reg = 0xB6
	},
	{
		.name = "lut_2_temp_3_setting_8",
		.reg = 0xB7
	},
	{
		.name = "lut_2_temp_4_setting_8",
		.reg = 0xB8
	},
	{
		.name = "lut_2_temp_hysteresis",
		.reg = 0xB9
	},
	{
		.name = "muxed_pin_configuration_register",
		.reg = 0xE0
	},
	{
		.name = "gpio_direction_register",
		.reg = 0xE1
	},
	{
		.name = "gpio_output_configuration_register",
		.reg = 0xE2
	},
	{
		.name = "gpio_input_register",
		.reg = 0xE3
	},
	{
		.name = "gpio_output_register",
		.reg = 0xE4
	},
	{
		.name = "gpio_interrupt_enable_register",
		.reg = 0xE5
	},
	{
		.name = "gpio_status",
		.reg = 0xE6
	},
	{
		.name = "software_lock",
		.reg = 0xEF
	},
	{
		.name = "product_features",
		.reg = 0xFC
	},
	{
		.name = "product_id",
		.reg = 0xFD
	},
	{
		.name = "manufacturer_id",
		.reg = 0xFE
	},
	{
		.name = "revision",
		.reg = 0xFF
	}
};

static const struct emcfan_registers emcfanctl_2106_registers[] = {
	{
		.name = "internal_temp_reading_high_byte",
		.reg = 0x00
	},
	{
		.name = "internal_temp_reading_low_byte",
		.reg = 0x01
	},
	{
		.name = "external_diode_1_temp_reading_high_byte",
		.reg = 0x02
	},
	{
		.name = "external_diode_1_temp_reading_low_byte",
		.reg = 0x03
	},
	{
		.name = "external_diode_2_temp_reading_high_byte",
		.reg = 0x04
	},
	{
		.name = "external_diode_2_temp_reading_low_byte",
		.reg = 0x05
	},
	{
		.name = "external_diode_3_temp_reading_high_byte",
		.reg = 0x06
	},
	{
		.name = "external_diode_3_temp_reading_low_byte",
		.reg = 0x07
	},
	{
		.name = "external_diode_4_temp_reading_high_byte",
		.reg = 0x08
	},
	{
		.name = "external_diode_4_temp_reading_low_byte",
		.reg = 0x09
	},
	{
		.name = "critical/thermal_shutdown_temperature",
		.reg = 0x0A
	},
	{
		.name = "pushed_temperature_1",
		.reg = 0x0C
	},
	{
		.name = "pushed_temperature_2",
		.reg = 0x0D
	},
	{
		.name = "pushed_temperature_3",
		.reg = 0x0E
	},
	{
		.name = "pushed_temperature_4",
		.reg = 0x0F
	},
	{
		.name = "trip_set_voltage",
		.reg = 0x10
	},
	{
		.name = "external_diode_1_beta_configuration",
		.reg = 0x14
	},
	{
		.name = "external_diode_2_beta_configuration",
		.reg = 0x15
	},
	{
		.name = "external_diode_3_beta_configuration",
		.reg = 0x16
	},
	{
		.name = "external_diode_rec_configuration",
		.reg = 0x17
	},
	{
		.name = "external_diode_1_tcrit_limit",
		.reg = 0x19
	},
	{
		.name = "external_diode_2_tcrit_limit",
		.reg = 0x1A
	},
	{
		.name = "external_diode_3_tcrit_limit",
		.reg = 0x1B
	},
	{
		.name = "external_diode_4_tcrit_limit",
		.reg = 0x1C
	},
	{
		.name = "internal_diode_tcrit_limit",
		.reg = 0x1D
	},
	{
		.name = "tcrit_limit_status",
		.reg = 0x1F
	},
	{
		.name = "configuration",
		.reg = 0x20
	},
	{
		.name = "configuration_2",
		.reg = 0x21
	},
	{
		.name = "configuration_3",
		.reg = 0x22
	},
	{
		.name = "interrupt_status",
		.reg = 0x23
	},
	{
		.name = "high_limit_status",
		.reg = 0x24
	},
	{
		.name = "low_limit_status",
		.reg = 0x25
	},
	{
		.name = "diode_fault",
		.reg = 0x26
	},
	{
		.name = "fan_status",
		.reg = 0x27
	},
	{
		.name = "interrupt_enable_register",
		.reg = 0x28
	},
	{
		.name = "fan_interruptenable_register",
		.reg = 0x29
	},
	{
		.name = "pwm_config",
		.reg = 0x2A
	},
	{
		.name = "pwm_base_frequency",
		.reg = 0x2B
	},
	{
		.name = "pwm_3_frequency_divide",
		.reg = 0x2C
	},
	{
		.name = "pwm3_setting",
		.reg = 0x2D
	},
	{
		.name = "pwm4_setting",
		.reg = 0x2E
	},
	{
		.name = "pwm4_frequency_divide",
		.reg = 0x2F
	},
	{
		.name = "external_diode_1_temp_high_limit",
		.reg = 0x30
	},
	{
		.name = "external_diode_2_temp_high_limit",
		.reg = 0x31
	},
	{
		.name = "external_diode_3_temp_high_limit",
		.reg = 0x32
	},
	{
		.name = "external_diode_4_temp_high_limit",
		.reg = 0x33
	},
	{
		.name = "internal_diode_high_limit",
		.reg = 0x34
	},
	{
		.name = "voltage_4_high_limit",
		.reg = 0x35
	},
	{
		.name = "external_diode_1_temp_low_limit",
		.reg = 0x38
	},
	{
		.name = "external_diode_2_temp_low_limit",
		.reg = 0x39
	},
	{
		.name = "external_diode_3_temp_low_limit",
		.reg = 0x3A
	},
	{
		.name = "external_diode_4_temp_low_limit",
		.reg = 0x3B
	},
	{
		.name = "internal_diode_low_limit",
		.reg = 0x3C
	},
	{
		.name = "voltage_4_low_limit",
		.reg = 0x3D
	},
	{
		.name = "fan_1_setting",
		.reg = 0x40
	},
	{
		.name = "pwm_1_divide",
		.reg = 0x41
	},
	{
		.name = "fan_1_configuration_1",
		.reg = 0x42
	},
	{
		.name = "fan_1_configuration_2",
		.reg = 0x43
	},
	{
		.name = "gain_1",
		.reg = 0x45
	},
	{
		.name = "fan_1_spin_up_configuration",
		.reg = 0x46
	},
	{
		.name = "fan_1_step",
		.reg = 0x47
	},
	{
		.name = "fan_1_minimum_drive",
		.reg = 0x48
	},
	{
		.name = "fan_1_valid_tach_count",
		.reg = 0x49
	},
	{
		.name = "fan_1_drive_fail_band_low_byte",
		.reg = 0x4A
	},
	{
		.name = "fan_1_drive_fail_band_high_byte",
		.reg = 0x4B
	},
	{
		.name = "tach_1_target_low_byte",
		.reg = 0x4C
	},
	{
		.name = "tach_1_target_high_byte",
		.reg = 0x4D
	},
	{
		.name = "tach_1_readinghigh_byte",
		.reg = 0x4E
	},
	{
		.name = "tach_1_reading_low_byte",
		.reg = 0x4F
	},
	{
		.name = "lut_1_configuration",
		.reg = 0x50
	},
	{
		.name = "lut_1_drive_1",
		.reg = 0x51
	},
	{
		.name = "lut_1_temp_1_setting_1",
		.reg = 0x52
	},
	{
		.name = "lut_1_temp_2_setting_1",
		.reg = 0x53
	},
	{
		.name = "lut_1_temp_3_setting_1",
		.reg = 0x54
	},
	{
		.name = "lut_1_temp_4_setting_1",
		.reg = 0x55
	},
	{
		.name = "lut_1_drive_2",
		.reg = 0x56
	},
	{
		.name = "lut_1_temp_1_setting_2",
		.reg = 0x57
	},
	{
		.name = "lut_1_temp_2_setting_2",
		.reg = 0x58
	},
	{
		.name = "lut_1_temp_3_setting_2",
		.reg = 0x59
	},
	{
		.name = "lut_1_temp_4_setting_2",
		.reg = 0x5A
	},
	{
		.name = "lut_1_drive_3",
		.reg = 0x5B
	},
	{
		.name = "lut_1_temp_1_setting_3",
		.reg = 0x5C
	},
	{
		.name = "lut_1_temp_2_setting_3",
		.reg = 0x5D
	},
	{
		.name = "lut_1_temp_3_setting_3",
		.reg = 0x5E
	},
	{
		.name = "lut_1_temp_4_setting_3",
		.reg = 0x5F
	},
	{
		.name = "lut_1_drive_4",
		.reg = 0x60
	},
	{
		.name = "lut_1_temp_1_setting_4",
		.reg = 0x61
	},
	{
		.name = "lut_1_temp_2_setting_4",
		.reg = 0x62
	},
	{
		.name = "lut_1_temp_3_setting_4",
		.reg = 0x63
	},
	{
		.name = "lut_1_temp_4_setting_4",
		.reg = 0x64
	},
	{
		.name = "lut_1_drive_5",
		.reg = 0x65
	},
	{
		.name = "lut_1_temp_1_setting_5",
		.reg = 0x66
	},
	{
		.name = "lut_1_temp_2_setting_5",
		.reg = 0x67
	},
	{
		.name = "lut_1_temp_3_setting_5",
		.reg = 0x68
	},
	{
		.name = "lut_1_temp_4_setting_5",
		.reg = 0x69
	},
	{
		.name = "lut_1_drive_6",
		.reg = 0x6A
	},
	{
		.name = "lut_1_temp_1_setting_6",
		.reg = 0x6B
	},
	{
		.name = "lut_1_temp_2_setting_6",
		.reg = 0x6C
	},
	{
		.name = "lut_1_temp_3_setting_6",
		.reg = 0x6D
	},
	{
		.name = "lut_1_temp_4_setting_6",
		.reg = 0x6E
	},
	{
		.name = "lut_1_drive_7",
		.reg = 0x6F
	},
	{
		.name = "lut_1_temp_1_setting_7",
		.reg = 0x70
	},
	{
		.name = "lut_1_temp_2_setting_7",
		.reg = 0x71
	},
	{
		.name = "lut_1_temp_3_setting_7",
		.reg = 0x72
	},
	{
		.name = "lut_1_temp_4_setting_7",
		.reg = 0x73
	},
	{
		.name = "lut_1_drive_8",
		.reg = 0x74
	},
	{
		.name = "lut_1_temp_1_setting_8",
		.reg = 0x75
	},
	{
		.name = "lut_1_temp_2_setting_8",
		.reg = 0x76
	},
	{
		.name = "lut_1_temp_3_setting_8",
		.reg = 0x77
	},
	{
		.name = "lut_1_temp_4_setting_8",
		.reg = 0x78
	},
	{
		.name = "lut_1_temp_hysteresis",
		.reg = 0x79
	},
	{
		.name = "fan_2_setting",
		.reg = 0x80
	},
	{
		.name = "pwm2_divide",
		.reg = 0x81
	},
	{
		.name = "fan_2_configuration_1",
		.reg = 0x82
	},
	{
		.name = "fan_2_configuration_2",
		.reg = 0x83
	},
	{
		.name = "gain_2",
		.reg = 0x85
	},
	{
		.name = "fan_2_spin_up_configuration",
		.reg = 0x86
	},
	{
		.name = "fan_2_step",
		.reg = 0x87
	},
	{
		.name = "fan_2_minimum_drive",
		.reg = 0x88
	},
	{
		.name = "fan_2_valid_tach_count",
		.reg = 0x89
	},
	{
		.name = "fan_2_drive_fail_band_low_byte",
		.reg = 0x8A
	},
	{
		.name = "fan_2_drive_fail_band_high_byte",
		.reg = 0x8B
	},
	{
		.name = "tach_2_targetlow_byte",
		.reg = 0x8C
	},
	{
		.name = "tach_2_target_high_byte",
		.reg = 0x8D
	},
	{
		.name = "tach_2_reading_high_byte",
		.reg = 0x8E
	},
	{
		.name = "tach_2_reading_low_byte",
		.reg = 0x8F
	},
	{
		.name = "lut_2_configuration",
		.reg = 0x90
	},
	{
		.name = "lut_2_drive_1",
		.reg = 0x91
	},
	{
		.name = "lut_2_temp_1_setting_1",
		.reg = 0x92
	},
	{
		.name = "lut_2_temp_2_setting_1",
		.reg = 0x93
	},
	{
		.name = "lut_2_temp_3_setting_1",
		.reg = 0x94
	},
	{
		.name = "lut_2_temp_4_setting_1",
		.reg = 0x95
	},
	{
		.name = "lut_2_drive_2",
		.reg = 0x96
	},
	{
		.name = "lut_2_temp_1_setting_2",
		.reg = 0x97
	},
	{
		.name = "lut_2_temp_2_setting_2",
		.reg = 0x98
	},
	{
		.name = "lut_2_temp_3_setting_2",
		.reg = 0x99
	},
	{
		.name = "lut_2_temp_4_setting_2",
		.reg = 0x9A
	},
	{
		.name = "lut_2_drive_3",
		.reg = 0x9B
	},
	{
		.name = "lut_2_temp_1_setting_3",
		.reg = 0x9C
	},
	{
		.name = "lut_2_temp_2_setting_3",
		.reg = 0x9D
	},
	{
		.name = "lut_2_temp_3_setting_3",
		.reg = 0x9E
	},
	{
		.name = "lut_2_temp_4_setting_3",
		.reg = 0x9F
	},
	{
		.name = "lut_2_drive_4",
		.reg = 0xA0
	},
	{
		.name = "lut_2_temp_1_setting_4",
		.reg = 0xA1
	},
	{
		.name = "lut_2_temp_2_setting_4",
		.reg = 0xA2
	},
	{
		.name = "lut_2_temp_3_setting_4",
		.reg = 0xA3
	},
	{
		.name = "lut_2_temp_4_setting_4",
		.reg = 0xA4
	},
	{
		.name = "lut_2_drive_5",
		.reg = 0xA5
	},
	{
		.name = "lut_2_temp_1_setting_5",
		.reg = 0xA6
	},
	{
		.name = "lut_2_temp_2_setting_5",
		.reg = 0xA7
	},
	{
		.name = "lut_2_temp_3_setting_5",
		.reg = 0xA8
	},
	{
		.name = "lut_2_temp_4_setting_5",
		.reg = 0xA9
	},
	{
		.name = "lut_2_drive_6",
		.reg = 0xAA
	},
	{
		.name = "lut_2_temp_1_setting_6",
		.reg = 0xAB
	},
	{
		.name = "lut_2_temp_2_setting_6",
		.reg = 0xAC
	},
	{
		.name = "lut_2_temp_3_setting_6",
		.reg = 0xAD
	},
	{
		.name = "lut_2_temp_4_setting_6",
		.reg = 0xAE
	},
	{
		.name = "lut_2_drive_7",
		.reg = 0xAF
	},
	{
		.name = "lut_2_temp_1_setting_6",
		.reg = 0xB0
	},
	{
		.name = "lut_2_temp_2_setting_6",
		.reg = 0xB1
	},
	{
		.name = "lut_2_temp_3setting_6",
		.reg = 0xB2
	},
	{
		.name = "lut_2_temp_4_setting_6",
		.reg = 0xB3
	},
	{
		.name = "lut_2_drive_8",
		.reg = 0xB4
	},
	{
		.name = "lut_2_temp_1_setting_8",
		.reg = 0xB5
	},
	{
		.name = "lut_2_temp_2_setting_8",
		.reg = 0xB6
	},
	{
		.name = "lut_2_temp_3_setting_8",
		.reg = 0xB7
	},
	{
		.name = "lut_2_temp_4_setting_8",
		.reg = 0xB8
	},
	{
		.name = "lut_2_temp_hysteresis",
		.reg = 0xB9
	},
	{
		.name = "muxed_pin_configuration_register",
		.reg = 0xE0
	},
	{
		.name = "gpio_direction_register",
		.reg = 0xE1
	},
	{
		.name = "gpio_output_configuration_register",
		.reg = 0xE2
	},
	{
		.name = "gpio_input_register",
		.reg = 0xE3
	},
	{
		.name = "gpio_output_register",
		.reg = 0xE4
	},
	{
		.name = "gpio_interrupt_enable_register",
		.reg = 0xE5
	},
	{
		.name = "gpio_status",
		.reg = 0xE6
	},
	{
		.name = "software_lock",
		.reg = 0xEF
	},
	{
		.name = "product_features",
		.reg = 0xFC
	},
	{
		.name = "product_id",
		.reg = 0xFD
	},
	{
		.name = "manufacturer_id",
		.reg = 0xFE
	},
	{
		.name = "revision",
		.reg = 0xFF
	}
};

static const struct emcfan_registers emcfanctl_230x_registers[] = {
	{
		.name = "configuration",
		.reg = 0x20
	},
	{
		.name = "fan_status",
		.reg = 0x24
	},
	{
		.name = "fan_stall_status",
		.reg = 0x25
	},
	{
		.name = "fan_spin_status",
		.reg = 0x26
	},
	{
		.name = "drive_fail_status",
		.reg = 0x27
	},
	{
		.name = "fan_interrupt_enable_register",
		.reg = 0x29
	},
	{
		.name = "pwm_polarity_config",
		.reg = 0x2A
	},
	{
		.name = "pwm_output_config",
		.reg = 0x2B
	},
	{
		.name = "pwm_basef45",
		.reg = 0x2C
	},
	{
		.name = "pwm_basef123",
		.reg = 0x2D
	},
	{
		.name = "fan_1_setting",
		.reg = 0x30
	},
	{
		.name = "pwm_1_divide",
		.reg = 0x31
	},
	{
		.name = "fan_1_configuration_1",
		.reg = 0x32
	},
	{
		.name = "fan_1_configuration_2",
		.reg = 0x33
	},
	{
		.name = "gain_1",
		.reg = 0x35
	},
	{
		.name = "fan_1_spin_up_configuration",
		.reg = 0x36
	},
	{
		.name = "fan_1_max_step",
		.reg = 0x37
	},
	{
		.name = "fan_1_minimum_drive",
		.reg = 0x38
	},
	{
		.name = "fan_1_valid_tach_count",
		.reg = 0x39
	},
	{
		.name = "fan_1_drive_fail_band_low_byte",
		.reg = 0x3A
	},
	{
		.name = "fan_1_drive_fail_band_high_byte",
		.reg = 0x3B
	},
	{
		.name = "tach_1_target_low_byte",
		.reg = 0x3C
	},
	{
		.name = "tach_1_target_high_byte",
		.reg = 0x3D
	},
	{
		.name = "tach_1_reading_high_byte",
		.reg = 0x3E
	},
	{
		.name = "tach_1_reading_low_byte",
		.reg = 0x3F
	},
	{
		.name = "fan_2_setting",
		.reg = 0x40
	},
	{
		.name = "pwm_2_divide",
		.reg = 0x41
	},
	{
		.name = "fan_2_configuration_1",
		.reg = 0x42
	},
	{
		.name = "fan_2_configuration_2",
		.reg = 0x43
	},
	{
		.name = "gain_2",
		.reg = 0x45
	},
	{
		.name = "fan_2_spin_up_configuration",
		.reg = 0x46
	},
	{
		.name = "fan_2_max_step",
		.reg = 0x47
	},
	{
		.name = "fan_2_minimum_drive",
		.reg = 0x48
	},
	{
		.name = "fan_2_valid_tach_count",
		.reg = 0x49
	},
	{
		.name = "fan_2_drive_fail_band_low_byte",
		.reg = 0x4A
	},
	{
		.name = "fan_2_drive_fail_band_high_byte",
		.reg = 0x4B
	},
	{
		.name = "tach_2_target_low_byte",
		.reg = 0x4C
	},
	{
		.name = "tach_2_target_high_byte",
		.reg = 0x4D
	},
	{
		.name = "tach_2_reading_high_byte",
		.reg = 0x4E
	},
	{
		.name = "tach_2_reading_low_byte",
		.reg = 0x4F
	},
	{
		.name = "fan_3_setting",
		.reg = 0x50
	},
	{
		.name = "pwm_3_divide",
		.reg = 0x51
	},
	{
		.name = "fan_3_configuration_1",
		.reg = 0x52
	},
	{
		.name = "fan_3_configuration_2",
		.reg = 0x53
	},
	{
		.name = "gain_3",
		.reg = 0x55
	},
	{
		.name = "fan_3_spin_up_configuration",
		.reg = 0x56
	},
	{
		.name = "fan_3_max_step",
		.reg = 0x57
	},
	{
		.name = "fan_3_minimum_drive",
		.reg = 0x58
	},
	{
		.name = "fan_3_valid_tach_count",
		.reg = 0x59
	},
	{
		.name = "fan_3_drive_fail_band_low_byte",
		.reg = 0x5A
	},
	{
		.name = "fan_3_drive_fail_band_high_byte",
		.reg = 0x5B
	},
	{
		.name = "tach_3_target_low_byte",
		.reg = 0x5C
	},
	{
		.name = "tach_3_target_high_byte",
		.reg = 0x5D
	},
	{
		.name = "tach_3_reading_high_byte",
		.reg = 0x5E
	},
	{
		.name = "tach_3_reading_low_byte",
		.reg = 0x5F
	},
	{
		.name = "fan_4_setting",
		.reg = 0x60
	},
	{
		.name = "pwm_4_divide",
		.reg = 0x61
	},
	{
		.name = "fan_4_configuration_1",
		.reg = 0x62
	},
	{
		.name = "fan_4_configuration_2",
		.reg = 0x63
	},
	{
		.name = "gain_4",
		.reg = 0x65
	},
	{
		.name = "fan_4_spin_up_configuration",
		.reg = 0x66
	},
	{
		.name = "fan_4_max_step",
		.reg = 0x67
	},
	{
		.name = "fan_4_minimum_drive",
		.reg = 0x68
	},
	{
		.name = "fan_4_valid_tach_count",
		.reg = 0x69
	},
	{
		.name = "fan_4_drive_fail_band_low_byte",
		.reg = 0x6A
	},
	{
		.name = "fan_4_drive_fail_band_high_byte",
		.reg = 0x6B
	},
	{
		.name = "tach_4_target_low_byte",
		.reg = 0x6C
	},
	{
		.name = "tach_4_target_high_byte",
		.reg = 0x6D
	},
	{
		.name = "tach_4_reading_high_byte",
		.reg = 0x6E
	},
	{
		.name = "tach_4_reading_low_byte",
		.reg = 0x6F
	},
	{
		.name = "fan_5_setting",
		.reg = 0x70
	},
	{
		.name = "pwm_5_divide",
		.reg = 0x71
	},
	{
		.name = "fan_5_configuration_1",
		.reg = 0x72
	},
	{
		.name = "fan_5_configuration_2",
		.reg = 0x73
	},
	{
		.name = "gain_5",
		.reg = 0x75
	},
	{
		.name = "fan_5_spin_up_configuration",
		.reg = 0x76
	},
	{
		.name = "fan_5_max_step",
		.reg = 0x77
	},
	{
		.name = "fan_5_minimum_drive",
		.reg = 0x78
	},
	{
		.name = "fan_5_valid_tach_count",
		.reg = 0x79
	},
	{
		.name = "fan_5_drive_fail_band_low_byte",
		.reg = 0x7A
	},
	{
		.name = "fan_5_drive_fail_band_high_byte",
		.reg = 0x7B
	},
	{
		.name = "tach_5_target_low_byte",
		.reg = 0x7C
	},
	{
		.name = "tach_5_target_high_byte",
		.reg = 0x7D
	},
	{
		.name = "tach_5_reading_high_byte",
		.reg = 0x7E
	},
	{
		.name = "tach_5_reading_low_byte",
		.reg = 0x7F
	},
	{
		.name = "software_lock",
		.reg = 0xEF
	},
	{
		.name = "product_features",
		.reg = 0xFC
	},
	{
		.name = "product_id",
		.reg = 0xFD
	},
	{
		.name = "manufacturer_id",
		.reg = 0xFE
	},
	{
		.name = "revision",
		.reg = 0xFF
	}
};

static const struct emcfan_bits_translate fan_polarity[] = {
	{
		.type = EMCFAN_TRANSLATE_STR,
		.clear_mask = 0b00000001,
		.bit_mask = 0b00000001,
		.human_str = "inverted",
		.instance = 0
	},
	{
		.type = EMCFAN_TRANSLATE_STR,
		.clear_mask = 0b00000001,
		.bit_mask = 0b00000000,
		.human_str = "non-inverted",
		.instance = 0
	},
	{
		.type = EMCFAN_TRANSLATE_STR,
		.clear_mask = 0b00000010,
		.bit_mask = 0b00000010,
		.human_str = "inverted",
		.instance = 1
	},
	{
		.type = EMCFAN_TRANSLATE_STR,
		.clear_mask = 0b00000010,
		.bit_mask = 0b00000000,
		.human_str = "non-inverted",
		.instance = 1
	},
	{
		.type = EMCFAN_TRANSLATE_STR,
		.clear_mask = 0b00000100,
		.bit_mask = 0b00000100,
		.human_str = "inverted",
		.instance = 2
	},
	{
		.type = EMCFAN_TRANSLATE_STR,
		.clear_mask = 0b00000100,
		.bit_mask = 0b00000000,
		.human_str = "non-inverted",
		.instance = 2
	},
	{
		.type = EMCFAN_TRANSLATE_STR,
		.clear_mask = 0b00001000,
		.bit_mask = 0b00001000,
		.human_str = "inverted",
		.instance = 3
	},
	{
		.type = EMCFAN_TRANSLATE_STR,
		.clear_mask = 0b00001000,
		.bit_mask = 0b00000000,
		.human_str = "non-inverted",
		.instance = 3
	},
	{
		.type = EMCFAN_TRANSLATE_STR,
		.clear_mask = 0b00010000,
		.bit_mask = 0b00010000,
		.human_str = "inverted",
		.instance = 4
	},
	{
		.type = EMCFAN_TRANSLATE_STR,
		.clear_mask = 0b00010000,
		.bit_mask = 0b00000000,
		.human_str = "non-inverted",
		.instance = 4
	},
	{
		.type = EMCFAN_TRANSLATE_STR,
		.clear_mask = 0b00010000,
		.bit_mask = 0b00010000,
		.human_str = "inverted",
		.instance = 2101
	},
	{
		.type = EMCFAN_TRANSLATE_STR,
		.clear_mask = 0b00010000,
		.bit_mask = 0b00000000,
		.human_str = "non-inverted",
		.instance = 2101
	}
};

static const struct emcfan_bits_translate fan_minexpectedrpm[] = {
	{
		.type = EMCFAN_TRANSLATE_INT,
		.clear_mask = 0b01100000,
		.bit_mask = 0b00000000,
		.human_int = 500
	},
	{
		.type = EMCFAN_TRANSLATE_INT,
		.clear_mask = 0b01100000,
		.bit_mask = 0b00100000,
		.human_int = 1000
	},
	{
		.type = EMCFAN_TRANSLATE_INT,
		.clear_mask = 0b01100000,
		.bit_mask = 0b01000000,
		.human_int = 2000
	},
	{
		.type = EMCFAN_TRANSLATE_INT,
		.clear_mask = 0b01100000,
		.bit_mask = 0b01100000,
		.human_int = 4000
	}
};

static const struct emcfan_bits_translate fan_numedges[] = {
	{
		.type = EMCFAN_TRANSLATE_INT,
		.clear_mask = 0b00011000,
		.bit_mask = 0b00000000,
		.human_int = 3
	},
	{
		.type = EMCFAN_TRANSLATE_INT,
		.clear_mask = 0b00011000,
		.bit_mask = 0b00001000,
		.human_int = 5
	},
	{
		.type = EMCFAN_TRANSLATE_INT,
		.clear_mask = 0b00011000,
		.bit_mask = 0b00010000,
		.human_int = 7
	},
	{
		.type = EMCFAN_TRANSLATE_INT,
		.clear_mask = 0b00011000,
		.bit_mask = 0b00011000,
		.human_int = 9
	}
};

static const struct emcfan_bits_translate fan_pwm_basefreq[] = {
	{
		.type = EMCFAN_TRANSLATE_INT,
		.clear_mask = 0b00000011,
		.bit_mask = 0b00000000,
		.human_int = 26000,
		.instance = 0
	},
	{
		.type = EMCFAN_TRANSLATE_INT,
		.clear_mask = 0b00000011,
		.bit_mask = 0b00000001,
		.human_int = 19531,
		.instance = 0
	},
	{
		.type = EMCFAN_TRANSLATE_INT,
		.clear_mask = 0b00000011,
		.bit_mask = 0b00000010,
		.human_int = 4882,
		.instance = 0
	},
	{
		.type = EMCFAN_TRANSLATE_INT,
		.clear_mask = 0b00000011,
		.bit_mask = 0b00000011,
		.human_int = 2441,
		.instance = 0
	},
	{
		.type = EMCFAN_TRANSLATE_INT,
		.clear_mask = 0b00001100,
		.bit_mask = 0b00000000,
		.human_int = 26000,
		.instance = 1
	},
	{
		.type = EMCFAN_TRANSLATE_INT,
		.clear_mask = 0b00001100,
		.bit_mask = 0b00000100,
		.human_int = 19531,
		.instance = 1
	},
	{
		.type = EMCFAN_TRANSLATE_INT,
		.clear_mask = 0b00001100,
		.bit_mask = 0b00001000,
		.human_int = 4882,
		.instance = 1
	},
	{
		.type = EMCFAN_TRANSLATE_INT,
		.clear_mask = 0b00001100,
		.bit_mask = 0b00001100,
		.human_int = 2441,
		.instance = 1
	},
	{
		.type = EMCFAN_TRANSLATE_INT,
		.clear_mask = 0b00110000,
		.bit_mask = 0b00000000,
		.human_int = 26000,
		.instance = 2
	},
	{
		.type = EMCFAN_TRANSLATE_INT,
		.clear_mask = 0b00110000,
		.bit_mask = 0b00010000,
		.human_int = 19531,
		.instance = 2
	},
	{
		.type = EMCFAN_TRANSLATE_INT,
		.clear_mask = 0b00110000,
		.bit_mask = 0b00100000,
		.human_int = 4882,
		.instance = 2
	},
	{
		.type = EMCFAN_TRANSLATE_INT,
		.clear_mask = 0b00110000,
		.bit_mask = 0b00110000,
		.human_int = 2441,
		.instance = 2
	},

	{
		.type = EMCFAN_TRANSLATE_INT,
		.clear_mask = 0b11000000,
		.bit_mask = 0b00000000,
		.human_int = 26000,
		.instance = 3
	},
	{
		.type = EMCFAN_TRANSLATE_INT,
		.clear_mask = 0b11000000,
		.bit_mask = 0b01000000,
		.human_int = 19531,
		.instance = 3
	},
	{
		.type = EMCFAN_TRANSLATE_INT,
		.clear_mask = 0b11000000,
		.bit_mask = 0b10000000,
		.human_int = 4882,
		.instance = 3
	},
	{
		.type = EMCFAN_TRANSLATE_INT,
		.clear_mask = 0b11000000,
		.bit_mask = 0b11000000,
		.human_int = 2441,
		.instance = 3
	},

	{
		.type = EMCFAN_TRANSLATE_INT,
		.clear_mask = 0b00000011,
		.bit_mask = 0b00000000,
		.human_int = 26000,
		.instance = 23053
	},
	{
		.type = EMCFAN_TRANSLATE_INT,
		.clear_mask = 0b00000011,
		.bit_mask = 0b00000001,
		.human_int = 19531,
		.instance = 23053
	},
	{
		.type = EMCFAN_TRANSLATE_INT,
		.clear_mask = 0b00000011,
		.bit_mask = 0b00000010,
		.human_int = 4882,
		.instance = 23053
	},
	{
		.type = EMCFAN_TRANSLATE_INT,
		.clear_mask = 0b00000011,
		.bit_mask = 0b00000011,
		.human_int = 2441,
		.instance = 23053
	},

	{
		.type = EMCFAN_TRANSLATE_INT,
		.clear_mask = 0b00001100,
		.bit_mask = 0b00000000,
		.human_int = 26000,
		.instance = 23054
	},
	{
		.type = EMCFAN_TRANSLATE_INT,
		.clear_mask = 0b00001100,
		.bit_mask = 0b00000100,
		.human_int = 19531,
		.instance = 23054
	},
	{
		.type = EMCFAN_TRANSLATE_INT,
		.clear_mask = 0b00001100,
		.bit_mask = 0b00001000,
		.human_int = 4882,
		.instance = 23054
	},
	{
		.type = EMCFAN_TRANSLATE_INT,
		.clear_mask = 0b00001100,
		.bit_mask = 0b00001100,
		.human_int = 2441,
		.instance = 23054
	}
};

static const struct emcfan_bits_translate fan_pwm_output_type[] = {
	{
		.type = EMCFAN_TRANSLATE_STR,
		.clear_mask = 0b00000001,
		.bit_mask = 0b00000001,
		.human_str = "push-pull",
		.instance = 0
	},
	{
		.type = EMCFAN_TRANSLATE_STR,
		.clear_mask = 0b00000001,
		.bit_mask = 0b00000000,
		.human_str = "open-drain",
		.instance = 0
	},
	{
		.type = EMCFAN_TRANSLATE_STR,
		.clear_mask = 0b00000010,
		.bit_mask = 0b00000010,
		.human_str = "push-pull",
		.instance = 1
	},
	{
		.type = EMCFAN_TRANSLATE_STR,
		.clear_mask = 0b00000010,
		.bit_mask = 0b00000000,
		.human_str = "open-drain",
		.instance = 1
	},
	{
		.type = EMCFAN_TRANSLATE_STR,
		.clear_mask = 0b00000100,
		.bit_mask = 0b00000100,
		.human_str = "push-pull",
		.instance = 2
	},
	{
		.type = EMCFAN_TRANSLATE_STR,
		.clear_mask = 0b00000100,
		.bit_mask = 0b00000000,
		.human_str = "open-drain",
		.instance = 2
	},
	{
		.type = EMCFAN_TRANSLATE_STR,
		.clear_mask = 0b00001000,
		.bit_mask = 0b00001000,
		.human_str = "push-pull",
		.instance = 3
	},
	{
		.type = EMCFAN_TRANSLATE_STR,
		.clear_mask = 0b00001000,
		.bit_mask = 0b00000000,
		.human_str = "open-drain",
		.instance = 3
	},
	{
		.type = EMCFAN_TRANSLATE_STR,
		.clear_mask = 0b00010000,
		.bit_mask = 0b00010000,
		.human_str = "push-pull",
		.instance = 4
	},
	{
		.type = EMCFAN_TRANSLATE_STR,
		.clear_mask = 0b00010000,
		.bit_mask = 0b00000000,
		.human_str = "open-drain",
		.instance = 4
	}
};

static const struct emcfan_bits_translate smbus_timeout[] = {
	{
		.type = EMCFAN_TRANSLATE_STR,
		.clear_mask = 0b00001000,
		.bit_mask = 0b00000000,
		.human_str = "on",
		.instance = 2101
	},
	{
		.type = EMCFAN_TRANSLATE_STR,
		.clear_mask = 0b00001000,
		.bit_mask = 0b00001000,
		.human_str = "off",
		.instance = 2101
	},
	{
		.type = EMCFAN_TRANSLATE_STR,
		.clear_mask = 0b00100000,
		.bit_mask = 0b00000000,
		.human_str = "on",
		.instance = 2103
	},
	{
		.type = EMCFAN_TRANSLATE_STR,
		.clear_mask = 0b00100000,
		.bit_mask = 0b00100000,
		.human_str = "off",
		.instance = 2103
	},
	{
		.type = EMCFAN_TRANSLATE_STR,
		.clear_mask = 0b01000000,
		.bit_mask = 0b00000000,
		.human_str = "on",
		.instance = 2301
	},
	{
		.type = EMCFAN_TRANSLATE_STR,
		.clear_mask = 0b01000000,
		.bit_mask = 0b01000000,
		.human_str = "off",
		.instance = 2301
	}
};

static const struct emcfan_bits_translate apd[] = {
	{
		.type = EMCFAN_TRANSLATE_STR,
		.clear_mask = 0b00000001,
		.bit_mask = 0b00000001,
		.human_str = "on"
	},
	{
		.type = EMCFAN_TRANSLATE_STR,
		.clear_mask = 0b00000001,
		.bit_mask = 0b00000000,
		.human_str = "off"
	}
};

#endif
