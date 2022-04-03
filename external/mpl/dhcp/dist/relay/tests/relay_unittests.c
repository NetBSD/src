/*	$NetBSD: relay_unittests.c,v 1.4 2022/04/03 01:10:59 christos Exp $	*/

/*
 * Copyright (C) 2019-2022 Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 *   Internet Systems Consortium, Inc.
 *   PO Box 360
 *   Newmarket, NH 03857 USA
 *   <info@isc.org>
 *   https://www.isc.org/
 *
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: relay_unittests.c,v 1.4 2022/04/03 01:10:59 christos Exp $");

#include "config.h"
#include <atf-c.h>
#include <omapip/omapip_p.h>
#include "dhcpd.h"

/* @brief Externs for dhcrelay.c functions under test */
extern int add_agent_options;
extern int add_relay_agent_options(struct interface_info *,
                                   struct dhcp_packet *, unsigned,
                                   struct in_addr);

extern int find_interface_by_agent_option(struct dhcp_packet *,
                                          struct interface_info **,
                                          u_int8_t *, int);

extern int strip_relay_agent_options(struct interface_info *,
                                     struct interface_info **,
                                     struct dhcp_packet *, unsigned);

/* @brief Add the given option data to a DHCPv4 packet
*
* It first fills the packet.options buffer with the given pad character.
* Next it copies the DHCP magic cookie value into the beginning of the
* options buffer. Finally it appends the given data after the cookie.
*
* @param packet pointer to the packet
* @param data pointer to the option data to copy into the packet's options
* buffer
* @param len length of the option data to copy
* @param pad byte value with which to initialize the packet's options buffer
*
* @return returns the new length of the packet
*/
unsigned set_packet_options(struct dhcp_packet *packet,
                            unsigned char* data, unsigned len,
                            unsigned char pad) {
    unsigned new_len;
    memset(packet->options, pad, DHCP_MAX_OPTION_LEN);

    // Add the COOKIE
    new_len = 4;
    memcpy(packet->options, DHCP_OPTIONS_COOKIE, new_len);

    new_len += len;
    if (new_len > DHCP_MAX_OPTION_LEN) {
        return(0);
    }

    memcpy(&packet->options[4], data, len);
    return(new_len + DHCP_FIXED_NON_UDP);
}

/* @brief Checks two sets of option data for equalit
*
* It constructs the expected options content by creating an options buffer
* filled with the pad value.  Next it copies the DHCP magic cookie value
* into the beginning of the buffer and then appends the expected data after
* the cookie.  It the compares this buffer to the actual buffer passed in
* for equality and returns the result.
*
* @param actual_options pointer to the packet::options to be checked
* @param expected_data pointer to the expected options data (everything after
* the DHCP cookie)
* @param data_len length of the expected options data
* @param pad byte value with which to initialize the packet's options buffer
*
* @return zero it the sets of data match, non-zero otherwise
*/
int check_with_pad(unsigned char* actual_options,
                   unsigned char *expected_data,
                   unsigned data_len, unsigned char pad) {

    unsigned char exp_options[DHCP_MAX_OPTION_LEN];
    unsigned new_len;

    memset(exp_options, pad, DHCP_MAX_OPTION_LEN);
    new_len = 4;
    memcpy(exp_options, DHCP_OPTIONS_COOKIE, new_len);

    new_len += data_len;
    if (new_len > DHCP_MAX_OPTION_LEN) {
        return(-1);
    }

    memcpy(&exp_options[4], expected_data, data_len);
    return (memcmp(actual_options, exp_options, DHCP_MAX_OPTION_LEN));
}

ATF_TC(strip_relay_agent_options_test);

ATF_TC_HEAD(strip_relay_agent_options_test, tc) {
    atf_tc_set_md_var(tc, "descr", "tests strip_relay-agent_options");
}

