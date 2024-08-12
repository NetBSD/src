--  Copyright 2021-2023 Free Software Foundation, Inc.
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

procedure pr is

   type Report is (Hello, Goodbye);

   type Response (Length : integer ) is record
      Length_t : Integer;
      Bytes : Natural;
      Msg : Report;
      Val : String(1..Length);
   end record;

   pragma pack (Response);

   Var : Response(11) := (11, 23, 13, Hello, "abcdefghijk");

begin

   null; -- STOP

end pr;
