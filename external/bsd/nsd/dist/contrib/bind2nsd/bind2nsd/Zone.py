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
#	class to represent a zone file
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


class Zone:

   def __init__(self, name):
      self.name = name
      self.file = ''
      self.type = ''
      self.masters = []		# empty unless we're a slave zone
      self.allow_transfer = []
      self.also_notify = []
      self.allow_notify = []
      return

   def dump(self):
      report_info('=> Zone:')
      report_info('   name = %s' % (self.name))
      report_info('   file = %s' % (self.file))
      report_info('   type = %s' % (self.type))
      report_info('   masters = %s' % (str(self.masters)))
      report_info('   allow_notify = %s' % (str(self.allow_notify)))
      report_info('   allow_transfer = %s' % (str(self.allow_transfer)))
      report_info('   also_notify = %s' % (str(self.also_notify)))
      return

   def setName(self, name):
      self.name = name
      return

   def getName(self):
      return self.name

   def setFile(self, file):
      self.file = file
      return

   def getFile(self):
      return self.file

   def setType(self, type):
      self.type = type
      return

   def getType(self):
      return self.type

   def addMaster(self, quad):
      self.masters.append(quad)
      return

   def setMasters(self, list):
      self.masters = list
      return

   def getMasters(self):
      return self.masters

   def addAllowTransfer(self, quad):
      self.allow_transfer.append(quad)
      return

   def setAllowTransfers(self, list):
      self.allow_transfer = list
      return

   def getAllowTransfer(self):
      return self.allow_transfer

   def addAlsoNotify(self, quad):
      self.also_notify.append(quad)
      return

   def setAlsoNotify(self, list):
      self.also_notify = list
      return

   def getAlsoNotify(self):
      return self.also_notify

   def addAllowNotify(self, quad):
      self.allow_notify.append(quad)
      return

   def setAllowNotify(self, list):
      self.allow_notify = list
      return

   def getAllowNotify(self):
      return self.allow_notify

