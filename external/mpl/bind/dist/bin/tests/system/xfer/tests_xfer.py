# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# SPDX-License-Identifier: MPL-2.0
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0.  If a copy of the MPL was not distributed with this
# file, you can obtain one at https://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.


from re import compile as Re

import fileinput
import os
import socket
import time

import dns.message
import dns.query
import dns.rdataclass
import dns.rdatatype
import dns.tsig
import dns.zone
import pytest

from isctest.util import param

import isctest

NEW_SOA_SERIAL = 1397051953
OLD_SOA_SERIAL = 1397051952


def sendcmd(cmdfile):
    host = "10.53.0.5"
    port = int(isctest.vars.ALL["EXTRAPORT1"])
    cmdfile = f"ans5/{cmdfile}"
    assert os.path.exists(cmdfile)

    sock = socket.create_connection((host, port))
    with open(cmdfile, "r", encoding="utf-8") as f:
        for line in f:
            sock.sendall(line.encode())
    sock.close()


@pytest.fixture(scope="module", autouse=True)
def after_servers_start(templates, ns4):
    # initial correctly-signed transfer should succeed
    sendcmd("goodaxfr")

    with ns4.watch_log_from_here() as watcher:
        templates.render("ns4/named.conf", {"ns4_as_secondary_for_nil": True})
        ns4.reconfigure()
        watcher.wait_for_line("Transfer status: success")

    with ns4.watch_log_from_here() as watcher_retransfer_nil_success:
        ns4.rndc("retransfer nil.")
        watcher_retransfer_nil_success.wait_for_line("Transfer status: success")


def get_response(msg, server_ip, allow_empty_answer=False):
    res = isctest.query.tcp(msg, server_ip)
    isctest.check.noerror(res)
    if not allow_empty_answer:
        assert len(res.answer) > 0
    return res


def check_rdata_in_txt_record(rdata, should_exist=True):
    def check_data():
        qname = "nil."
        msg = dns.message.make_query(qname, "TXT")
        res = get_response(msg, "10.53.0.4")
        rrset = res.get_rrset(
            dns.message.ANSWER, qname, dns.rdataclass.IN, dns.rdatatype.TXT
        )
        found = rdata in rrset.to_text()
        if found == should_exist:
            return True
        status = "not found" if should_exist else "found"
        assert False, f"TXT rdata '{rdata}' {status} in the response\n{res}"

    isctest.run.retry_with_timeout(check_data, timeout=10)


def nsupdate(config):
    isctest.run.cmd(isctest.vars.ALL["NSUPDATE"], input_text=config.encode("utf-8"))


def validate_axfr_from_query_and_file(msg, server_ip, filename):
    res = get_response(msg, server_ip)
    with open(filename, "r", encoding="utf-8") as file:
        expected = dns.message.from_file(file)
        isctest.check.rrsets_equal(expected.answer, res.answer)


def test_zone_transfer_fallback_to_dns_after_dot_failed():
    msg = dns.message.make_query("dot-fallback.", "AXFR")
    validate_axfr_from_query_and_file(msg, "10.53.0.2", "response3.good")


def test_tsig_signed_zone_transfer():
    key = dns.tsig.Key(
        name="tsigzone.",
        secret="1234abcd8765",
        algorithm=isctest.vars.ALL["DEFAULT_HMAC"],
    )
    msg = dns.message.make_query("tsigzone.", "AXFR")
    msg.use_tsig(keyring=key)
    res2 = get_response(msg, "10.53.0.2")
    res3 = get_response(msg, "10.53.0.3")
    isctest.check.rrsets_equal(res2.answer, res3.answer)