/* This Test exercises strip_relay_agent_options() function */
ATF_TC_BODY(strip_relay_agent_options_test, tc) {

    struct interface_info ifaces;
    struct interface_info *matched;
    struct dhcp_packet packet;
    unsigned len;
    int ret;

    memset(&ifaces, 0x0, sizeof(ifaces));
    matched = 0;
    memset(&packet, 0x0, sizeof(packet));
    len = 0;

    /* Make sure an empty packet is harmless. We set add_agent_options = 1
     * to avoid early return when it's 0.  */
    add_agent_options = 1;
    ret = strip_relay_agent_options(&ifaces, &matched, &packet, len);
    if (ret != 0) {
       atf_tc_fail("empty packet failed");
    }

    {
        /*
         * Uses valid input option data to verify that:
         * - When add_agent_options is false, the output option data is the
         *   the same as the input option data (i.e. nothing removed)
         * - When add_agent_options is true, 0 length relay agent option has
         *   been removed from the output option data
         * - When add_agent_options is true, a relay agent option has
         *   been removed from the output option data
         *
        */

        unsigned char input_data[] = {
            0x35, 0x00,                   /* DHCP_TYPE = DISCOVER */
            0x52, 0x08, 0x01, 0x06, 0x65, /* Relay Agent Option, len = 8 */
            0x6e, 0x70, 0x30, 0x73, 0x4f,

            0x42, 0x02, 0x00, 0x10,       /* Opt 0x42, len = 2 */
            0x43, 0x00                    /* Opt 0x43, len = 0 */
        };

        unsigned char input_data2[] = {
            0x35, 0x00,                   /* DHCP_TYPE = DISCOVER */
            0x52, 0x00,                   /* Relay Agent Option, len = 0 */
            0x42, 0x02, 0x00, 0x10,       /* Opt 0x42, len = 2 */
            0x43, 0x00                    /* Opt 0x43, len = 0 */
        };

        unsigned char output_data[] = {
            0x35, 0x00,                   /* DHCP_TYPE = DISCOVER */
            0x42, 0x02, 0x00, 0x10,       /* Opt 0x42, len = 2 */
            0x43, 0x00                    /* Opt 0x43, len = 0 */
        };

        unsigned char pad = 0x0;
        len = set_packet_options(&packet, input_data, sizeof(input_data), pad);
        if (len == 0) {
            atf_tc_fail("input_data: set_packet_options failed");
        }

        /* When add_agent_options = 0, no change should occur */
        add_agent_options = 0;
        ret = strip_relay_agent_options(&ifaces, &matched, &packet, len);
        if (ret != len) {
            atf_tc_fail("expected unchanged len %d, returned %d", len, ret);
        }

        if (check_with_pad(packet.options, input_data, sizeof(input_data),
                           pad) != 0) {
            atf_tc_fail("expected unchanged data, does not match");
        }

        /* When add_agent_options = 1, it should remove the eight byte
         * relay agent option. */
        add_agent_options = 1;

        /* Beause the inbound option data is less than the BOOTP_MIN_LEN,
         * the output data  should get padded out to BOOTP_MIN_LEN
         * padded out to BOOTP_MIN_LEN */
        ret = strip_relay_agent_options(&ifaces, &matched, &packet, len);
        if (ret != BOOTP_MIN_LEN) {
            atf_tc_fail("input_data: len of %d, returned %d",
                        BOOTP_MIN_LEN, ret);
        }

        if (check_with_pad(packet.options, output_data, sizeof(output_data),
                           pad) != 0) {
            atf_tc_fail("input_data: expected data does not match");
        }

        /* Now let's repeat it with a relay agent option 0 bytes in length. */
        len = set_packet_options(&packet, input_data2, sizeof(input_data2), pad);
        if (len == 0) {
            atf_tc_fail("input_data2 set_packet_options failed");
        }

        /* Because the inbound option data is less than the BOOTP_MIN_LEN,
         * the output data should get padded out to BOOTP_MIN_LEN
         * padded out to BOOTP_MIN_LEN */
        ret = strip_relay_agent_options(&ifaces, &matched, &packet, len);
        if (ret != BOOTP_MIN_LEN) {
            atf_tc_fail("input_data2: len of %d, returned %d",
                        BOOTP_MIN_LEN, ret);
        }

        if (check_with_pad(packet.options, output_data, sizeof(output_data),
                           pad) != 0) {
            atf_tc_fail("input_data2: expected output does not match");
        }
    }

    {
        /* Verify that oversized packet filled with long options does not
         * cause overrun */

        /* We borrowed this union from discover.c:got_one() */
        union {
                unsigned char packbuf [4095]; /* Packet input buffer.
                                               * Must be as large as largest
                                               * possible MTU. */
                struct dhcp_packet packet;
        } u;

        unsigned char input_data[] = {
            0x35, 0x00,             /* DHCP_TYPE = DISCOVER        */
            0x52, 0x00,             /* Relay Agent Option, len = 0 */
            0x42, 0x02, 0x00, 0x10, /* Opt 0x42, len = 2           */
            0x43, 0x00              /* Opt 0x43, len = 0           */
        };

        /* We'll pad it 0xFE, that way wherever we hit for "length" we'll
         * have length of 254. Increasing the odds we'll push over the end. */
        unsigned char pad = 0xFE;
        memset(u.packbuf, pad, 4095);

        len = set_packet_options(&u.packet, input_data, sizeof(input_data), pad);
        if (len == 0) {
            atf_tc_fail("overrun: set_packet_options failed");
        }

        /* Enable option stripping by setting add_agent_options = 1 */
        add_agent_options = 1;

        /* strip_relay_agent_options() should detect the overrun and return 0 */
        ret = strip_relay_agent_options(&ifaces, &matched, &u.packet, 4095);
        if (ret != 0) {
            atf_tc_fail("overrun expected return len = 0, we got %d", ret);
        }
    }
}

