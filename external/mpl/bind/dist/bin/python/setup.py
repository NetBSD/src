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

from distutils.core import setup
setup(name='isc',
      version='2.0',
      description='Python functions to support BIND utilities',
      url='https://www.isc.org/bind',
      author='Internet Systems Consortium, Inc',
      author_email='info@isc.org',
      license='MPL',
      requires=['ply'],
      packages=['isc'])
