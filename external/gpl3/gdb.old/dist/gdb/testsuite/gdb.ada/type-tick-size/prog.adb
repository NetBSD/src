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

with Pck; use Pck;

procedure Prog is
   type Color is (Red, Orange, Yellow, Green, Blue, Indigo, Violet);

   type Simple is record
      I : Integer;
   end record;

   subtype Small_Int is Natural range 0 .. 100;
   type Rec (L : Small_Int := 0) is record
      S : String (1 .. L);
   end record;

   Boolean_Val : Boolean := True;
   Boolean_Size : Integer := Boolean_Val'Size;
   Boolean_Type_Size : Integer := Boolean'Object_Size;

   Color_Val : Color := Indigo;
   Color_Size : Integer := Color_Val'Size;
   Color_Type_Size : Integer := Color'Object_Size;

   Simple_Val : Simple := (I => 91);
   Simple_Size : Integer := Simple_Val'Size;
   Simple_Type_Size : Integer := Simple'Object_Size;

   Small_Int_Val : Small_Int := 31;
   Small_Int_Size : Integer := Small_Int_Val'Size;
   Small_Int_Type_Size : Integer := Small_Int'Object_Size;

   Rec_Val : Rec := (L => 2, S => "xy");
   -- This is implementation defined, so the compiler does provide
   -- something, but gdb does not.
   Rec_Size : Integer := Rec_Val'Size;
   Rec_Type_Size : Integer := Rec'Object_Size;

begin
   Do_Nothing (Static_Blob'Address);
   Do_Nothing (Dynamic_Blob'Address);
   null;                        -- STOP
end Prog;