def test_zone_is_dumped_after_transfer(ns1, ns2, ns3, ns6, ns7):
    def check_soa_serial_with_retry(checked_zones, recovery_function):
        def get_soa_serial(qname, server_ip, serial):
            msg = dns.message.make_query(qname, "SOA")
            res = get_response(msg, server_ip)
            rrset = res.get_rrset(
                dns.message.ANSWER, qname, dns.rdataclass.IN, dns.rdatatype.SOA
            )
            return rrset[0].serial == serial

        def find_serial_in_responses():
            serial_found_in_responses = 0
            for server, zone in checked_zones:
                if get_soa_serial(zone, server, NEW_SOA_SERIAL):
                    serial_found_in_responses += 1
            if serial_found_in_responses == len(checked_zones):
                return True
            recovery_function()
            return False

        isctest.run.retry_with_timeout(
            find_serial_in_responses,
            timeout=20,
            msg=f"SOA serial {NEW_SOA_SERIAL} not found in responses",
        )

    def validate_axfr_from_query_and_query(msg, server_ip1, server_ip2):
        res1 = get_response(msg, server_ip1)
        res2 = get_response(msg, server_ip2)
        isctest.check.rrsets_equal(res1.answer, res2.answer)

    def rndc_reload(*servers):
        for server in servers:
            server.reload()

    isctest.log.info("reload servers for preparation of ixfr-from-differences tests")
    rndc_reload(ns1, ns2, ns3, ns6, ns7)

    isctest.log.info("basic zone transfer")
    msg = dns.message.make_query("example.", "AXFR")
    validate_axfr_from_query_and_file(msg, "10.53.0.2", "response1.good")
    validate_axfr_from_query_and_file(msg, "10.53.0.3", "response1.good")

    isctest.log.info("update primary zones for ixfr-from-differences tests")
    for zone_file in [
        "ns1/sec.db",
        "ns2/example.db",
        "ns6/primary.db",
        "ns7/primary2.db",
    ]:
        with fileinput.FileInput(zone_file, inplace=True) as file:
            for line in file:
                print(
                    line.replace(" 0.0.0.0", " 0.0.0.1").replace(
                        str(OLD_SOA_SERIAL), str(NEW_SOA_SERIAL)
                    ),
                    end="",
                )
    rndc_reload(ns1, ns2, ns6, ns7)

    qname = "secondary."
    msg = dns.message.make_query(qname, "SOA")
    res = get_response(msg, "10.53.0.2")
    rrset = res.get_rrset(
        dns.message.ANSWER, qname, dns.rdataclass.IN, dns.rdatatype.SOA
    )
    assert (
        rrset[0].serial == OLD_SOA_SERIAL
    ), f"SOA serial {OLD_SOA_SERIAL} not found in the response"

    sec_db = isctest.text.TextFile("ns2/sec.db")
    assert (
        f"{OLD_SOA_SERIAL} ; serial" in sec_db
    ), f"{OLD_SOA_SERIAL} not found in ns2/sec.db"

    isctest.log.info("wait for reloads")
    reloaded_zones = (
        ("10.53.0.6", "primary."),
        ("10.53.0.1", "secondary."),
        ("10.53.0.2", "example."),
    )
    check_soa_serial_with_retry(reloaded_zones, lambda: time.sleep(1))

    def notify_some_servers():
        ns6.rndc("notify primary.")
        ns1.rndc("notify secondary.")
        ns2.rndc("notify example.")
        time.sleep(2)

    isctest.log.info("wait for transfers")
    transfered_zones = (
        ("10.53.0.3", "example."),
        ("10.53.0.3", "primary."),
        ("10.53.0.6", "secondary."),
    )
    check_soa_serial_with_retry(transfered_zones, notify_some_servers)

    msg = dns.message.make_query("example.", "AXFR")
    validate_axfr_from_query_and_file(msg, "10.53.0.3", "response2.good")

    isctest.log.info("ns3 has a journal iff it received an IXFR.")
    assert os.path.exists("ns3/example.bk")
    assert os.path.exists("ns3/example.bk.jnl")

    isctest.log.info("testing ixfr-from-differences primary; (primary zone)")
    msg = dns.message.make_query("primary.", "AXFR")
    validate_axfr_from_query_and_query(msg, "10.53.0.6", "10.53.0.3")

    isctest.log.info("ns3 has a journal iff it received an IXFR.")
    assert os.path.exists("ns3/primary.bk")
    assert os.path.exists("ns3/primary.bk.jnl")

    isctest.log.info("testing ixfr-from-differences primary; (secondary zone)")
    msg = dns.message.make_query("secondary.", "AXFR")
    validate_axfr_from_query_and_query(msg, "10.53.0.6", "10.53.0.1")

    isctest.log.info("ns6 has a journal iff it received an IXFR.")
    assert os.path.exists("ns6/sec.bk")
    assert not os.path.exists("ns6/sec.bk.jnl")

    isctest.log.info("testing ixfr-from-differences secondary; (primary zone)")

    isctest.log.info("ns7 has a journal iff it generates an IXFR.")
    assert os.path.exists("ns7/primary2.db")
    assert not os.path.exists("ns7/primary2.db.jnl")

    isctest.log.info("testing ixfr-from-differences secondary; (secondary zone)")
    msg = dns.message.make_query("secondary.", "AXFR")
    validate_axfr_from_query_and_query(msg, "10.53.0.1", "10.53.0.7")

    isctest.log.info("ns7 has a journal iff it generates an IXFR.")
    assert os.path.exists("ns7/sec.bk")
    assert os.path.exists("ns7/sec.bk.jnl")


