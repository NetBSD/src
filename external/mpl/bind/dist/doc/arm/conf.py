############################################################################
# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# SPDX-License-Identifier: MPL-2.0
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, you can obtain one at https://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.
############################################################################

from pathlib import Path
import re
import sys

from typing import List, Tuple

from docutils import nodes
from docutils.nodes import Node, system_message
from docutils.parsers.rst import roles

from sphinx import addnodes

try:
    from sphinx.util.docutils import ReferenceRole
except ImportError:

    class ReferenceRole(roles.GenericRole):
        """
        The ReferenceRole class (used as a base class by GitLabRefRole
        below) is only defined in Sphinx >= 2.0.0.  For older Sphinx
        versions, this stub version of the ReferenceRole class is used
        instead.
        """

        def __init__(self):
            super().__init__("", nodes.strong)


GITLAB_BASE_URL = "https://gitlab.isc.org/isc-projects/bind9/-/"
KNOWLEDGEBASE_BASE_URL = "https://kb.isc.org/docs/"


# Custom Sphinx role enabling automatic hyperlinking to security advisory in
# ISC Knowledgebase
class CVERefRole(ReferenceRole):
    def __init__(self, base_url: str) -> None:
        self.base_url = base_url
        super().__init__()

    def run(self) -> Tuple[List[Node], List[system_message]]:
        cve_identifier = "(CVE-%s)" % self.target

        target_id = "index-%s" % self.env.new_serialno("index")
        entries = [
            ("single", "ISC Knowledgebase; " + cve_identifier, target_id, "", None)
        ]

        index = addnodes.index(entries=entries)
        target = nodes.target("", "", ids=[target_id])
        self.inliner.document.note_explicit_target(target)

        try:
            refuri = self.base_url + "cve-%s" % self.target
            reference = nodes.reference(
                "", "", internal=False, refuri=refuri, classes=["cve"]
            )
            if self.has_explicit_title:
                reference += nodes.strong(self.title, self.title)
            else:
                reference += nodes.strong(cve_identifier, cve_identifier)
        except ValueError:
            error_text = "invalid ISC Knowledgebase identifier %s" % self.target
            msg = self.inliner.reporter.error(error_text, line=self.lineno)
            prb = self.inliner.problematic(self.rawtext, self.rawtext, msg)
            return [prb], [msg]

        return [index, target, reference], []


# Custom Sphinx role enabling automatic hyperlinking to GitLab issues/MRs.
class GitLabRefRole(ReferenceRole):
    def __init__(self, base_url: str) -> None:
        self.base_url = base_url
        super().__init__()

    def run(self) -> Tuple[List[Node], List[system_message]]:
        gl_identifier = "[GL %s]" % self.target

        target_id = "index-%s" % self.env.new_serialno("index")
        entries = [("single", "GitLab; " + gl_identifier, target_id, "", None)]

        index = addnodes.index(entries=entries)
        target = nodes.target("", "", ids=[target_id])
        self.inliner.document.note_explicit_target(target)

        try:
            refuri = self.build_uri()
            reference = nodes.reference(
                "", "", internal=False, refuri=refuri, classes=["gl"]
            )
            if self.has_explicit_title:
                reference += nodes.strong(self.title, self.title)
            else:
                reference += nodes.strong(gl_identifier, gl_identifier)
        except ValueError:
            error_text = "invalid GitLab identifier %s" % self.target
            msg = self.inliner.reporter.error(error_text, line=self.lineno)
            prb = self.inliner.problematic(self.rawtext, self.rawtext, msg)
            return [prb], [msg]

        return [index, target, reference], []

    def build_uri(self):
        if self.target[0] == "#":
            return self.base_url + "issues/%d" % int(self.target[1:])
        if self.target[0] == "!":
            return self.base_url + "merge_requests/%d" % int(self.target[1:])
        raise ValueError


def setup(app):
    roles.register_local_role("cve", CVERefRole(KNOWLEDGEBASE_BASE_URL))
    roles.register_local_role("gl", GitLabRefRole(GITLAB_BASE_URL))
    app.add_crossref_type("iscman", "iscman", "pair: %s; manual page")


#
# Configuration file for the Sphinx documentation builder.
#
# This file only contains a selection of the most common options. For a full
# list see the documentation:
# https://www.sphinx-doc.org/en/master/config

# -- Path setup --------------------------------------------------------------

# If extensions (or modules to document with autodoc) are in another directory,
# add these directories to sys.path here. If the directory is relative to the
# documentation root, make it absolute.
#
sys.path.append(str(Path(__file__).resolve().parent / "_ext"))
sys.path.append(str(Path(__file__).resolve().parent.parent / "misc"))

# -- Project information -----------------------------------------------------

project = "BIND 9"
# pylint: disable=redefined-builtin
copyright = "2023, Internet Systems Consortium"
author = "Internet Systems Consortium"

m4_vars = {}
with open("../../configure.ac", encoding="utf-8") as configure_ac:
    for line in configure_ac:
        match = re.match(
            r"m4_define\(\[(?P<key>bind_VERSION_[A-Z]+)\], (?P<val>[^)]*)\)dnl", line
        )
        if match:
            m4_vars[match.group("key")] = match.group("val")

version = "%s.%s.%s%s" % (
    m4_vars["bind_VERSION_MAJOR"],
    m4_vars["bind_VERSION_MINOR"],
    m4_vars["bind_VERSION_PATCH"],
    m4_vars["bind_VERSION_EXTRA"],
)
release = version

# -- General configuration ---------------------------------------------------

# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
extensions = ["namedconf", "rndcconf"]

# Add any paths that contain templates here, relative to this directory.
templates_path = ["_templates"]

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
# This pattern also affects html_static_path and html_extra_path.
exclude_patterns = ["_build", "Thumbs.db", ".DS_Store", "*.inc.rst"]

# The master toctree document.
master_doc = "index"

smartquotes = False

# -- Options for HTML output -------------------------------------------------

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
#
html_theme = "sphinx_rtd_theme"
html_static_path = ["_static"]
html_css_files = ["custom.css"]

# -- Options for EPUB output -------------------------------------------------

epub_basename = "Bv9ARM"

# -- Options for LaTeX output ------------------------------------------------
latex_engine = "xelatex"

# pylint disable=line-too-long
latex_documents = [
    (
        master_doc,
        "Bv9ARM.tex",
        "BIND 9 Administrator Reference Manual",
        author,
        "manual",
    ),
]

latex_logo = "isc-logo.pdf"

# -- Options for linkcheck ----------------------------------------------
linkcheck_timeout = 10
linkcheck_ignore = [
    "http://127.0.0.1",
    "https://dl.acm.org",
    "https://gitlab.isc.org",
    "https://kb.isc.org",
    "https://simpleicon.com/",
    "https://www.dnssec-or-not.com/",
    "https://www.flaticon.com/",
    "https://www.freepik.com/",
    "https://www.gnu.org",
    "https://www.godaddy.com",
    "https://www.icann.org",
]
# Anchors checking does not work for GitHub, see
# https://github.com/pypa/packaging.python.org/issues/1272.
linkcheck_anchors_ignore_for_url = [
    "https://.*github.*",
]

#
# The rst_epilog will be completely overwritten from the Makefile,
# the definition here is provided purely for situations when
# sphinx-build is run by hand.
#
rst_epilog = """
.. |rndc_conf| replace:: ``/etc/rndc.conf``
.. |rndc_key| replace:: ``/etc/rndc.key``
.. |named_conf| replace:: ``/etc/named.conf``
.. |named_pid| replace:: ``/run/named.pid``
.. |session_key| replace:: ``/run/session.key``
"""
