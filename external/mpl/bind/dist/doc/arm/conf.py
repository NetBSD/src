############################################################################
# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.
############################################################################

# flake8: noqa: E501

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

project = u'BIND 9'
# pylint: disable=redefined-builtin
copyright = u'2020, Internet Systems Consortium'
author = u'Internet Systems Consortium'

# -- General configuration ---------------------------------------------------

# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
extensions = []

# Add any paths that contain templates here, relative to this directory.
templates_path = ['_templates']

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
# This pattern also affects html_static_path and html_extra_path.
exclude_patterns = [
    '_build',
    'Thumbs.db',
    '.DS_Store',
    '*.grammar.rst',
    '*.zoneopts.rst',
    'catz.rst',
    'dlz.rst',
    'dnssec.rst',
    'dyndb.rst',
    'logging-cattegories.rst',
    'managed-keys.rst',
    'pkcs11.rst',
    'plugins.rst'
    ]

# The master toctree document.
master_doc = 'index'

# -- Options for HTML output -------------------------------------------------

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
#
html_theme = 'sphinx_rtd_theme'

# -- Options for LaTeX output ------------------------------------------------
latex_engine = 'xelatex'

# pylint disable=line-too-long
latex_documents = [
    (master_doc, 'Bv9ARM.tex', u'BIND 9 Administrator Reference Manual', author, 'manual'),
    ]

latex_logo = "isc-logo.pdf"
