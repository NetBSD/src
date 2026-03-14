--  Copyright 2025 Free Software Foundation, Inc.
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

procedure Exam is
   type Small is range -7 .. -4;
   for Small'Size use 2;

   type Range_Int is range 1 .. 8;
   for Range_Int'Size use 3;

   type Packed_Array is array (Range_Int range <>) of Small;
   pragma pack (Packed_Array);

   type Some_Packed_Record (Discr : Range_Int) is tagged record
      Field: Small;
      Array_Field : Packed_Array (1 .. Discr);
   end record;
   pragma Pack (Some_Packed_Record);

   type Sub_Class (Inner, Outer : Range_Int)
   is new Some_Packed_Record (Inner) with
      record
         Another_Array : Packed_Array (1 .. Outer);
      end record;
   pragma Pack (Sub_Class);

   SPR : Some_Packed_Record := (Discr => 3,
                                Field => -4,
                                Array_Field => (-5, -6, -7));

   SC : Sub_Class := (Inner => 3,
                      Outer => 2,
                      Field => -4,
                      Array_Field => (-5, -6, -7),
                      Another_Array => (-7, -6));

begin
   null;                        --  STOP
end Exam;
