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
with Support; use Support;

package Pck is
   subtype Length is Natural range 0 .. 10;
   type Bounded_String (Max_Length : Length := Length'Last) is
   record
      Current_Length : Length := 0;
      Buffer : String (1 .. Max_Length);
   end record;

   type Blob is array (Integer range <>) of Bounded_String;

   Static_Blob : Blob (1 .. 10);
   Static_Blob_Size : Integer := Static_Blob'Size;

   Dynamic_Blob : Blob (1 .. Ident (10));
   Dynamic_Blob_Size : Integer := Dynamic_Blob'Size;

   procedure Do_Nothing (A : System.Address);
end Pck;
