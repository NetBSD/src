#!/usr/bin/env python
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
#	class to represent a named.conf key
#
#

import os
import os.path
import sys

if os.path.exists('../bind2nsd/Config.py'):
   sys.path.append('../bind2nsd')
   from Utils import *
else:
   from bind2nsd.Utils import *


class Key:

   def __init__(self, name):
      self.name = name
      self.algorithm = ''
      self.secret = ''
      self.ipaddrs = []
      return

   def dump(self):
      report_info('=> Key:')
      report_info('   algorithm = %s' % (self.algorithm))
      report_info('   name      = %s' % (self.name))
      report_info('   secret    = %s' % (self.secret))
      report_info('   ipaddrs   = %s' % (str(self.ipaddrs)))
      return

   def setName(self, val):
      self.name = val
      return

   def getName(self):
      return self.name

   def setAlgorithm(self, val):
      self.algorithm = val
      return

   def getAlgorithm(self):
      return self.algorithm

   def setSecret(self, val):
      self.secret = val
      return

   def getSecret(self):
      return self.secret

   def addIpAddr(self, addr):
      if addr not in self.ipaddrs:
         self.ipaddrs.append(addr)
      return
      
   def getIpAddrs(self):
      return self.ipaddrs

