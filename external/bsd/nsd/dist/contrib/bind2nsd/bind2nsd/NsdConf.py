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
#	class to represent a nsd.conf file
#
#

import os.path
import sys

if os.path.exists('../bind2nsd/Config.py'):
   sys.path.append('../bind2nsd')
   from Utils import *
else:
   from bind2nsd.Utils import *


class NsdKey:

   def __init__(self, name):
      self.name = name
      self.algorithm = ''
      self.secret = ''
      return

   def dump(self, ofile):
      if ofile == sys.stdout:
         print >> ofile, '=> NsdKey:'
         print >> ofile,'   name      = %s' % (self.name)
         print >> ofile,'   algorithm = %s' % (self.algorithm)
         print >> ofile,'   secret    = %s' % (self.secret)
      else:
         print >> ofile, 'key:'
         print >> ofile,'   name: "%s"' % (self.name)
         print >> ofile,'   algorithm: %s' % (self.algorithm)
         print >> ofile,'   secret: "%s"' % (self.secret)
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

class NsdZone:

   def __init__(self, name, config, ipkeymap):
      self.name = name
      self.config = config
      self.include = ''
      self.zonefile = ''
      self.type = ''
      self.allow_notify = []		# empty unless we're a slave zone
      self.also_notify = []		# empty unless we're a master zone
      self.provide_xfr = []
      self.oldrootdir = ''
      self.ipkeymap = ipkeymap
      return

   def dump(self, ofile):
      if ofile == sys.stdout:
         print >> ofile, '=> NsdZone:'
         print >> ofile, '   name = %s' % (self.name)
         print >> ofile, '   include = %s' % (self.include)
         print >> ofile, '   zonefile = %s' % (self.zonefile)
         print >> ofile, '   type = %s' % (self.type)
         print >> ofile, '   allow_notify = %s' % (str(self.allow_notify))
         print >> ofile, '   also_notify = %s' % (str(self.also_notify))
         print >> ofile, '   provide_xfr = %s' % (str(self.provide_xfr))
      else:
         print >> ofile, ''
         print >> ofile, 'zone:'
         print >> ofile, '   name:        "%s"' % (self.name)
         print >> ofile, '   zonefile:    "%s"' % (self.zonefile)

	 # BOZO: this is a hack to avoid errors in the zone compiler
	 # and needs to be handled more intelligently; this allows the
	 # zone file to be non-existent (done by some ISPs) but still
	 # listed in the named.conf
	 if not os.path.exists(self.oldrootdir + self.zonefile):
	    report_error('? missing zone file "%s"' % \
	                 (self.oldrootdir + self.zonefile))

         nlist = self.allow_notify
	 if self.type == 'slave':
	    print >> ofile, '   # this is a slave zone.  masters are listed next.'
	    for ii in nlist:
	       print >> ofile, '   allow-notify: %s NOKEY' % (ii)
	       print >> ofile, '   request-xfr:  %s NOKEY' % (ii)
	 else:
            print >> ofile, '   include:     "%s"' % (self.include)
            for ii in self.also_notify:
	       print >> ofile, '   notify: %s NOKEY' % (ii)

	 for ii in self.provide_xfr:
	    if ii in self.ipkeymap:
	       print >> ofile, '   provide-xfr: %s %s' % (ii, self.ipkeymap[ii])
	       print >> ofile, '   notify: %s %s' % (ii, self.ipkeymap[ii])
	    else:
	       print >> ofile, '   provide-xfr: %s NOKEY' % (ii)
	       print >> ofile, '   notify: %s NOKEY' % (ii)

      return

   def setName(self, name):
      self.name = name
      return

   def getName(self):
      return self.name

   def setOldRootDir(self, name):
      self.oldrootdir = name
      return

   def getOldRootDir(self):
      return self.oldrootdir

   def setZonefile(self, file):
      self.zonefile = file
      return

   def getZonefile(self):
      return self.zonefile

   def setType(self, type):
      self.type = type
      return

   def getType(self):
      return self.type

   def addMaster(self, quad):
      self.masters.append(quad)
      return

   def getMasters(self):
      return self.masters

   def setInclude(self, name):
      self.include = name
      return

   def getInclude(self):
      return self.include

   def setAllowNotify(self, quad_list):
      self.allow_notify = quad_list
      return

   def getAllowNotify(self):
      return self.allow_notify

   def addAllowNotify(self, nlist):
      self.allow_notify.append(nlist)
      return

   def setAlsoNotify(self, quad_list):
      self.also_notify = quad_list
      return

   def getAlsoNotify(self):
      return self.also_notify

   def addAlsoNotify(self, nlist):
      self.also_notify.append(nlist)
      return

   def setProvideXfr(self, quad_list):
      self.provide_xfr = quad_list
      return

   def getProvideXfr(self):
      return self.provide_xfr

