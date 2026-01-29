#!/usr/bin/python3
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
"""
isctest.name.ZoneAnalyzer self-test
Generate insane test zone and check expected output of ZoneAnalyzer utility class
"""


import collections
import itertools
from pathlib import Path

import dns.name
from dns.name import Name
import pytest

import isctest
import isctest.name

pytest.importorskip("dns", minversion="2.3.0")

# set of properies present in the tested zone - read by tests_zone_analyzer.py
CATEGORIES = frozenset(
    [
        "all_existing_names",
        "delegations",
        "dnames",
        "ents",
        "occluded",
        "reachable",
        "reachable_delegations",
        "reachable_dnames",
        "reachable_wildcards",
        "reachable_wildcard_parents",
        "wildcards",
    ]
)


pytestmark = pytest.mark.extra_artifacts(["analyzer.db"])
SUFFIX = dns.name.from_text("nsec3.example.")

LABELS = (b"*", b"dname", b"ent", b"ns", b"txt")
LABEL2RRTYPE = {  # leftmost label encodes RR type we will synthesize for given name
    b"*": "TXT",
    b"dname": "DNAME",
    b"ent": None,  # ENT is not really a 'type'
    b"ns": "NS",
    b"txt": "TXT",
}
LABEL2TAGS = {  # leftmost label encodes 'initial' meaning of a complete name
    b"*": {"wildcards"},
    b"dname": {"dnames"},
    b"ns": {"delegations"},
    b"txt": set(),  # perhaps reachable, perhaps not, we need to decide based on other labels
}


def name2tags(name):
    """
    Decode meaning hidden in labels and their relationships
    and return all tags expected from ZoneAnalyzer
    """
    tags = LABEL2TAGS[name[0]].copy()

    parent_labels = name[1:]
    if b"ns" in parent_labels or b"dname" in parent_labels:
        tags.add("occluded")

    if "occluded" not in tags:
        tags.add("all_existing_names")
        if "delegations" in tags:
            # delegations are ambiguous and don't count as 'reachable'
            tags.add("reachable_delegations")
        elif "dnames" in tags:
            tags.add("reachable")
            tags.add("reachable_dnames")
        elif "wildcards" in tags:
            tags.add("reachable")
            tags.add("reachable_wildcards")
        else:
            tags.add("reachable")

    return tags


def gen_node(nodes, labels):
    name = Name(labels)
    nodes[name] = name2tags(name)


def add_ents(nodes):
    """
    Non-occluded nodes with 'ent' as a parent label imply existence of 'ent' nodes.
    """
    new_ents = {}
    for name, tags in nodes.items():
        if "occluded" in tags:
            continue

        # check if any parent is ENT
        entidx = 1
        while True:
            try:
                entidx = name.labels.index(b"ent", entidx)
            except ValueError:
                break
            entname = Name(name[entidx:])
            new_ents[entname] = {"all_existing_names", "ents"}
            entidx += 1

    return new_ents


def tag_wildcard_parents(nodes):
    """
    Non-occluded nodes with '*' as a leftmost label tag their immediate parent
    nodes as 'reachable_wildcard_parents'.
    """
    for name, tags in nodes.items():
        if "occluded" in tags or not name.is_wild():
            continue

        parent_name = Name(name[1:])
        nodes[parent_name].add("reachable_wildcard_parents")


def is_non_ent(labels):
    """
    Filter out nodes with 'ent' at leftmost position. To become ENT a name must
    not have data by itself but have some other node defined underneath it,
    and must not be occluded, which is something itertools.product() cannot
    decide.
    """
    return labels[0] != b"ent"


def gen_zone(nodes):
    """
    Generate zone file in text format.

    All names are relative.
    Right-hand side of RRs contains dot-separated list of categories a node
    belongs to (except for zone origin).
    """
    for name, tags in sorted(nodes.items()):
        if len(name) == 0:
            # origin, very special case
            yield "@\tSOA\treachable. origin-special-case. 0 0 0 0 0\n"
            yield "@\tNS\treachable.\n"
            yield "@\tA\t192.0.2.1\n"
            continue

        rrtype = LABEL2RRTYPE[name[0]]
        if rrtype is None:  # ENT
            prefix = "; "
        else:
            prefix = ""
        assert tags
        yield f"{prefix}{name}\t{rrtype}\t{'.'.join(sorted(tags))}.\n"


def gen_expected_output(nodes):
    """
    {category: set(names)} mapping used by the pytest check
    """
    categories = collections.defaultdict(set)
    for name, tags in nodes.items():
        for tag in tags:
            categories[tag].add(name)

    assert set(categories.keys()) == CATEGORIES, (
        "CATEGORIES needs updating",
        CATEGORIES.symmetric_difference(set(categories.keys())),
    )

    return categories


def generate_test_data():
    """
    Prepare the analyzer.db zone file in the current working directory and
    return the expected attribute values for the ZoneAnalyzer instance that
    will be tested using that file.
    """
    nodes = {}

    for length in range(1, len(LABELS) + 1):
        for labelseq in filter(is_non_ent, itertools.product(LABELS, repeat=length)):
            gen_node(nodes, labelseq)

    # special-case to make this look as a valid DNS zone - it needs zone origin node
    nodes[Name([])] = {"all_existing_names", "reachable"}

    nodes.update(add_ents(nodes))
    tag_wildcard_parents(nodes)

    with open("analyzer.db", "w", encoding="ascii") as outf:
        outf.writelines(gen_zone(nodes))

    return gen_expected_output(nodes)


if __name__ == "__main__":
    generate_test_data()


@pytest.fixture(scope="module")
def analyzer_fixture():
    expected_results = generate_test_data()  # creates the "analyzer.db" file
    analyzer = isctest.name.ZoneAnalyzer.read_path(Path("analyzer.db"), origin=SUFFIX)
    return expected_results, analyzer


# pylint: disable=redefined-outer-name
@pytest.mark.parametrize("category", sorted(CATEGORIES))
def test_analyzer_attrs(category, analyzer_fixture):
    expected_results, analyzer = analyzer_fixture
    # relativize results to zone name to make debugging easier
    results = {name.relativize(SUFFIX) for name in getattr(analyzer, category)}
    assert results == expected_results[category]
