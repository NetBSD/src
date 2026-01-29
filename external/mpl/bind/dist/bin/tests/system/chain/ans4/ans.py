"""
Copyright (C) Internet Systems Consortium, Inc. ("ISC")

SPDX-License-Identifier: MPL-2.0

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0.  If a copy of the MPL was not distributed with this
file, you can obtain one at https://mozilla.org/MPL/2.0/.

See the COPYRIGHT file distributed with this work for additional
information regarding copyright ownership.
"""

from dataclasses import dataclass
from enum import Enum
from typing import AsyncGenerator, List, Optional, Tuple

import abc
import logging
import re

import dns.name
import dns.rcode
import dns.rdataclass
import dns.rdatatype
import dns.rrset

from isctest.asyncserver import (
    ControlCommand,
    ControllableAsyncDnsServer,
    DnsResponseSend,
    DomainHandler,
    QueryContext,
    ResponseAction,
)


class ChainNameGenerator:
    """
    Convenience class generating sequential owner/target names used in chained
    responses.

    >>> name_generator = ChainNameGenerator()
    >>> name_generator.current_name
    <DNS name test.domain.nil.>
    >>> name_generator.generate_next_name()
    <DNS name cname0.domain.nil.>
    >>> name_generator.generate_next_name()
    <DNS name cname1.domain.nil.>
    >>> name_generator.generate_next_sld()
    <DNS name domain2.nil.>
    >>> name_generator.generate_next_sld()
    <DNS name domain3.nil.>
    >>> name_generator.current_name
    <DNS name cname1.domain3.nil.>
    >>> name_generator.generate_next_name()
    <DNS name cname4.domain3.nil.>
    >>> name_generator.generate_next_name_in_next_sld()
    <DNS name cname5.domain6.nil.>
    >>> name_generator.generate_next_name_in_next_sld()
    <DNS name cname7.domain8.nil.>
    """

    def __init__(self) -> None:
        self._i = 0
        self._current_label = dns.name.Name(["test"])
        self._current_sld = dns.name.Name(["domain"])
        self._tld = dns.name.Name(["nil", ""])

    @property
    def current_name(self) -> dns.name.Name:
        return self._current_label.concatenate(self.current_domain)

    @property
    def current_domain(self) -> dns.name.Name:
        return self._current_sld.concatenate(self._tld)

    def generate_next_name(self) -> dns.name.Name:
        self._current_label = dns.name.Name([f"cname{self._i}"])
        self._i += 1
        return self.current_name

    def generate_next_sld(self) -> dns.name.Name:
        self._current_sld = dns.name.Name([f"domain{self._i}"])
        self._i += 1
        return self.current_domain

    def generate_next_name_in_next_sld(self) -> dns.name.Name:
        self.generate_next_name()
        self.generate_next_sld()
        return self.current_name


