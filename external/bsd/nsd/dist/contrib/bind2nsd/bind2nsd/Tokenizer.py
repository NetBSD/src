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
#	class to tokenize a named.conf file
#
#

T_KEYWORD = 0
T_STRING  = 1
T_LBRACE  = 2
T_RBRACE  = 3
T_SEMI    = 4
T_COMMA   = 5
T_QUAD    = 6
T_COMMENT = 7
T_NAME    = 8
T_EOF     = 9
T_LPAREN  = 10
T_RPAREN  = 11

NAMED_KEYWORDS = [ 'algorithm', 'allow-notify', 'allow-recursion', 
                   'allow-transfer', 'also-notify',
                   'category', 'channel', 'check-names', 'controls', 
		   'coresize',
                   'directory', 'dump-file',
		   'file',
		   'IN', 'in', 'include',
		   'key', 'keys',
		   'logging',
		   'masters',
                   'options',
		   'pid-file', 'print-time',
		   'recursive-clients',
		   'secret', 'severity', 'statistics-file', 'syslog',
		   'transfer-format', 'type',
		   'version', 'versions',
		   'zone',
		 ]

class Tokenizer:

   def __init__(self, fname):
      infile = file(fname)
      self.data = ''.join(infile.readlines())
      self.curpos = 0
      self.lastpos = len(self.data)
      self.line = 1
      return

   def get(self):
      tok = ''
      done = False
      while not done:
         if self.curpos >= self.lastpos:
	    return (T_EOF, None, self.line)
         c = self.data[self.curpos]
	 if c == '\n':
	    self.line += 1
	 if c.isspace():
	    self.curpos += 1
	    continue
	 elif c == '{':
	    self.curpos += 1
	    return (T_LBRACE, c, self.line)
	 elif c == '}':
	    self.curpos += 1
	    return (T_RBRACE, c, self.line)
	 elif c == '(':
	    self.curpos += 1
	    return (T_LPAREN, c, self.line)
	 elif c == ')':
	    self.curpos += 1
	    return (T_RPAREN, c, self.line)
	 elif c == ';':
	    self.curpos += 1
	    return (T_SEMI, c, self.line)
	 elif c == '#':
	    while c != '\n':
	       tok += c
	       self.curpos += 1
	       c = self.data[self.curpos]
            self.curpos += 1
	    if c == '\n':
	       self.line += 1
	    return (T_COMMENT, tok, self.line)
	 elif c == '"':
	    tok += c
	    self.curpos += 1
	    c = self.data[self.curpos]
	    while c != '"':
	       tok += c
	       self.curpos += 1
	       c = self.data[self.curpos]
	    tok += c
            self.curpos += 1
	    return (T_STRING, tok, self.line)
	 elif c == '/' and self.data[self.curpos+1] == '/':
	    while c != '\n':
	       tok += c
	       self.curpos += 1
	       c = self.data[self.curpos]
	    if c == '\n':
	       self.line += 1
            self.curpos += 1
	    return (T_COMMENT, tok, self.line)
	 elif c == '/' and self.data[self.curpos+1] == '*':
	    cmtDone = False
	    tok += c
	    tok += self.data[self.curpos+1]
	    self.curpos += 2
	    c = self.data[self.curpos]
	    while not cmtDone:
	       if c == '*' and self.data[self.curpos+1] == '/':
		  tok += c
	          tok += self.data[self.curpos+1]
	          self.curpos += 2
	          return (T_COMMENT, tok, self.line)
	       else:
	          tok += c
		  if c == '\n':
		     self.line += 1
		  self.curpos +=1
	          c = self.data[self.curpos]
	    return (T_COMMENT, tok, self.line)
	 elif c.isdigit():
	    tok += c
	    self.curpos += 1
	    c = self.data[self.curpos]
	    while c.isdigit() or c == '.' or c == '/':
	       tok += c
	       self.curpos += 1
	       c = self.data[self.curpos]
	    return (T_QUAD, tok, self.line)
	 else:
	    while c.isalnum() or c == '_' or c == '-' or c == '.':
	       tok += c
	       self.curpos += 1
	       c = self.data[self.curpos]
	    if tok in NAMED_KEYWORDS:
	       return (T_KEYWORD, tok, self.line)
	    else:
	       return (T_NAME, tok, self.line)

      return

   def dump(self):
      print 'Tokenizer raw data:'
      print 'cur, last: %d, %d' % (self.curpos, self.lastpos)
      print self.data
      return

