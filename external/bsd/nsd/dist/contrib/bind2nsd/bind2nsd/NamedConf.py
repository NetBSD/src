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
#	class to represent a named.conf file
#

import os
import os.path
import sys

if os.path.exists('../bind2nsd/Config.py'):
   sys.path.append('../bind2nsd')
   from Config import *
   from Parser import *
   from Zone import *
   from Utils import *
else:
   from bind2nsd.Config import *
   from bind2nsd.Parser import *
   from bind2nsd.Zone import *
   from bind2nsd.Utils import *


class Options:
   def __init__(self):
      self.allow_notify = []
      self.allow_recursion = []
      self.allow_transfer = []
      self.also_notify = []
      self.check_names = []
      self.coresize = ""
      self.directory = ""
      self.dump_file = ""
      self.pid_file = ""
      self.recursive_clients = ""
      self.statistics_file = ""
      self.transfer_format = ""
      self.version = ""
      return

   def dump(self):
      report_info('=> Options:')
      report_info('   allow-notify:      %s' % (self.allow_notify))
      report_info('   allow-recursion:   %s' % (self.allow_recursion))
      report_info('   allow-transfer:    %s' % (self.allow_transfer))
      report_info('   also-notify:       %s' % (self.also_notify))
      report_info('   check-names:       %s' % (str(self.check_names)))
      report_info('   coresize:          %s' % (self.coresize))
      report_info('   directory:         %s' % (self.directory))
      report_info('   dump-file:         %s' % (self.dump_file))
      report_info('   pid-file:          %s' % (self.pid_file))
      report_info('   recursive-clients: %s' % (self.recursive_clients))
      report_info('   statistics-file:   %s' % (self.statistics_file))
      report_info('   transfer-format:   %s' % (self.transfer_format))
      report_info('   version:           %s' % (self.version))
      return

   def setDirectory(self, name):
      self.directory = name
      return

   def getDirectory(self):
      return self.directory

   def setDumpFile(self, name):
      self.dump_file = name
      return

   def getDumpFile(self):
      return self.dump_file

   def setPidFile(self, name):
      self.pid_file = name
      return

   def getPidFile(self):
      return self.pid_file

   def setStatisticsFile(self, name):
      self.statistics_file = name
      return

   def getStatisticsFile(self):
      return self.statistics_file

   def addAllowNotify(self, quad):
      self.allow_notify.append(quad)
      return

   def setAllowNotify(self, list):
      self.allow_notify = list
      return

   def getAllowNotify(self):
      return self.allow_notify

   def addAllowTransfer(self, quad):
      self.allow_transfer.append(quad)
      return

   def setAllowTransfer(self, list):
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

   def addAllowRecursion(self, quad):
      self.allow_recursion.append(quad)
      return

   def setAllowRecursion(self, list):
      self.allow_recursion = list
      return

   def getAllowRecursion(self):
      return self.allow_recursion

   def addCheckNames(self, val):
      self.check_names.append(val)
      return

   def getCheckNames(self):
      return self.check_names

   def setCoresize(self, val):
      self.coresize = val
      return

   def getCoresize(self):
      return self.coresize

   def setTransferFormat(self, val):
      self.transfer_format = val
      return

   def getTransferFormat(self):
      return self.transfer_format

   def setVersion(self, val):
      self.version = val
      return

   def getVersion(self):
      return self.version

   def setRecursiveClients(self, val):
      self.recursive_clients = val
      return

   def getRecursiveClients(self):
      return self.recursive_clients


class NamedConf:

   def __init__(self, fname):
      self.fname = fname
      self.options = Options()
      self.includes = []
      self.zones = {}
      self.keys = {}

      parser = Parser(self)
      parser.parse(Tokenizer(self.fname))
      return

   def dump(self):
      report_info('=> NamedConf: dumping data from \"%s\"...' % (self.fname))
      self.options.dump()
      report_info('=> NamedConf: list of includes...')
      report_info(str(self.includes))

      report_info('=> NamedConf: zone info...')
      zlist = self.zones.keys()
      zlist.sort()
      for ii in zlist:
         self.zones[ii].dump()
   
      report_info('=> NamedConf: key info...')
      klist = self.keys.keys()
      klist.sort()
      for ii in klist:
         self.keys[ii].dump()

      return

   def getOptions(self):
      return self.options

   def addZone(self, zone):
      name = zone.getName()
      self.zones[name] = zone
      return

   def getZones(self):
      return self.zones

   def getZone(self, name):
      if name in self.zones:
         return self.zones[name]
      else:
         return None

   def addKey(self, key):
      name = key.getName()
      self.keys[name] = key
      return

   def getKey(self, name):
      if name in self.keys:
         return self.keys[name]
      else:
         return None

   def getKeys(self):
      return self.keys