# check that a multi-message uncompressable zone transfers
def test_multi_message_uncompressable_zone_transfers(named_port):
    zone = dns.zone.Zone(".")
    isctest.log.info("Initiate a zone transfer from the server")
    dns.query.inbound_xfr("10.53.0.4", zone, port=named_port, timeout=10, lifetime=10)

    for name, node in zone.nodes.items():
        label = name.to_text()
        fqdn = name.derelativize(zone.origin).to_text()

        for rdataset in node.rdatasets:
            rtype = dns.rdatatype.to_text(rdataset.rdtype)
            for rdata in rdataset:
                if rtype == "A":
                    # The A records name is either "." or in the format "xN",
                    # where N is a number between 0 and 9999
                    assert fqdn == "." or (
                        label.startswith("x")
                        and label[1:].isdigit()
                        and 0 <= int(label[1:]) < 10000
                    )
                elif rtype in ("SOA", "NS"):
                    assert fqdn == "."
                else:
                    assert (
                        False
                    ), f"Unexpected RRset: {fqdn} {rdataset.ttl} IN {rtype} {rdata}"


# Initially, ns4 is not authoritative for anything.
# Now that ans is up and running with the right data, we make ns4
# a secondary for nil.
def test_make_ns4_secondary_for_nil():
    # now we test transfers with assorted TSIG glitches.
    # testing that incorrectly signed transfers will fail.

    def wait_for_soa():
        def _wait_for_soa():
            qname = "nil."
            msg = dns.message.make_query(qname, "SOA")
            res = isctest.query.tcp(msg, "10.53.0.4")
            rrset = res.get_rrset(
                dns.message.ANSWER, qname, dns.rdataclass.IN, dns.rdatatype.SOA
            )
            return True if rrset else time.sleep(1)

        isctest.run.retry_with_timeout(_wait_for_soa, timeout=10)
        return True

    sendcmd("goodaxfr")
    assert wait_for_soa(), "SOA not found in the response"
    check_rdata_in_txt_record("initial AXFR")


def test_handle_ixfr_notimp(ns4):
    sendcmd("ixfrnotimp")
    with ns4.watch_log_from_here() as watcher_transfer_success:
        with ns4.watch_log_from_here() as watcher_requesting_ixfr:
            ns4.rndc("refresh nil.")
            watcher_requesting_ixfr.wait_for_line(
                "zone nil/IN: requesting IXFR from 10.53.0.5"
            )
        watcher_transfer_success.wait_for_line("Transfer status: success")

    check_rdata_in_txt_record("IXFR NOTIMP")


@pytest.mark.parametrize(
    "command_file,expected_rdata,named_log_line",
    [
        param(
            "unsigned",
            "unsigned AXFR",
            "Transfer status: expected a TSIG or SIG(0)",
        ),
        param(
            "badkeydata",
            "bad keydata AXFR",
            "Transfer status: tsig verify failure",
        ),
        param(
            "partial",
            "partially signed AXFR",
            "Transfer status: expected a TSIG or SIG(0)",
        ),
        param(
            "unknownkey",
            "unknown key AXFR",
            "tsig key 'tsig_key': key name and algorithm do not match",
        ),
        param(
            "wrongkey",
            "incorrect key AXFR",
            "tsig key 'tsig_key': key name and algorithm do not match",
        ),
        param(
            "wrongname",
            "wrong question AXFR",
            "question name mismatch",
        ),
        param(
            "badmessageid",
            "bad message id",
            "Transfer status: unexpected error",
        ),
        param(
            "soamismatch",
            "SOA mismatch AXFR",
            "Transfer status: FORMERR",
        ),
    ],
)
def test_under_signed_transfer(command_file, expected_rdata, named_log_line, ns4):
    sendcmd(command_file)
    with ns4.watch_log_from_here() as watcher:
        ns4.rndc("retransfer nil.")
        watcher.wait_for_line(named_log_line)
    check_rdata_in_txt_record(expected_rdata, should_exist=False)


