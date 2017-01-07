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
#	class to represent all of the bind2nsd/syncem config items
#

import os
import os.path
import sys

if os.path.exists('../pyDes-1.2'):
   sys.path.append('../pyDes-1.2')
import pyDes

def mkcipher(val):
   data = val.split()
   ctxt = ''
   for ii in range(0, len(data)):
      ctxt += chr(int(data[ii]))
   return ctxt

def mkprintable(val):
   cstr = ''
   for ii in range(0, len(val)):
      c = val[ii]
      cstr += '%d ' % (ord(c))
   return cstr


class Config:

   def __init__(self):
      #-- set all of the defaults
      self.fname = ''
      self.config = \
         { 'acl_list'            : 'acl_list',
	   'bind2nsd'            : '/usr/bin/bind2nsd',
	   'database'            : '"nsd.db"',
	   'DEBUG'               : False,
	   'DEMO-MODE'           : False,
	   'destdir'             : '/tmp/foobar',
	   'dest-ip'             : '127.0.0.1',
	   'destuser'            : 'dns',
           'difffile'            : '"ixfr.db"',
	   'dnspw'               : 'iforgot',
           'identity'            : '"unknown"',
	   'ip-address'          : '127.0.0.1',
           'logfile'             : '"log"',
	   'named-checkconf'     : '/usr/sbin/named-checkconf',
	   'named-checkzone'     : '/usr/sbin/named-checkzone',
	   'named_root'          : '/etc/bind9',
           'named_conf'          : 'named.conf',
	   'named_watchlist'     : '/etc/named.conf',
	   'nsd-checkconf'       : '/usr/sbin/nsd-checkconf',
	   'nsd_conf'            : 'nsd.conf',
           'nsd_conf_dir'        : '/etc/nsd/',
	   'nsd_preamble'        : 'nsd.conf-preamble',
	   'password_file'       : '/etc/bind2nsd/passwd',
           'pidfile'             : '"nsd.pid"',
	   'port'                : '53',
	   'rebuild_cmd'         : '/etc/init.d/nsdc rebuild',
	   'restart_cmd'         : '/etc/init.d/nsdc restart',
	   'sleep_time'          : '5',
	   'start_cmd'           : '/etc/init.d/nsdc start',
	   'statistics'          : '3600',
	   'stop_cmd'            : '/etc/init.d/nsdc stop',
	   'syspw'               : 'iforgot',
	   'tmpdir'              : '/tmp/secure64/',   # must have trailing '/'
	   'username'            : 'nsd',
           'version'             : '0.5.0',
	   'xfrd-reload-timeout' : '10',
	   'zonec_cmd'           : '/etc/init.d/zonec',
         }
 
      self.init()
      self.read_passwords()

      if self.config['DEBUG']:
         self.dump()
      return

   def init(self):
      fname = ''
      if os.path.exists('bind2nsd.conf'):
         self.fname = 'bind2nsd.conf'
      else:
         fname = os.getenv('HOME', '.') + '/bind2nsd.conf'
	 if os.path.exists(fname):
	    self.fname = fname
	 else:
	    if os.path.exists('/etc/bind2nsd/bind2nsd.conf'):
	       self.fname = '/etc/bind2nsd/bind2nsd.conf'
	    else:
	       print '? hrm.  no config file found -- did you _mean_ that?'

      #-- override the defaults
      if self.fname != '':
         fd = open(self.fname, 'r')
	 line = fd.readline()
	 while line:
	    if len(line) > 0:
	       info = line.split()
	       if line[0] == '#':
	          pass		# ignore comments
	       elif len(info) > 0:
	          item = info[0].strip()
	          if info[1].strip() == '=':
	             if item in self.config:
		        self.config[item] = ' '.join(info[2:])
	       else:
	          pass 		# ignore lines with only one field
	    else:
	       pass		# ignore empty lines
	    line = fd.readline()

      return

   def read_passwords(self):
      fname = self.config['password_file']
      if os.path.exists(fname):
         fd = open(fname, 'r+')
	 syspw = fd.readline()
	 dnspw = fd.readline()
	 fd.close()

	 obj = pyDes.triple_des('aBcDeFgHiJkLmNoP', pyDes.ECB)
	 self.config['syspw'] = obj.decrypt(mkcipher(syspw), '#')
	 self.config['dnspw'] = obj.decrypt(mkcipher(dnspw), '#')

      return

   def getValue(self, item):
      if item in self.config:
         return self.config[item]
      else:
         return None

   def setValue(self, item, val):
      if item in self.config:
         self.config[item] = val
      else:
         print '? no such config item "%s" (%s)' % (item, val)
      return

   def dump(self):
      print '=> Config:'
      print '   %-20s = %s' % ('fname', self.fname)
      for ii in self.config:
         print '   %-20s = %s' % (ii, self.config[ii])
      return

