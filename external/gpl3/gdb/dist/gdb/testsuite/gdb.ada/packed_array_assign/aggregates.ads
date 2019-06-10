--  Copyright 2018-2019 Free Software Foundation, Inc.
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

package Aggregates is
   subtype Int is Integer range 0 .. 7;

   type Packed_Rec is record
      Packed_Array_Assign_W    : Integer;
      Packed_Array_Assign_X, Packed_Array_Assign_Y : Int;
   end record;
   pragma Pack (Packed_Rec);

   type Packed_RecArr is array (Integer range <>) of Packed_Rec;
   pragma Pack (Packed_RecArr);

   procedure Run_Test;

private
   PR : Packed_Rec := (Packed_Array_Assign_Y => 3,
                       Packed_Array_Assign_W => 104,
                       Packed_Array_Assign_X  => 2);
   PRA : Packed_RecArr (1 .. 3);
end Aggregates;
