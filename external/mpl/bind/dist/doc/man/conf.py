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

import datetime
from docutils.parsers.rst import roles

#
# Configuration file for the Sphinx documentation builder.
#
# This file only contains a selection of the most common options. For a full
# list see the documentation:
# http://www.sphinx-doc.org/en/master/config

# -- Path setup --------------------------------------------------------------

# If extensions (or modules to document with autodoc) are in another directory,
# add these directories to sys.path here. If the directory is relative to the
# documentation root, use os.path.abspath to make it absolute, like shown here.
#
# import os
# import sys
# sys.path.insert(0, os.path.abspath('.'))

# -- Project information -----------------------------------------------------

project = "BIND 9"
# pylint: disable=wrong-import-position
year = datetime.datetime.now().year
# pylint: disable=redefined-builtin
copyright = "%d, Internet Systems Consortium" % year
author = "Internet Systems Consortium"

# -- General configuration ---------------------------------------------------

# Build man pages directly in _build/man/, not in _build/man/<section>/.
# This is what the shell code in Makefile.am expects.
man_make_section_directory = False

# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
extensions = []

# Add any paths that contain templates here, relative to this directory.
templates_path = ["../arm/_templates"]

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
# This pattern also affects html_static_path and html_extra_path.
exclude_patterns = [
    "_build",
    "Thumbs.db",
    ".DS_Store",
]

# The master toctree document.
master_doc = "index"

# pylint: disable=line-too-long
man_pages = [
    (
        "arpaname",
        "arpaname",
        "translate IP addresses to the corresponding ARPA names",
        author,
        1,
    ),
    ("ddns-confgen", "ddns-confgen", "ddns key generation tool", author, 8),
    ("delv", "delv", "DNS lookup and validation utility", author, 1),
    ("dig", "dig", "DNS lookup utility", author, 1),
    (
        "dnssec-cds",
        "dnssec-cds",
        "change DS records for a child zone based on CDS/CDNSKEY",
        author,
        8,
    ),
    (
        "dnssec-checkds",
        "dnssec-checkds",
        "DNSSEC delegation consistency checking tool",
        author,
        8,
    ),
    (
        "dnssec-coverage",
        "dnssec-coverage",
        "checks future DNSKEY coverage for a zone",
        author,
        8,
    ),
    ("dnssec-dsfromkey", "dnssec-dsfromkey", "DNSSEC DS RR generation tool", author, 8),
    (
        "dnssec-importkey",
        "dnssec-importkey",
        "import DNSKEY records from external systems so they can be managed",
        author,
        8,
    ),
    (
        "dnssec-keyfromlabel",
        "dnssec-keyfromlabel",
        "DNSSEC key generation tool",
        author,
        8,
    ),
    ("dnssec-keygen", "dnssec-keygen", "DNSSEC key generation tool", author, 8),
    (
        "dnssec-keymgr",
        "dnssec-keymgr",
        "ensure correct DNSKEY coverage based on a defined policy",
        author,
        8,
    ),
    (
        "dnssec-revoke",
        "dnssec-revoke",
        "set the REVOKED bit on a DNSSEC key",
        author,
        8,
    ),
    (
        "dnssec-settime",
        "dnssec-settime",
        "set the key timing metadata for a DNSSEC key",
        author,
        8,
    ),
    ("dnssec-signzone", "dnssec-signzone", "DNSSEC zone signing tool", author, 8),
    ("dnssec-verify", "dnssec-verify", "DNSSEC zone verification tool", author, 8),
    (
        "dnstap-read",
        "dnstap-read",
        "print dnstap data in human-readable form",
        author,
        1,
    ),
    (
        "filter-aaaa",
        "filter-aaaa",
        "filter AAAA in DNS responses when A is present",
        author,
        8,
    ),
    ("host", "host", "DNS lookup utility", author, 1),
    ("mdig", "mdig", "DNS pipelined lookup utility", author, 1),
    (
        "named-checkconf",
        "named-checkconf",
        "named configuration file syntax checking tool",
        author,
        8,
    ),
    (
        "named-checkzone",
        "named-checkzone",
        "zone file validity checking or converting tool",
        author,
        8,
    ),
    (
        "named-compilezone",
        "named-compilezone",
        "zone file validity checking or converting tool",
        author,
        8,
    ),
    (
        "named-journalprint",
        "named-journalprint",
        "print zone journal in human-readable form",
        author,
        8,
    ),
    (
        "named-nzd2nzf",
        "named-nzd2nzf",
        "convert an NZD database to NZF text format",
        author,
        8,
    ),
    (
        "named-rrchecker",
        "named-rrchecker",
        "syntax checker for individual DNS resource records",
        author,
        1,
    ),
    ("named.conf", "named.conf", "configuration file for **named**", author, 5),
    ("named", "named", "Internet domain name server", author, 8),
    ("nsec3hash", "nsec3hash", "generate NSEC3 hash", author, 8),
    ("nslookup", "nslookup", "query Internet name servers interactively", author, 1),
    ("nsupdate", "nsupdate", "dynamic DNS update utility", author, 1),
    ("pkcs11-destroy", "pkcs11-destroy", "destroy PKCS#11 objects", author, 8),
    ("pkcs11-keygen", "pkcs11-keygen", "generate keys on a PKCS#11 device", author, 8),
    ("pkcs11-list", "pkcs11-list", "list PKCS#11 objects", author, 8),
    ("pkcs11-tokens", "pkcs11-tokens", "list PKCS#11 available tokens", author, 8),
    ("rndc-confgen", "rndc-confgen", "rndc key generation tool", author, 8),
    ("rndc.conf", "rndc.conf", "rndc configuration file", author, 5),
    ("rndc", "rndc", "name server control utility", author, 8),
    ("tsig-keygen", "tsig-keygen", "TSIG key generation tool", author, 8),
]


def setup(app):
    app.add_crossref_type("iscman", "iscman", "pair: %s; manual page")
    # ignore :option: references to simplify doc backports to v9_16 branch
    app.add_role_to_domain("std", "option", roles.code_role)