class RecordGenerator(abc.ABC):
    """
    An abstract class used as a base class for RRset generators (see the
    description of "actions" in `ChainSetupCommand`) and as a convenience class
    for creating RRsets in `ChainResponseHandler`.
    """

    @classmethod
    def create_rrset(
        cls, owner: dns.name.Name, rrtype: dns.rdatatype.RdataType, rdata: str
    ) -> dns.rrset.RRset:
        return dns.rrset.from_text(owner, 86400, dns.rdataclass.IN, rrtype, rdata)

    @classmethod
    def create_rrset_signature(
        cls, owner: dns.name.Name, rrtype: dns.rdatatype.RdataType
    ) -> dns.rrset.RRset:
        covers = dns.rdatatype.to_text(rrtype)
        ttl = "86400"
        expiry = "20900101000000"
        inception = "20250101000000"
        domain = "domain.nil."
        sigdata = "OCXH2De0yE4NMTl9UykvOsJ4IBGs/ZIpff2rpaVJrVG7jQfmj50otBAp "
        sigdata += "A0Zo7dpBU4ofv0N/F2Ar6LznCncIojkWptEJIAKA5tHegf/jY39arEpO "
        sigdata += "cevbGp6DKxFhlkLXNcw7k9o7DSw14OaRmgAjXdTFbrl4AiAa0zAttFko "
        sigdata += "Tso="
        rdata = f"{covers} 5 3 {ttl} {expiry} {inception} 12345 {domain} {sigdata}"
        return cls.create_rrset(owner, dns.rdatatype.RRSIG, rdata)

    def __init__(self, name_generator: ChainNameGenerator) -> None:
        self._name_generator = name_generator

    def get_rrsets(self) -> Tuple[List[dns.rrset.RRset], List[dns.rrset.RRset]]:
        """
        Return the lists of records and their signatures that should be
        generated in response to a given "action".

        This method is a wrapper around `generate_rrsets()` that ensures all
        derived classes obey their promises about the number of records they
        generate.
        """
        responses, signatures = self.generate_rrsets()
        assert len(responses) == self.response_count
        assert len(signatures) == self.response_count
        return responses, signatures

    @property
    @abc.abstractmethod
    def response_count(self) -> int:
        """
        How many records this generator creates each time the "action"
        associated with it is used.  Every generated record needs to be
        accompanied by its corresponding signature, so e.g. setting this to 1
        causes `get_rrsets()` callers to expect it to return two RRset lists,
        each containing one RRset.

        This property could be derived from the size of the lists returned by
        `generate_rrsets()`, but it is left as a separate value to enable early
        detection of invalid "selector" indexes when the control commands are
        first parsed.
        """
        raise NotImplementedError

    @abc.abstractmethod
    def generate_rrsets(self) -> Tuple[List[dns.rrset.RRset], List[dns.rrset.RRset]]:
        """
        Return the lists of records and their signatures that should be
        generated in response to a given "action".

        This method must be defined by every derived class, but RecordGenerator
        users should call `get_rrsets()` instead.
        """
        raise NotImplementedError


class CnameRecordGenerator(RecordGenerator):

    response_count = 1

    def generate_rrsets(self) -> Tuple[List[dns.rrset.RRset], List[dns.rrset.RRset]]:
        owner = self._name_generator.current_name
        target = self._name_generator.generate_next_name().to_text()
        response = self.create_rrset(owner, dns.rdatatype.CNAME, target)
        signature = self.create_rrset_signature(owner, response.rdtype)
        return [response], [signature]


class DnameRecordGenerator(RecordGenerator):

    response_count = 2

    def generate_rrsets(self) -> Tuple[List[dns.rrset.RRset], List[dns.rrset.RRset]]:
        dname_owner = self._name_generator.current_domain
        cname_owner = self._name_generator.current_name
        dname_target = self._name_generator.generate_next_sld().to_text()
        cname_target = self._name_generator.current_name.to_text()
        dname_response = self.create_rrset(
            dname_owner, dns.rdatatype.DNAME, dname_target
        )
        cname_response = self.create_rrset(
            cname_owner, dns.rdatatype.CNAME, cname_target
        )
        dname_signature = self.create_rrset_signature(
            dname_owner, dname_response.rdtype
        )
        cname_signature = self.create_rrset_signature(
            cname_owner, cname_response.rdtype
        )
        return [dname_response, cname_response], [dname_signature, cname_signature]


class XnameRecordGenerator(RecordGenerator):

    response_count = 1

    def generate_rrsets(self) -> Tuple[List[dns.rrset.RRset], List[dns.rrset.RRset]]:
        owner = self._name_generator.current_name
        target = self._name_generator.generate_next_name_in_next_sld().to_text()
        response = self.create_rrset(owner, dns.rdatatype.CNAME, target)
        signature = self.create_rrset_signature(owner, response.rdtype)
        return [response], [signature]


class FinalRecordGenerator(RecordGenerator):

    response_count = 1

    def generate_rrsets(self) -> Tuple[List[dns.rrset.RRset], List[dns.rrset.RRset]]:
        owner = self._name_generator.current_name
        response = self.create_rrset(owner, dns.rdatatype.A, "10.53.0.4")
        signature = self.create_rrset_signature(owner, response.rdtype)
        return [response], [signature]


