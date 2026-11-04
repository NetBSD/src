// Copyright (C) 2024 Free Software Foundation, Inc.

// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#![allow(dead_code)]
#![allow(unused_variables)]
#![allow(unused_assignments)]

fn ignore<T>(x: T) { }

// A generic struct that is unsized if T is unsized.
pub struct MaybeUnsizedStruct<T: ?Sized> {
    pub regular: u32,
    pub rest: T,
}

// Same but without an ordinary part.
pub struct MaybeUnsizedStruct2<T: ?Sized> {
    value: T,
}

fn main() {
    // This struct is still sized because T is a fixed-length array
    let sized_struct = &MaybeUnsizedStruct {
        regular: 23,
        rest: [5, 6, 7],
    };

    // This struct is still sized because T is sized
    let nested_sized_struct = &MaybeUnsizedStruct {
        regular: 91,
        rest: MaybeUnsizedStruct {
            regular: 23,
            rest: [5, 6, 7],
        },
    };

    // This will be a fat pointer, containing the length of the final
    // field.
    let unsized_struct: &MaybeUnsizedStruct<[u32]> = sized_struct;

    // This will also be a fat pointer, containing the length of the
    // final field.
    let nested_unsized_struct:
        &MaybeUnsizedStruct<MaybeUnsizedStruct<[u32]>> = nested_sized_struct;

    let alpha: MaybeUnsizedStruct2<[u8; 4]> = MaybeUnsizedStruct2 { value: *b"abc\0" };
    let beta: &MaybeUnsizedStruct2<[u8]> = &alpha;

    let reference = &unsized_struct;

    ignore(sized_struct); // set breakpoint here
    ignore(nested_sized_struct);
    ignore(unsized_struct);
    ignore(nested_unsized_struct);
}
