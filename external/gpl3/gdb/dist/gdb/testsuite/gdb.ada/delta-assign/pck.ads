--  Copyright 2024 Free Software Foundation, Inc.
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

   type Record_Type is record
      A : Integer;
      B : Integer;
   end record;

   V1 : Record_Type := (A => 23, B => 24);
   V2 : Record_Type := (A => 47, B => 91);

   type Other_Record_Type is record
      A : Integer;
      B : Integer;
      C : Integer;
   end record;

   V3 : Other_Record_Type := (A => 47, B => 91, C => 102);

   type Array_Type is array (1 .. 3) of Integer;

   A1 : Array_Type := (2, 4, 6);

   procedure Do_Nothing (A : System.Address);

end Pck;
