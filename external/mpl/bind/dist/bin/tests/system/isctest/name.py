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

from collections.abc import Iterable

from dns.name import Name

import dns.name
import dns.rdatatype
import dns.zone


def prepend_label(label: str, name: Name) -> Name:
    return Name((label,) + name.labels)


def len_wire_uncompressed(name: Name) -> int:
    return len(name) + sum(map(len, name.labels))


def get_wildcard_names(names: Iterable[Name]) -> frozenset[Name]:
    return frozenset(name for name in names if name.is_wild())


class ZoneAnalyzer:
    """
    Categorize names in zone and provide list of ENTs:

    - delegations - names with NS RR
    - dnames - names with DNAME RR
    - wildcards - names with leftmost label '*'
    - reachable - non-empty authoritative nodes in zone
      - have at least one auth RR set and are not occluded
    - ents - reachable empty non-terminals
    - occluded - names under a parent node which has DNAME or a non-apex NS
    - reachable_delegations
      - have NS RR on it, are not zone's apex, and are not occluded
    - reachable_dnames - have DNAME RR on it and are not occluded
    - reachable_wildcards - have leftmost label '*' and are not occluded
    - reachable_wildcard_parents - reachable_wildcards with leftmost '*' stripped

    Warnings:
    - Quadratic complexity ahead! Use only on small test zones.
    - Zone must be constant.
    """

    @classmethod
    def read_path(cls, zpath, origin):
        with open(zpath, encoding="ascii") as zf:
            zonedb = dns.zone.from_file(zf, origin, relativize=False)
        return cls(zonedb)

    def __init__(self, zone: dns.zone.Zone):
        self.zone = zone
        assert self.zone.origin  # mypy hack
        # based on individual nodes but not relationship between nodes
        self.delegations = self.get_names_with_type(dns.rdatatype.NS) - {
            self.zone.origin
        }
        self.dnames = self.get_names_with_type(dns.rdatatype.DNAME)
        self.wildcards = get_wildcard_names(self.zone)

        # takes relationship between nodes into account
        self._categorize_names()
        self.ents = self.generate_ents()
        self.reachable_dnames = self.dnames.intersection(self.reachable)
        self.reachable_wildcards = self.wildcards.intersection(self.reachable)
        self.reachable_wildcard_parents = {
            Name(wname[1:]) for wname in self.reachable_wildcards
        }

        # (except for wildcard expansions) all names in zone which result in NOERROR answers
        self.all_existing_names = (
            self.reachable.union(self.ents)
            .union(self.reachable_delegations)
            .union(self.reachable_dnames)
        )

    def get_names_with_type(self, rdtype) -> frozenset[Name]:
        return frozenset(
            name for name in self.zone if self.zone.get_rdataset(name, rdtype)
        )

    def _categorize_names(
        self,
    ) -> None:
        """
        Split names defined in a zone into three sets:
        Generally reachable, reachable delegations, and occluded.

        Delegations are set aside because they are a weird hybrid with different
        rules for different RR types (NS, DS, NSEC, everything else).
        """
        assert self.zone.origin  # mypy workaround
        reachable = set(self.zone)
        # assume everything is reachable until proven otherwise
        reachable_delegations = set(self.delegations)
        occluded = set()

        def _mark_occluded(name: Name) -> None:
            occluded.add(name)
            if name in reachable:
                reachable.remove(name)
            if name in reachable_delegations:
                reachable_delegations.remove(name)

        # sanity check, should be impossible with dnspython 2.7.0 zone reader
        for name in reachable:
            relation, _, _ = name.fullcompare(self.zone.origin)
            if relation in (
                dns.name.NameRelation.NONE,  # out of zone?
                dns.name.NameRelation.SUPERDOMAIN,  # parent of apex?!
            ):
                raise NotImplementedError

        for maybe_occluded in reachable.copy():
            for cut in self.delegations:
                rel, _, _ = maybe_occluded.fullcompare(cut)
                if rel == dns.name.NameRelation.EQUAL:
                    # data _on_ a parent-side of a zone cut are in limbo:
                    # - are not really authoritative (except for DS)
                    # - but NS is not really 'occluded'
                    # We remove them from 'reachable' but do not add them to 'occluded' set,
                    # i.e. leave them in 'reachable_delegations'.
                    if maybe_occluded in reachable:
                        reachable.remove(maybe_occluded)

                if rel == dns.name.NameRelation.SUBDOMAIN:
                    _mark_occluded(maybe_occluded)
                # do not break cycle - handle also nested NS and DNAME

            # DNAME itself is authoritative but nothing under it is reachable
            for dname in self.dnames:
                rel, _, _ = maybe_occluded.fullcompare(dname)
                if rel == dns.name.NameRelation.SUBDOMAIN:
                    _mark_occluded(maybe_occluded)
                # do not break cycle - handle also nested NS and DNAME

        self.reachable = frozenset(reachable)
        self.reachable_delegations = frozenset(reachable_delegations)
        self.occluded = frozenset(occluded)

    def generate_ents(self) -> frozenset[Name]:
        """
        Generate reachable names of empty nodes "between" all reachable
        names with a RR and the origin.
        """
        assert self.zone.origin
        all_reachable_names = self.reachable.union(self.reachable_delegations)
        ents = set()
        for name in all_reachable_names:
            _, super_name = name.split(len(name) - 1)
            while len(super_name) > len(self.zone.origin):
                if super_name not in all_reachable_names:
                    ents.add(super_name)
                _, super_name = super_name.split(len(super_name) - 1)

        return frozenset(ents)

    def closest_encloser(self, qname: Name):
        """
        Get (closest encloser, next closer name) for given qname.
        """
        ce = None  # Closest encloser, RFC 4592
        nce = None  # Next closer name, RFC 5155
        for zname in self.all_existing_names:
            relation, _, common_labels = qname.fullcompare(zname)
            if relation == dns.name.NameRelation.SUBDOMAIN:
                if not ce or common_labels > len(ce):
                    # longest match so far
                    ce = zname
                    _, nce = qname.split(len(ce) + 1)
        assert ce is not None
        assert nce is not None
        return ce, nce

    def source_of_synthesis(self, qname: Name) -> Name:
        """
        Return source of synthesis according to RFC 4592 section 3.3.1.
        Name is not guaranteed to exist or be reachable.
        """
        ce, _ = self.closest_encloser(qname)
        return Name("*") + ce