ATF_TC(add_relay_agent_options_test);

ATF_TC_HEAD(add_relay_agent_options_test, tc) {
    atf_tc_set_md_var(tc, "descr", "tests agent_relay-agent_options");
}

/* This Test exercises add_relay_agent_options() function */
ATF_TC_BODY(add_relay_agent_options_test, tc) {

    struct interface_info ifc;
    struct dhcp_packet packet;
    unsigned len;
    int ret;
    struct in_addr giaddr;

    giaddr.s_addr = inet_addr("192.0.1.1");

    u_int8_t circuit_id[] = { 0x01,0x02,0x03,0x04,0x05,0x06 };
    u_int8_t remote_id[] = { 0x11,0x22,0x33,0x44,0x55,0x66 };

    memset(&ifc, 0x0, sizeof(ifc));
    ifc.circuit_id = circuit_id;
    ifc.circuit_id_len = sizeof(circuit_id);
    ifc.remote_id = remote_id;
    ifc.remote_id_len = sizeof(remote_id);

    memset(&packet, 0x0, sizeof(packet));
    len = 0;

    /* Make sure an empty packet is harmless */
    ret = add_relay_agent_options(&ifc, &packet, len, giaddr);
    if (ret != 0) {
       atf_tc_fail("empty packet failed");
    }

    {
        /*
         * Uses valid input option data to verify that:
         * - When add_agent_options is false, the output option data is the
         *   the same as the input option data (i.e. nothing removed)
         * - When add_agent_options is true, the relay agent option has
         *   been removed from the output option data
         * - When add_agent_options is true, 0 length relay agent option has
         *   been removed from the output option data
         *
        */

        unsigned char input_data[] = {
            0x35, 0x00,                   /* DHCP_TYPE = DISCOVER */
            0x52, 0x08, 0x01, 0x06, 0x65, /* Relay Agent Option, len = 8 */
            0x6e, 0x70, 0x30, 0x73, 0x4f,
            0x42, 0x02, 0x00, 0x10,       /* Opt 0x42, len = 2 */
            0x43, 0x00                    /* Opt 0x43, len = 0 */
        };

        unsigned char input_data2[] = {
            0x35, 0x00,                   /* DHCP_TYPE = DISCOVER */
            0x52, 0x00,                   /* Relay Agent Option, len = 0 */
            0x42, 0x02, 0x00, 0x10,       /* Opt 0x42, len = 2 */
            0x43, 0x00                    /* Opt 0x43, len = 0 */
        };

        unsigned char output_data[] = {
            0x35, 0x00,                   /* DHCP_TYPE = DISCOVER */
            0x42, 0x02, 0x00, 0x10,       /* Opt 0x42, len = 2 */
            0x43, 0x00,                   /* Opt 0x43, len = 0 */
            0x52, 0x10,                   /* Relay Agent, len = 16 */
            0x1, 0x6,                     /* Circuit id, len = 6 */
            0x1, 0x2, 0x3, 0x4, 0x5, 0x6,
            0x2, 0x6,                     /* Remete id, len = 6 */
            0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0xff
        };

        unsigned char pad = 0x0;
        len = set_packet_options(&packet, input_data, sizeof(input_data), pad);
        if (len == 0) {
            atf_tc_fail("input_data: set_packet_options failed");
        }

        /* When add_agent_options = 0, no change should occur */
        add_agent_options = 0;
        ret = add_relay_agent_options(&ifc, &packet, len, giaddr);
        if (ret != len) {
            atf_tc_fail("expected unchanged len %d, returned %d", len, ret);
        }

        if (check_with_pad(packet.options, input_data, sizeof(input_data),
                           pad) != 0) {
            atf_tc_fail("expected unchanged data, does not match");
        }

        /* When add_agent_options = 1, it should remove the eight byte
         * relay agent option. */
        add_agent_options = 1;

        /* Because the inbound option data is less than the BOOTP_MIN_LEN,
         * the output data should get padded out to BOOTP_MIN_LEN
         * padded out to BOOTP_MIN_LEN */
        ret = add_relay_agent_options(&ifc, &packet, len, giaddr);
        if (ret != BOOTP_MIN_LEN) {
            atf_tc_fail("input_data: len of %d, returned %d",
                        BOOTP_MIN_LEN, ret);
        }

        if (check_with_pad(packet.options, output_data, sizeof(output_data),
                           pad) != 0) {
            atf_tc_fail("input_data: expected data does not match");
        }

        /* Now let's repeat it with a relay agent option 0 bytes in length. */
        len = set_packet_options(&packet, input_data2, sizeof(input_data2),
                                 pad);
        if (len == 0) {
            atf_tc_fail("input_data2 set_packet_options failed");
        }

        /* Because the inbound option data is less than the BOOTP_MIN_LEN,
         * the output data should get padded out to BOOTP_MIN_LEN
         * padded out to BOOTP_MIN_LEN */
        ret = add_relay_agent_options(&ifc, &packet, len, giaddr);
        if (ret != BOOTP_MIN_LEN) {
            atf_tc_fail("input_data2: len of %d, returned %d",
                        BOOTP_MIN_LEN, ret);
        }

        if (check_with_pad(packet.options, output_data, sizeof(output_data),
                           pad) != 0) {
            atf_tc_fail("input_data2: expected output does not match");
        }
    }
}

ATF_TC(gwaddr_override_test);

ATF_TC_HEAD(gwaddr_override_test, tc) {
    atf_tc_set_md_var(tc, "descr", "tests that gateway addr (giaddr) field can be overridden");
}

extern isc_boolean_t use_fake_gw;
extern struct in_addr gw;

/* This basic test checks if the new gwaddr override (-g) option is disabled by default */
ATF_TC_BODY(gwaddr_override_test, tc) {

    if (use_fake_gw == ISC_TRUE) {
        atf_tc_fail("the gwaddr override should be disabled by default");
    }
    char txt[16] = {0};
    inet_ntop(AF_INET, &gw, txt, sizeof(txt));
    if (strncmp(txt, "0.0.0.0", 8) != 0) {
        atf_tc_fail("the default gwaddr override value should be 0.0.0.0, but is %s", txt);
    }
}

ATF_TP_ADD_TCS(tp) {
    ATF_TP_ADD_TC(tp, strip_relay_agent_options_test);
    ATF_TP_ADD_TC(tp, add_relay_agent_options_test);
    ATF_TP_ADD_TC(tp, gwaddr_override_test);

    return (atf_no_error());
}
