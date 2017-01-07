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
#	class containing the parser for BIND configuration files
#
#

import os
import os.path
import sys

if os.path.exists('../bind2nsd/Config.py'):
   sys.path.append('../bind2nsd')
   from NamedConf import *
   from Zone import *
   from Key import *
   from Tokenizer import *
   from Utils import *
else:
   from bind2nsd.NamedConf import *
   from bind2nsd.Zone import *
   from bind2nsd.Key import *
   from bind2nsd.Tokenizer import *
   from bind2nsd.Utils import *


#-- the following options {} clauses are currently currently ignored,
#   typically because they are not implemented or not applicable to NSD
IGNORED_OPTIONS = [ 'cleaning-interval',
                    'datasize',
                    'interface-interval',
		    'max-cache-size',
                    'stacksize',
                  ]

class Parser:

   def __init__(self, named):
      self.named = named
      return

   def handle_include(self, tokens):
      (ttype, val, curline) = tokens.get()
      if ttype == T_STRING:
         fname = val.replace('"', '')
         self.named.includes.append(fname)
         report_info('   include file "%s"...' % (fname))
         if os.path.exists(fname):
            self.parse(Tokenizer(fname))
         else:
            report_error('? missing include file "%s"' % (fname))
      else:
         bail('? dude.  where is the filename string after "include"?')
      (ttype, val, curline) = tokens.get()
      if ttype != T_SEMI:
         bail('? need a semicolon to end the "include" (%d: %s)' % (ttype, val))
      return
   
   def handle_key(self, tokens):
      (ttype, val, curline) = tokens.get()
      if ttype == T_STRING:
         name = val.replace('"', '')
         key = Key(name)
         (ttype, val, curline) = tokens.get()
         if ttype != T_LBRACE:
            bail('? need opening brace for "key" defn (%d: %s)' % (ttype, val))
         (ttype, val, curline) = tokens.get()
         while ttype != T_RBRACE:
            if ttype == T_KEYWORD and val == 'algorithm':
               (ttype, val, curline) = tokens.get()
 	       if ttype == T_KEYWORD or ttype == T_NAME:
	          key.setAlgorithm(val)
	       else:
	          bail('? bogosity after "algorithm" (%d: %s)' % (ttype, val))
	    elif ttype == T_KEYWORD and val == 'secret':
	       (ttype, val, curline) = tokens.get()
	       if ttype == T_STRING:
	          s = val.replace('"','')
		  key.setSecret(s)
	       else:
	          bail('? bogosity after "secret" (%d: %s)' % (ttype, val))
	    elif ttype == T_SEMI:
	       pass
	    (ttype, val, curline) = tokens.get()
      else:
         bail('? need to have a string after "key" (%d: %s)' % (ttype, val))

      self.named.addKey(key)
      (ttype, val, curline) = tokens.get()
      if ttype != T_SEMI:
         bail('? need a semicolon to end the "key" (%d: %s)' % (ttype, val))
      return
   
   def handle_server(self, tokens):
      (ttype, val, curline) = tokens.get()
      if ttype == T_QUAD:
         ipaddr = val
         (ttype, val, curline) = tokens.get()
         if ttype != T_LBRACE:
            bail('? need opening brace for "server" (%d: %s)' % (ttype, val))
         (ttype, val, curline) = tokens.get()
         while ttype != T_RBRACE:
            if ttype == T_KEYWORD and val == 'keys':
               (ttype, val, curline) = tokens.get()
 	       if ttype == T_LBRACE:
	          nlist = self.name_list(val, tokens)
		  for ii in nlist:
		     self.named.keys[ii].addIpAddr(ipaddr)
	       else:
	          bail('? bogosity after "keys" (%d: %s)' % (ttype, val))
	    (ttype, val, curline) = tokens.get()
      else:
         bail('? need to have an IP address after "server" (%d: %s)' % \
	      (ttype, val))

      (ttype, val, curline) = tokens.get()
      if ttype != T_SEMI:
         bail('? need a semicolon to end the "server" (%d: %s)' % (ttype, val))
      return
   
   def name_list(self, name, tokens):
      nlist = []
      (ttype, val, curline) = tokens.get()
      if ttype == T_LBRACE or ttype == T_LPAREN:
         (ttype, val, curline) = tokens.get()
         while ttype != T_RBRACE and ttype != T_RPAREN:
            if ttype == T_NAME:
	       nlist.append(val)
            elif ttype == T_SEMI or ttype == T_COMMENT:
               pass
            else:
               bail('? was expecting a name in "%s" (%d, %d: %s)' % \
	            (name, ttype, curline, val.strip()))
            (ttype, val, curline) = tokens.get()
         (ttype, val, curline) = tokens.get()
         if ttype != T_SEMI:
            bail('? junk after closing brace of "%s" section' % (name))

      return nlist

   def quad_list(self, name, tokens):
      qlist = []
      (ttype, val, curline) = tokens.get()
      if ttype == T_LBRACE or ttype == T_LPAREN:
         (ttype, val, curline) = tokens.get()
         while ttype != T_RBRACE and ttype != T_RPAREN:
            if ttype == T_QUAD:
	       qlist.append(val)
            elif ttype == T_SEMI or ttype == T_COMMENT:
               pass
            else:
               bail('? was expecting a quad in "%s" (%d, %d: %s)' % \
	            (name, ttype, curline, val.strip()))
            (ttype, val, curline) = tokens.get()
         (ttype, val, curline) = tokens.get()
         if ttype != T_SEMI:
            bail('? junk after closing brace of "%s" section' % (name))

      return qlist

   def quad_list_with_keys(self, name, tokens, named):
      qlist = []
      (ttype, val, curline) = tokens.get()
      if ttype == T_LBRACE or ttype == T_LPAREN:
         (ttype, val, curline) = tokens.get()
         while ttype != T_RBRACE and ttype != T_RPAREN:
            if ttype == T_QUAD:
	       qlist.append(val)
            elif ttype == T_SEMI or ttype == T_COMMENT:
               pass
	    elif ttype == T_KEYWORD and val == 'key':
               (ttype, val, curline) = tokens.get()
	       if ttype == T_NAME:
	          if val in named.getKeys():
		     qlist.append(val)
	       else:
                  bail('? was expecting a key name in "%s" (%d, %d: %s)' % \
	               (name, ttype, curline, val.strip()))
            else:
               bail('? was expecting a quad in "%s" (%d, %d: %s)' % \
	            (name, ttype, curline, val.strip()))
            (ttype, val, curline) = tokens.get()
         (ttype, val, curline) = tokens.get()
         if ttype != T_SEMI:
            bail('? junk after closing brace of "%s" section' % (name))

      return qlist

   def skip_value(self, name, tokens):
      (ttype, val, curline) = tokens.get()
      if ttype == T_NAME or ttype == T_STRING:
         pass
      else:
         bail('? need something after "%s" keyword (%d: %s)' % 
	      (name, ttype, val))
      return

   def handle_options(self, tokens):
      report_info('=> processing server options')
      optsDone = False
      depth = 0
      (ttype, val, curline) = tokens.get()
      if ttype == T_LBRACE:
         depth += 1
      else:
         bail('? urk.  bad info after "options" keyword.')
      (ttype, val, curline) = tokens.get()

      while not optsDone:
         if ttype == T_KEYWORD and val == 'directory':
            (ttype, val, curline) = tokens.get()
            if ttype == T_STRING:
               self.named.options.setDirectory(val)
	    else:
               bail('? need string after "directory" keyword')

	 elif ttype == T_KEYWORD and val == 'dump-file':
	    (ttype, val, curline) = tokens.get()
	    if ttype == T_STRING:
	       self.named.options.setDumpFile(val)
	    else:
	       bail('? need string after "dump-file" keyword')

	 elif ttype == T_KEYWORD and val == 'pid-file':
	    (ttype, val, curline) = tokens.get()
	    if ttype == T_STRING:
	       self.named.options.setPidFile(val)
	    else:
	       bail('? need string after "pid-file" keyword')

	 elif ttype == T_KEYWORD and val == 'coresize':
	    (ttype, val, curline) = tokens.get()
	    if ttype == T_NAME or ttype == T_KEYWORD:
	       self.named.options.setCoresize(val)
	    else:
	       bail('? need name after "coresize" keyword (%d: %s)' % \
	            (ttype, val))

	 elif ttype == T_KEYWORD and val == 'version':
	    (ttype, val, curline) = tokens.get()
	    if ttype == T_STRING:
	       self.named.options.setVersion(val)
	    else:
	       bail('? need string after "version" keyword')

	 elif ttype == T_KEYWORD and val == 'transfer-format':
	    (ttype, val, curline) = tokens.get()
	    if ttype == T_NAME or ttype == T_KEYWORD:
	       self.named.options.setTransferFormat(val)
	    else:
	       bail('? need name after "transfer-format" keyword (%d: %s)' % \
	            (ttype, val))

         elif ttype == T_KEYWORD and val == 'statistics-file':
            (ttype, val, curline) = tokens.get()
            if ttype == T_STRING:
               self.named.options.setStatisticsFile(val)
            else:
  	       bail('? need string after "statistics-file" keyword')

	 elif ttype == T_KEYWORD and val == 'allow-transfer':
	    qlist = self.quad_list_with_keys(val, tokens, self.named)
	    self.named.options.setAllowTransfer(qlist)

	 elif ttype == T_KEYWORD and val == 'allow-notify':
	    qlist = self.quad_list(val, tokens)
	    self.named.options.setAllowNotify(qlist)

	 elif ttype == T_KEYWORD and val == 'also-notify':
	    qlist = self.quad_list(val, tokens)
	    self.named.options.setAlsoNotify(qlist)

	 elif ttype == T_KEYWORD and val == 'allow-recursion':
	    qlist = self.quad_list(val, tokens)
	    self.named.options.setAllowRecursion(qlist)

         elif ttype == T_KEYWORD and val == 'check-names':
            (ttype, val, curline) = tokens.get()
            info = []
            while ttype != T_SEMI:
  	       info.append(val)
               (ttype, val, curline) = tokens.get()
            self.named.options.addCheckNames(' '.join(info))
   
         elif ttype == T_KEYWORD and val in IGNORED_OPTIONS:
	    self.skip_value(val, tokens)

	 elif ttype == T_KEYWORD and val == 'recursive-clients':
	    (ttype, val, curline) = tokens.get()
	    if ttype == T_QUAD:		# well, it's a number, actually...
	       self.named.options.setRecursiveClients(val)
	    else:
	       bail('? need value after "recursive-clients" keyword (%d: %s)'
	            % (ttype, val))

         elif ttype == T_RBRACE:
            optDone = True
            break
         (ttype, val, curline) = tokens.get()

      (ttype, val, curline) = tokens.get()
      if ttype != T_SEMI:
         bail('? um, junk after closing brace of "options" section')
   
      return

   def handle_zone(self, tokens):
      (ttype, val, curline) = tokens.get()
      if ttype != T_STRING:
         bail('? expected string for zone name (%d: %s)' % (ttype, val))
      zname = val.replace('"','')
      zone = Zone(zname)
   
      (ttype, val, curline) = tokens.get()
      if ttype == T_KEYWORD and (val == 'in' or val == 'IN'):
         (ttype, val, curline) = tokens.get()
      if ttype == T_LBRACE or ttype == T_LPAREN:
         (ttype, val, curline) = tokens.get()
         while ttype != T_RBRACE and ttype != T_RPAREN:
            if ttype == T_KEYWORD and val == 'type':
               (ttype, val, curline) = tokens.get()
  	       info = []
  	       while ttype == T_KEYWORD or ttype == T_NAME:
  	          info.append(val)
                  (ttype, val, curline) = tokens.get()
  	       zone.setType(' '.join(info))

            elif ttype == T_KEYWORD and val == 'file':
               (ttype, val, curline) = tokens.get()
	       if ttype != T_STRING:
	          bail('? eh?  no string for zone file name? (%d: %s)' % \
		       (ttype, val))
	       fname = val.replace('"','')
	       zone.setFile(fname)

	    elif ttype == T_KEYWORD and val == 'masters':
	       qlist = self.quad_list(val, tokens)
	       zone.setMasters(qlist)

	    elif ttype == T_KEYWORD and val == 'allow-transfer':
	       qlist = self.quad_list_with_keys(val, tokens, self.named)
	       zone.setAllowTransfers(qlist)

	    elif ttype == T_KEYWORD and val == 'also-notify':
	       qlist = self.quad_list(val, tokens)
	       zone.setAlsoNotify(qlist)

	    elif ttype == T_SEMI:
	       pass

	    else:
	       bail('? bugger.  do NOT grok "%s" yet (%d: %s @ line %s)' % \
	            (val, ttype, val, curline))

	    (ttype, val, curline) = tokens.get()

	 self.named.addZone(zone)

      else:
         bail('? expected left brace to start zone definition (%d: %s)' % \
	      (ttype, val))

      (ttype, val, curline) = tokens.get()
      if ttype != T_SEMI:
         bail('? need a semicolon to end the "zone" (%d: %s)' % (ttype, val))

      return
   
   def ignore_section(self, name, tokens):
      report_info('=> ignoring %s' % (name))
      done = False
      depth = 0
      (ttype, val, curline) = tokens.get()
      if ttype == T_LBRACE:
         depth += 1
      else:
         bail('? urk.  bad info after "%s" keyword.' % (name))
      (ttype, val, curline) = tokens.get()
      while not done:
         if ttype == T_LBRACE:
            depth += 1
         elif ttype == T_RBRACE:
            depth -= 1
            if depth == 0:
               break
            else:
               pass
         (ttype, val, curline) = tokens.get()
      (ttype, val, curline) = tokens.get()
      if ttype != T_SEMI:
         bail('? um, junk after closing brace of "%s" section' % (name))
      return

   def parse(self, tokens):
      (ttype, val, curline) = tokens.get()
      while ttype != T_EOF:
         if ttype == T_COMMENT:
            pass

         elif ttype == T_KEYWORD and val == 'controls':
	    self.ignore_section(val, tokens)
   
         elif ttype == T_KEYWORD and val == 'key':
	    self.handle_key(tokens)

         elif ttype == T_KEYWORD and val == 'include':
	    self.handle_include(tokens)

         elif ttype == T_KEYWORD and val == 'logging':
	    self.ignore_section(val, tokens)
   
         elif ttype == T_KEYWORD and val == 'lwres':
	    self.ignore_section(val, tokens)
   
         elif ttype == T_KEYWORD and val == 'options':
	    self.handle_options(tokens)

         elif ttype == T_KEYWORD and val == 'server':
	    self.handle_server(tokens)

         elif ttype == T_KEYWORD and val == 'trusted-keys':
	    self.ignore_section(val, tokens)

         elif ttype == T_KEYWORD and val == 'view':
	    self.ignore_section(val, tokens)

         elif ttype == T_KEYWORD and val == 'zone':
	    self.handle_zone(tokens)

         else:
            bail('? ew.  what _is_ this? -> %2d: %s @ line %d' \
	         % (ttype, val, curline))
         (ttype, val, curline) = tokens.get()
   
      return

