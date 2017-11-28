/* This testcase is part of GDB, the GNU debugger.

   Copyright 2007-2017 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

class Obj {
public:
  Obj ();
  Obj (const Obj &);
  ~Obj ();
  int var[2];
};

int foo (Obj arg)
{
  return arg.var[0] + arg.var[1];
}

Obj::Obj ()
{
  var[0] = 1;
  var[1] = 2;
}

Obj::Obj (const Obj &obj)
{
  var[0] = obj.var[0];
  var[1] = obj.var[1];
}

Obj::~Obj ()
{

}

struct Derived : public Obj
{
  int other;
};

int blap (Derived arg)
{
  return foo (arg);
}

struct Container
{
  Obj obj;
};

int blip (Container arg)
{
  return foo (arg.obj);
}

Obj global_obj;
Derived global_derived;
Container global_container;

int
main ()
{
  int bar = foo (global_obj);
  blap (global_derived);
  blip (global_container);
  return bar;
}
