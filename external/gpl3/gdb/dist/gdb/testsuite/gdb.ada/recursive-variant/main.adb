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

procedure Main is

   type Rec_Type;

   type Rec_Type_Ref is access all Rec_Type;

   type Rec_Type (X : Boolean) is record
      Link : Rec_Type_Ref;
      case X is
         when True =>
            Value : Integer;
         when False =>
            null;
      end case;
   end record;

   Instance : Rec_Type_Ref := new Rec_Type'(X => False, Link => null);

begin

   null; -- STOP

end Main;
