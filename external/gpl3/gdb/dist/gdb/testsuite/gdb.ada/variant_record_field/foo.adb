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
with Ada.Text_IO;

procedure Foo is

   package P is

      type Int16 is range 0 .. 2 ** 16 - 1;
      type Enum is (Zero, One, Two, Three, Four, Five, Six, Seven, Eight, Nine);

      type Rec (Kind : Enum := Zero) is record
         case Kind is
            when Four .. Seven =>
               I : Int16;
            when others =>
               null;
         end case;
      end record;

   end P;

   P_Record : P.Rec;
   I : P.Int16;

   procedure Dump is
   begin
      Ada.Text_IO.Put_Line ("P_Record.I => " & P_Record.I'Image); -- BREAK
   end Dump;

begin
   I := P.Int16'(1200);
   P_Record := (Kind => P.Five, I => I);
   Dump;
end Foo;