class NsdOptions:
   def __init__(self, config):
      self.database            = config.getValue('database')
      self.difffile            = config.getValue('difffile')
      self.identity            = config.getValue('identity')
      self.ip_address          = config.getValue('ip-address')
      self.logfile             = config.getValue('logfile')
      self.pidfile             = config.getValue('pidfile')
      self.port                = config.getValue('port')
      self.statistics          = config.getValue('statistics')
      self.username            = config.getValue('username')
      self.xfrd_reload_timeout = config.getValue('xfrd-reload-timeout')
      return

   def dump(self, ofile):
      if ofile == sys.stdout:
         print >> ofile, '=> Options:'
         print >> ofile, '   database:            %s' % (self.database)
         print >> ofile, '   difffile:            %s' % (self.difffile)
         print >> ofile, '   identity:            %s' % (self.identity)
         print >> ofile, '   ip-address:          %s' % (self.ip_address)
         print >> ofile, '   logfile:             %s' % (self.logfile)
         print >> ofile, '   pidfile:             %s' % (self.pidfile)
         print >> ofile, '   port:                %s' % (self.port)
         print >> ofile, '   statistics:          %s' % (self.statistics)
         print >> ofile, '   username:            %s' % (self.username)
         print >> ofile, '   xfrd-reload-timeout: %s' % (self.xfrd_reload_timeout)
      else:
         print >> ofile, '#'
         print >> ofile, '# All of the values below have been pulled from'
         print >> ofile, '# either defaults or the config file'
         print >> ofile, '#'
         print >> ofile, ''
         print >> ofile, 'server:'
         print >> ofile, '   database: %s' % (self.database)
         print >> ofile, '   difffile: %s' % (self.difffile)
         print >> ofile, '   identity: %s' % (self.identity)
	 iplist = self.ip_address.split()
	 for ii in iplist:
            print >> ofile, '   ip-address: %s' % (ii.strip())
         print >> ofile, '   logfile: %s' % (self.logfile)
         print >> ofile, '   pidfile: %s' % (self.pidfile)
         print >> ofile, '   port: %s' % (self.port)
         print >> ofile, '   statistics: %s' % (self.statistics)
         print >> ofile, '   username: %s' % (self.username)
         print >> ofile, '   xfrd-reload-timeout: %s' % \
	                 (self.xfrd_reload_timeout)
         print >> ofile, ''
      return

   def setDatabase(self, name):
      self.database = name
      return

   def getDatabase(self):
      return self.database

   def setDifffile(self, name):
      self.difffile = name
      return

   def getDifffile(self):
      return self.difffile

   def setIdentity(self, name):
      self.identity = name
      return

   def getIdentity(self):
      return self.identity

   def setIpAddress(self, quad):
      self.ip_address = quad
      return

   def getIpAddress(self):
      return self.ip_address

   def setLogfile(self, val):
      self.logfile = val
      return

   def getLogfile(self):
      return self.logfile

   def setPort(self, val):
      self.port = val
      return

   def getPort(self):
      return self.port

   def setPidfile(self, val):
      self.pidfile = val
      return

   def getPidfile(self):
      return self.pidfile

   def setStatistics(self, val):
      self.statistics = val
      return

   def getStatistics(self):
      return self.statistics

   def setXfrdReloadTimeout(self, val):
      self.xfrd_reload_timeout = val
      return

   def getXfrdReloadTimeout(self):
      return self.xfrd_reload_timeout