class ChainAction(Enum):
    """
    Chained answer types that this server can send.  `ChainSetupCommand` sets
    up a collection of these for generating responses.
    """

    CNAME = CnameRecordGenerator
    DNAME = DnameRecordGenerator
    XNAME = XnameRecordGenerator
    FINAL = FinalRecordGenerator


@dataclass(frozen=True)
class ChainSelector:
    """
    A "selector" for a specific RRset - one of all possible RRsets generated by
    `ChainAction`s - to include in responses to queries.
    """

    response_index: int
    response_signature: bool


class ChainSetupCommand(ControlCommand):
    """
    Set up a chained response to return for subsequent queries.

    The control query consists of two label sequences separated by a `_` label.

    The first label sequence is a set of "actions"; these cause a set of
    response RRsets to be generated.  Valid labels in that sequence are:

      - `cname`: CNAME from the current name to a new one in the same domain,
      - `dname`: DNAME to a new domain, plus a synthesized CNAME,
      - `xname`: "external" CNAME, to a new name in a new domain.

    The final response to the client query (an A RRset) is automatically
    appended to the ANSWER section of every response.

    Example: `xname.dname.cname` represents a CNAME to an external domain which
    is then answered by a DNAME and a synthesized CNAME pointing to yet another
    domain, which is then answered by a CNAME within the same domain, and
    finally an answer to the query.

    Each of the generated RRsets is associated with a corresponding RRSIG.
    These signatures are not valid, but are intended to exercise the response
    parser.

    The second label sequence is a set of "selectors"; these specify which
    RRsets out of all the possible RRsets generated by "actions" to actually
    include in the answer and in what order.  The RRsets are indexed starting
    from 1.  If prepended with `s`, the number indicates which signature to
    include.

    Examples:

      - `cname.cname.cname._.1.s1.2.s2.3.s3.4.s4` indicates that all four
        RRsets (three CNAME RRsets + one A RRset with the final answer) should
        be included in the answer, with their corresponding signatures, in the
        original order,

      - `cname.cname.cname._.4.s4.3.s3.2.s2.1.s1` causes the same RRsets to be
        returned, but in reverse order,

      - `cname.cname.cname._.s3.s3.s3.s3` causes the RRSIG RRset for the third
        CNAME to be repeated four times in the response and everything else to
        be omitted.
    """

    control_subdomain = "setup-chain"

    def __init__(self) -> None:
        self._current_handler: Optional[ChainResponseHandler] = None

    def handle(
        self, args: List[str], server: ControllableAsyncDnsServer, qctx: QueryContext
    ) -> Optional[str]:
        try:
            actions, selectors = self._parse_args(args)
        except ValueError as exc:
            qctx.response.set_rcode(dns.rcode.SERVFAIL)
            logging.error("%s", exc)
            return str(exc)

        if self._current_handler:
            server.uninstall_response_handler(self._current_handler)

        answer_rrsets = self._prepare_answer(actions, selectors)

        self._current_handler = ChainResponseHandler(answer_rrsets)
        server.install_response_handler(self._current_handler)

        return "chain response setup successful"

    def _parse_args(
        self, args: List[str]
    ) -> Tuple[List[ChainAction], List[ChainSelector]]:
        try:
            delimiter = args.index("_")
        except ValueError as exc:
            raise ValueError("chain setup delimiter not found in QNAME") from exc

        args_actions = args[:delimiter]
        actions = self._parse_args_actions(args_actions)

        args_selectors = args[delimiter + 1 :]
        selectors = self._parse_args_selectors(args_selectors, actions)

        return actions, selectors

    def _parse_args_actions(self, args_actions: List[str]) -> List[ChainAction]:
        actions = []

        for action in args_actions + ["FINAL"]:
            try:
                actions.append(ChainAction[action.upper()])
            except KeyError as exc:
                raise ValueError(f"unsupported action '{action}'") from exc

        return actions

    def _parse_args_selectors(
        self, args_selectors: List[str], actions: List[ChainAction]
    ) -> List[ChainSelector]:
        max_response_index = self._get_max_response_index(actions)
        selectors = []

        for selector in args_selectors:
            match = re.match(r"^(?P<signature>s?)(?P<index>[0-9]+)$", selector)
            if not match:
                raise ValueError(f"invalid selector '{selector}'")
            response_index = int(match.group("index"))
            if response_index > max_response_index:
                raise ValueError(
                    f"invalid response index {response_index} in '{selector}'"
                )
            response_signature = bool(match.group("signature"))
            selectors.append(ChainSelector(response_index, response_signature))

        return selectors

    def _get_max_response_index(self, actions: List[ChainAction]) -> int:
        rrset_generator_classes = [a.value for a in actions]
        return sum(g.response_count for g in rrset_generator_classes)

    def _prepare_answer(
        self, actions: List[ChainAction], selectors: List[ChainSelector]
    ) -> List[dns.rrset.RRset]:
        all_responses, all_signatures = self._generate_rrsets(actions)
        return self._select_rrsets(all_responses, all_signatures, selectors)

    def _generate_rrsets(
        self, actions: List[ChainAction]
    ) -> Tuple[List[dns.rrset.RRset], List[dns.rrset.RRset]]:
        all_responses = []
        all_signatures = []
        name_generator = ChainNameGenerator()

        for action in actions:
            rrset_generator_class = action.value
            rrset_generator = rrset_generator_class(name_generator)
            responses, signatures = rrset_generator.get_rrsets()
            all_responses.extend(responses)
            all_signatures.extend(signatures)

        return all_responses, all_signatures

    def _select_rrsets(
        self,
        all_responses: List[dns.rrset.RRset],
        all_signatures: List[dns.rrset.RRset],
        selectors: List[ChainSelector],
    ) -> List[dns.rrset.RRset]:
        rrsets = []

        for selector in selectors:
            index = selector.response_index - 1
            source = all_signatures if selector.response_signature else all_responses
            rrsets.append(source[index])

        return rrsets


