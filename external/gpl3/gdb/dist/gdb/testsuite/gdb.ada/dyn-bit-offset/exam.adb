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

   type Packed_Array is array (Integer range <>) of Small;
   pragma pack (Packed_Array);

   subtype Range_Int is Natural range 0 .. 7;

   type Some_Packed_Record (Discr : Range_Int := 3) is record
      Array_Field : Packed_Array (1 .. Discr);
      Field: Small;
      case Discr is
         when 3 =>
            Another_Field : Small;
         when others =>
            null;
      end case;
   end record;
   pragma Pack (Some_Packed_Record);
   pragma No_Component_Reordering (Some_Packed_Record);

   SPR : Some_Packed_Record := (Discr => 3,
                                Field => -5,
                                Another_Field => -6,
                                Array_Field => (-5, -6, -7));

begin
   null;                        --  STOP
end Exam;
