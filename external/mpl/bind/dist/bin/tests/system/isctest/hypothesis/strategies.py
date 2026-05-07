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

from warnings import warn

import collections.abc

from hypothesis.strategies import (
    binary,
    builds,
    composite,
    integers,
    just,
    nothing,
    permutations,
    sampled_from,
)

import dns.name
import dns.rdataclass
import dns.rdatatype

import isctest.name


@composite
def dns_names(
    draw,
    *,
    prefix: dns.name.Name = dns.name.empty,
    suffix: dns.name.Name | collections.abc.Iterable[dns.name.Name] = dns.name.root,
    min_labels: int = 1,
    max_labels: int = 128,
) -> dns.name.Name:
    """
    This is a hypothesis strategy to be used for generating DNS names with given `prefix`, `suffix`
    and with total number of labels specified by `min_labels` and `max labels`.

    For example, calling
    ```
    dns_names(
        prefix=dns.name.from_text("test"),
        suffix=dns.name.from_text("isc.org"),
        max_labels=6
    ).example()
    ```
    will result in names like `test.abc.isc.org.` or `test.abc.def.isc.org`.

    There is no attempt to make the distribution of the generated names uniform in any way.
    The strategy however minimizes towards shorter names with shorter labels.

    It can be used with to build compound strategies, like this one which generates random DNS queries.

    ```
    dns_queries = builds(
        dns.message.make_query,
        qname=dns_names(),
        rdtype=dns_rdatatypes,
        rdclass=dns_rdataclasses,
    )
    ```
    """

    prefix = prefix.relativize(dns.name.root)
    # Python str is iterable, but that's most probably not what user actually wanted
    if isinstance(suffix, str):
        raise NotImplementedError(
            "ambiguous API use, convert suffix to Name or list to express intent"
        )
    if isinstance(suffix, collections.abc.Iterable):
        suffix = draw(sampled_from(sorted(suffix)))
    assert isinstance(suffix, dns.name.Name)
    suffix = suffix.derelativize(dns.name.root)

    try:
        outer_name = prefix + suffix
        remaining_bytes = 255 - isctest.name.len_wire_uncompressed(outer_name)
        assert remaining_bytes >= 0
    except dns.name.NameTooLong:
        warn(
            "Maximal length name of name execeeded by prefix and suffix. Strategy won't generate any names.",
            RuntimeWarning,
        )
        return draw(nothing())

    minimum_number_of_labels_to_generate = max(0, min_labels - len(outer_name.labels))
    maximum_number_of_labels_to_generate = max_labels - len(outer_name.labels)
    if maximum_number_of_labels_to_generate < 0:
        warn(
            "Maximal number of labels execeeded by prefix and suffix. Strategy won't generate any names.",
            RuntimeWarning,
        )
        return draw(nothing())

    maximum_number_of_labels_to_generate = min(
        maximum_number_of_labels_to_generate, remaining_bytes // 2
    )
    if maximum_number_of_labels_to_generate < minimum_number_of_labels_to_generate:
        warn(
            f"Minimal number set to {minimum_number_of_labels_to_generate}, but in {remaining_bytes} bytes there is only space for maximum of {maximum_number_of_labels_to_generate} labels.",
            RuntimeWarning,
        )
        return draw(nothing())

    if remaining_bytes == 0 or maximum_number_of_labels_to_generate == 0:
        warn(
            f"Strategy will return only one name ({outer_name}) as it exactly matches byte or label length limit.",
            RuntimeWarning,
        )
        return draw(just(outer_name))

    chosen_number_of_labels_to_generate = draw(
        integers(
            minimum_number_of_labels_to_generate, maximum_number_of_labels_to_generate
        )
    )
    chosen_number_of_bytes_to_partion = draw(
        integers(2 * chosen_number_of_labels_to_generate, remaining_bytes)
    )
    chosen_lengths_of_labels = draw(
        _partition_bytes_to_labels(
            chosen_number_of_bytes_to_partion, chosen_number_of_labels_to_generate
        )
    )
    generated_labels = tuple(
        draw(binary(min_size=l - 1, max_size=l - 1)) for l in chosen_lengths_of_labels
    )

    return dns.name.Name(prefix.labels + generated_labels + suffix.labels)


RDATACLASS_MAX = RDATATYPE_MAX = 65535
dns_rdataclasses = builds(dns.rdataclass.RdataClass, integers(0, RDATACLASS_MAX))
dns_rdatatypes = builds(dns.rdatatype.RdataType, integers(0, RDATATYPE_MAX))
dns_rdataclasses_without_meta = dns_rdataclasses.filter(dns.rdataclass.is_metaclass)

# NOTE: This should really be `dns_rdatatypes_without_meta = dns_rdatatypes_without_meta.filter(dns.rdatatype.is_metatype()`,
#       but hypothesis then complains about the filter being too strict, so it is done in a “constructive” way.
dns_rdatatypes_without_meta = integers(0, dns.rdatatype.OPT - 1) | integers(dns.rdatatype.OPT + 1, 127) | integers(256, RDATATYPE_MAX)  # type: ignore


@composite
def _partition_bytes_to_labels(
    draw, remaining_bytes: int, number_of_labels: int
) -> list[int]:
    two_bytes_reserved_for_label = 2

    # Reserve two bytes for each label
    partition = [two_bytes_reserved_for_label] * number_of_labels
    remaining_bytes -= two_bytes_reserved_for_label * number_of_labels

    assert remaining_bytes >= 0

    # Add a random number between 0 and the remainder to each partition
    for i in range(number_of_labels):
        added = draw(
            integers(0, min(remaining_bytes, 64 - two_bytes_reserved_for_label))
        )
        partition[i] += added
        remaining_bytes -= added

    # NOTE: Some of the remaining bytes will usually not be assigned to any label, but we don't care.

    return draw(permutations(partition))


def uint(byte_size: int):
    max_value = 2**byte_size - 1
    return integers(min_value=0, max_value=max_value)