class ChainResponseHandler(DomainHandler):
    """
    For trigger queries (`test.domain.nil`), return a chained response
    previously prepared by `ChainSetupCommand`.

    For any other query, return a non-chained response (a single A RRset).
    """

    domains = ["domain.nil."]

    def __init__(self, answer_rrsets: List[dns.rrset.RRset]):
        super().__init__()
        self._answer_rrsets = answer_rrsets

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[ResponseAction, None]:
        trigger_qname = dns.name.from_text("test.domain.nil.")
        if qctx.qname == trigger_qname:
            answer_rrsets = self._answer_rrsets
        else:
            answer_rrsets = self._non_chain_answer(qctx)

        for rrset in answer_rrsets:
            qctx.response.answer.append(rrset)
        for rrset in self._authority_rrsets:
            qctx.response.authority.append(rrset)
        for rrset in self._additional_rrsets:
            qctx.response.additional.append(rrset)

        qctx.response.use_edns()
        yield DnsResponseSend(qctx.response)

    def _non_chain_answer(self, qctx: QueryContext) -> List[dns.rrset.RRset]:
        owner = qctx.qname
        return [
            RecordGenerator.create_rrset(owner, dns.rdatatype.A, "10.53.0.4"),
            RecordGenerator.create_rrset_signature(owner, dns.rdatatype.A),
        ]

    @property
    def _authority_rrsets(self) -> List[dns.rrset.RRset]:
        owner = dns.name.from_text("domain.nil.")
        return [
            RecordGenerator.create_rrset(owner, dns.rdatatype.NS, "ns1.domain.nil."),
        ]

    @property
    def _additional_rrsets(self) -> List[dns.rrset.RRset]:
        owner = dns.name.from_text("ns1.domain.nil.")
        return [
            RecordGenerator.create_rrset(owner, dns.rdatatype.A, "10.53.0.4"),
            RecordGenerator.create_rrset(
                owner, dns.rdatatype.AAAA, "fd92:7065:b8e:ffff::4"
            ),
        ]


def main() -> None:
    server = ControllableAsyncDnsServer(
        default_aa=True, default_rcode=dns.rcode.NOERROR
    )
    server.install_control_command(ChainSetupCommand())
    server.run()


if __name__ == "__main__":
    main()
