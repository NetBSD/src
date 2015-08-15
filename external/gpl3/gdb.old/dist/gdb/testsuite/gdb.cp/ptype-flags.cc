/* Copyright 2012-2014 Free Software Foundation, Inc.

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

template<typename S>
class Simple
{
  S val;
};

template<typename T>
class Base
{
};

template<typename T>
class Holder : public Base<T>
{
public:
  Simple<T> t;
  Simple<T*> tstar;

  typedef Simple< Simple<T> > Z;

  Z z;

  double method(void) { return 23.0; }
};

Holder<int> value;

int main()
{
  return 0;
}