def test_handle_edns_notimp(ns4):
    sendcmd("ednsnotimp")
    with ns4.watch_log_from_here() as watcher:
        ns4.rndc("retransfer nil.")
        watcher.wait_for_line("Transfer status: NOTIMP")


def test_handle_edns_formerr(ns4):
    sendcmd("ednsformerr")
    with ns4.watch_log_from_here() as watcher:
        ns4.rndc("retransfer nil.")
        watcher.wait_for_line("Transfer status: success")
    check_rdata_in_txt_record("EDNS FORMERR")


# check that we asked for and received a EDNS EXPIRE response when transfering from a secondary
def test_edns_expire_from_secondary(ns7):
    pattern = Re(
        "zone edns-expire/IN: zone transfer finished: success, expire=1814[0-4][0-9][0-9]"
    )
    with ns7.watch_log_from_start() as watcher:
        watcher.wait_for_line(pattern)


# check that we ask for and get a EDNS EXPIRE response when refreshing
def test_edns_expire_refresh(ns7):
    time.sleep(1)
    with ns7.watch_log_from_here() as watcher:
        ns7.rndc("refresh edns-expire.")
        isctest.log.info("make sure the EDNS EXPIRE of 1814400 decreases a slightly")
        pattern = Re("zone edns-expire/IN: got EDNS EXPIRE of 1814[0-3][0-9][0-9]")
        watcher.wait_for_line(pattern)


# test small transfer TCP message size (transfer-message-size 1024;)
def test_tcp_message_compression_makes_difference(named_port, ns8):
    key = dns.tsig.Key(
        name="key1.",
        secret="1234abcd8765",
        algorithm=isctest.vars.ALL["DEFAULT_HMAC"],
    )
    msg = dns.message.make_query("example.", "AXFR")
    msg.use_tsig(keyring=key)
    zone = dns.zone.Zone("example.")
    dns.query.inbound_xfr(
        "10.53.0.8", zone, query=msg, port=named_port, timeout=10, lifetime=10
    )

    xfr_size = 0
    for name, node in zone.nodes.items():
        fqdn = name.derelativize(zone.origin).to_text()
        for rdataset in node.rdatasets:
            xfr_size += len(f"{fqdn} {rdataset}")
    assert xfr_size >= 452172, f"XFR size {xfr_size} seems too small"

    assert len(ns8.log.grep("sending TCP message of")) > 300


# test mapped. zone with out zone data
def test_mapped_zone(named_port, ns3):
    msg_txt = dns.message.make_query("mapped.", "TXT")
    get_response(msg_txt, "10.53.0.3", allow_empty_answer=True)

    ns3.stop()
    ns3.start(["--noclean", "--restart", "--port", str(named_port)])

    get_response(msg_txt, "10.53.0.3", allow_empty_answer=True)

    msg_axfr = dns.message.make_query("mapped.", "AXFR")
    validate_axfr_from_query_and_file(msg_axfr, "10.53.0.3", "knowngood.mapped")


# test that a zone with too many records is rejected (AXFR)
def test_axfr_too_many_records(ns6):
    with ns6.watch_log_from_start() as watcher:
        watcher.wait_for_line(Re("'axfr-too-big/IN'.*: too many records"))


# test that a zone with too many records is rejected (IXFR)
def test_ixfr_too_many_records(named_port, ns6):
    with ns6.watch_log_from_here(timeout=20) as watcher:
        nsupdate_config = f"""
        zone ixfr-too-big
        server 10.53.0.1 {named_port}
        update add the-31st-record.ixfr-too-big 0 TXT this is it
        send
        """
        nsupdate(nsupdate_config)
        watcher.wait_for_line("Transfer status: too many records")


