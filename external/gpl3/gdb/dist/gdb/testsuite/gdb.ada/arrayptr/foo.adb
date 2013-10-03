--  Copyright 2008-2013 Free Software Foundation, Inc.
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

procedure Foo is
   type String_Access is access String;

   String_P : String_Access := new String'("Hello");
   Null_String : String_Access := null;

   --  Same situation, but constrained array.
   type Little_Array is array (1 .. 10) of Integer;
   type Little_Array_Ptr is access all Little_Array;
   Arr_Ptr: Little_Array_Ptr :=
     new Little_Array'(21, 22, 23, 24, 25, 26, 27, 28, 29, 30);
begin
   Do_Nothing (String_P'Address);  -- STOP
   Do_Nothing (Null_String'Address);
   Do_Nothing (Arr_Ptr'Address);
end Foo;
