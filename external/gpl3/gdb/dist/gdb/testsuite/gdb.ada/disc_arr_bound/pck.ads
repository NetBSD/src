--  Copyright 2015-2017 Free Software Foundation, Inc.
--
--  This program is free software; you can redistribute it and/or modify
--  it under the terms of the GNU General Public License as published by
--  the Free Software Foundation; either version 3 of the License, or
--  (at your option) any later version.
--
--  This program is distributed in the hope that it will be useful,
--  but WITHOUT ANY WARRANTY; without even the implied warranty of
--  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
--  GNU General Public License for more details.
--
--  You should have received a copy of the GNU General Public License
--  along with this program.  If not, see <http://www.gnu.org/licenses/>.

with System;

package Pck is

   type Array_Type is array (Integer range <>) of Integer;
   type Record_Type (N : Integer) is record
      A : Array_Type (1 .. N);
   end record;

   function Get (N : Integer) return Record_Type;

   procedure Do_Nothing (A : System.Address);

end Pck;