# checking whether dig calculates AXFR statistics correctly
def test_dig_and_named_axfr_stats(named_port, ns3):
    # Use ns3 logs for checking incoming transfer statistics as ns3 is a
    # secondary server (for ns1) for "xfer-stats".
    with ns3.watch_log_from_start() as watcher_transfer_completed:
        pattern_transfer_completed = (
            "Transfer completed: 16 messages, 10003 records, 218403 bytes"
        )
        watcher_transfer_completed.wait_for_line(pattern_transfer_completed)

    # Loop until the secondary server manages to transfer the "xfer-stats" zone so
    # that we can both check dig output and immediately proceed with the next test.
    # Use -b so that we can discern between incoming and outgoing transfers in ns3
    # logs later on.
    dig_source_port = os.getenv("EXTRAPORT1")
    dig = isctest.run.isctest.run.EnvCmd("DIG", f"-p {str(named_port)}")
    output = dig(
        f"+tcp +noadd +nosea +nostat +noquest +nocomm +nocmd +edns +nocookie +noexpire +stat -b 10.53.0.2#{dig_source_port} @10.53.0.3 xfer-stats. AXFR"
    ).out

    assert "; Transfer failed" not in output
    assert ";; XFR size: 10003 records (messages 16, bytes 218403)" in output

    # Use ns3 logs for checking outgoing transfer statistics as ns3 is a
    # primary server (for dig queries from the previous test) for "xfer-stats".
    isctest.log.info(
        "checking whether named calculates outgoing AXFR statistics correctly"
    )
    with ns3.watch_log_from_start() as watcher_axfr_ended:
        pattern_axfr_ended = f"10.53.0.2#{dig_source_port} (xfer-stats): transfer of 'xfer-stats/IN': AXFR ended: 16 messages, 10003 records, 218403 bytes"
        watcher_axfr_ended.wait_for_line(pattern_axfr_ended)


# test that transfer-source uses port option correctly
def test_transfer_source_option_uses_port_option_correctly(ns6):
    assert ns6.log.grep(
        f"10.53.0.3#{os.getenv('EXTRAPORT1')} (primary): query 'primary/SOA/IN' approved"
    )


# First, test that named tries the next primary in the list when the first one
# fails (XoT -> Do53). Then, test that named tries the next primary in the list
# when the first one is already marked as unreachable (XoT -> Do53).
def test_xot_primary_try_next(ns6):
    def retransfer_and_check_log():
        with ns6.watch_log_from_here(timeout=60) as watcher:
            ns6.rndc("retransfer xot-primary-try-next.")
            watcher.wait_for_line("Transfer status: success")

    retransfer_and_check_log()
    retransfer_and_check_log()


# See #5307#note_558185
def test_reconfiguration_when_zone_transfer_is_in_the_middle_of_soa_query(ns6):
    isctest.log.info(
        "Check that xfr-and-reconfig has been successfully transferred by the secondary"
    )
    with ns6.watch_log_from_start() as watcher_transfer_completed:
        watcher_transfer_completed.wait_for_line(
            "zone xfr-and-reconfig/IN: zone transfer finished: success"
        )

    isctest.log.info("Make ans6 receive queries without responding to them")
    msg = dns.message.make_query("disable.send-responses._control.", "TXT")
    get_response(msg, "10.53.0.9")

    isctest.log.info("Try to reload the zone from an unresponsive primary")
    ns6.rndc("reload xfr-and-reconfig")

    isctest.log.info("Reconfigure named while zone transfer attempt is in progress")
    ns6.reconfigure()

    isctest.log.info(
        "Confirm that the ongoing SOA request was canceled, caused by the reconfiguration"
    )
    with ns6.watch_log_from_start() as watcher_transfer_cancelled:
        watcher_transfer_cancelled.wait_for_line(
            "refresh: request result: operation canceled"
        )

    isctest.log.info("Make ans6 receive queries and respond to them")
    msg = dns.message.make_query("enable.send-responses._control.", "TXT")
    with ns6.watch_log_from_here(timeout=30) as watcher_transfer_started:
        get_response(msg, "10.53.0.9")
        isctest.log.info("Try to reload the zone from the primary")
        ns6.rndc("reload xfr-and-reconfig")
        watcher_transfer_started.wait_for_line("Transfer started")