class NsdConf:

   def __init__(self, config):
      self.config = config
      self.fname = config.getValue('nsd_conf')
      self.files = config.getValue('nsd_files')
      self.preamble = config.getValue('nsd_preamble')
      self.acl_list = config.getValue('acl_list')
      self.oldrootdir = ''
      self.options = NsdOptions(config)
      self.includes = []
      self.zones = {}
      self.keys = {}
      self.ipkeymap = {}
      return

   def populate(self, named):
      self.oldrootdir = named.getOptions().getDirectory().replace('"','')
      if self.oldrootdir[len(self.oldrootdir)-1] != '/':
         self.oldrootdir += '/'

      klist = named.getKeys()
      for ii in klist:
         oldk = named.getKey(ii)
         k = NsdKey(oldk.getName())
         k.setAlgorithm(oldk.getAlgorithm())
         k.setSecret(oldk.getSecret())
	 self.keys[k.getName()] = k

	 #-- map each of the key ip addresses for faster lookup on dump()
	 iplist = oldk.getIpAddrs()
	 for jj in iplist:
	    if jj not in self.ipkeymap:
	       self.ipkeymap[jj] = k.getName()

      zlist = named.getZones()
      for ii in zlist:
         oldz = named.getZone(ii)
         z = NsdZone(oldz.getName(), self.config, self.ipkeymap)
	 z.setZonefile(oldz.getFile())
	 z.setInclude(self.acl_list)
	 z.setOldRootDir(self.oldrootdir)
	 if 'slave' in oldz.getType().split():
	    z.setType('slave')		# not used, but nice to know
	    nlist = oldz.getMasters()
	    for ii in nlist:
	       z.addAllowNotify(ii)
	    if len(oldz.getAllowNotify()) > 0:
	       nlist = oldz.getAllowNotify()
	    else:
	       nlist = named.getOptions().getAllowNotify()
	    for ii in nlist:
	       z.addAllowNotify(ii)
	 else:
	    z.setType('master')
	    if len(oldz.getAlsoNotify()):
	       nlist = oldz.getAlsoNotify()
	    else:
	       nlist = named.getOptions().getAlsoNotify()
	    for ii in nlist:
	       z.addAlsoNotify(ii)
	 if len(named.getOptions().getAllowTransfer()) > 0:
	    z.setProvideXfr(named.getOptions().getAllowTransfer())
	 self.zones[z.getName()] = z

      return

   def dump(self):
      report_info('=> NsdConf: dumping data from \"%s\"...' % (self.fname))
      self.options.dump(sys.stdout)
      report_info('=> NsdConf: list of includes...')
      report_info( str(self.includes))

      report_info('=> NsdConf: zone info...')
      zlist = self.zones.keys()
      zlist.sort()
      for ii in zlist:
         self.zones[ii].dump(sys.stdout)
   
      report_info('=> NsdConf: key info...')
      klist = self.keys.keys()
      klist.sort()
      for ii in klist():
         self.keys[ii].dump(sys.stdout)

      return

   def write_conf(self):
      nfile = open(self.fname, 'w+')
      self.options.dump(nfile)

      klist = self.keys.keys()
      klist.sort()
      for ii in klist:
         self.keys[ii].dump(nfile)

      zlist = self.zones.keys()
      zlist.sort()
      for ii in zlist:
         report_info('   writing info for zone "%s"...' % (self.zones[ii].getName()))
         self.zones[ii].dump(nfile)

      nfile.close()
      return
   
   def do_generate(self, newfd, start, stop, step, lhs, rrtype, rhs):
      #-- write the equivalent of the $GENERATE
      for ii in range(start, stop, step):
         left = lhs.replace('$', str(ii))
         right = rhs.replace('$', str(ii))
         print >> newfd, '%s %s %s' % (left, rrtype, right)
      return

   def make_zone_copy(self, oldpath, newpath):
      #-- copy the zone file line-by-line so we can catch $GENERATE usage
      report_info('=> copying "%s" to "%s"' % (oldpath, newpath))
      
      if not os.path.exists(oldpath):
         os.system('touch ' + newpath)
	 return

      oldfd = open(oldpath, 'r+')
      newfd = open(newpath, 'w+')
      line = oldfd.readline()
      while line:
         fields = line.split()
         if len(fields) > 0 and fields[0].strip() == '$GENERATE':
            report_info('=> processing "%s"...' % (line.strip()))
            index = 1

            #-- determine start/stop range
            indices = fields[index].split('-')
            start = 0
            stop  = 0
            if len(indices) == 2:
               start = int(indices[0])
               stop  = int(indices[1])
	       if stop < start:
                  bail('? stop < start in range "%s"' % (fields[1].strip()))
            else:
               bail('? invalid range "%s"' % (fields[1].strip()))
            index += 1
            
            #-- determine increment
            step = 1
            if len(fields) == 6:
               step = int(fields[index])
	       index += 1
      
            #-- get lhs
            lhs = fields[index].strip()
            index += 1
      
            #-- get RR type
            rrtype = fields[index].strip()
            if rrtype not in [ 'NS', 'PTR', 'A', 'AAAA', 'DNAME', 'CNAME' ]:
               bail('? illegal RR type "%s"' % (rrtype))
            index += 1
      
            #-- get rhs
            rhs = fields[index].strip()
            index += 1
      
	    #-- do the $GENERATE
            self.do_generate(newfd, start, stop, step, lhs, rrtype, rhs)
      
         else:		#-- just copy the line as is
	    print >> newfd, line,

         line = oldfd.readline()

      oldfd.close()
      newfd.close()
      
      return

   def write_zone_files(self):
      acls = {}

      zlist = self.zones.keys()
      zlist.sort()
      for ii in zlist:
         oldpath = self.oldrootdir + self.zones[ii].getZonefile()
	 newpath = self.config.getValue('tmpdir') + self.zones[ii].getZonefile()
	 if not os.path.exists(os.path.dirname(newpath)):
	    os.makedirs(os.path.dirname(newpath))
	 self.make_zone_copy(oldpath, newpath)

	 incl = self.zones[ii].getInclude()
         if acls.has_key(incl):
	    acls[incl] += 1
	 else:
	    acls[incl] = 1

      alist = acls.keys()
      alist.sort()
      for ii in alist:
	 newpath = self.config.getValue('tmpdir') + ii
         run_cmd('touch ' + newpath, 'touch "%s"' % (newpath))

      return

   def addZone(self, zone):
      name = zone.getName()
      self.zones[name] = zone
      return

   def getZones(self):
      return self.zones

   def addKey(self, key):
      self.keys[key.getName()] = key
      return

   def getKeys(self):
      return self.keys

   def getKey(self, name):
      if name in self.keys:
         return self.keys[name]
      else:
         return None

