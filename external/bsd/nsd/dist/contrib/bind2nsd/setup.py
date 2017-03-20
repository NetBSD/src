#
# Copyright (c) 2007, Secure64 Software Corporation
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
# 
#
#
#    installation/setup script for bind2nsd
#

from distutils.core import setup

setup(name = 'bind2nsd',
      version = '0.5.0',
      author = 'Secure64 Software Corporation',
      author_email = 'support@secure64.com',
      maintainer = 'Al Stone',
      maintainer_email = 'ahs3@secure64.com',
      url = 'http://www.secure64.com',
      description = 'automatic named/nsd translation and synchronization',
      packages = [ 'bind2nsd' ],
      scripts = [ 'scripts/bind2nsd', 'scripts/s64-sync',
                  'scripts/s64-mkpw', ],
      data_files = [ ('/usr/share/doc/bind2nsd', ['./README', './TODO']),
                     ('/etc/bind2nsd', ['etc/bind2nsd.conf']),
		   ]
     )

