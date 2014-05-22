package main

import ("fmt"
        "unsafe")

var mystring = "Shall we?"

func main () {
  fmt.Printf ("%d\n", unsafe.Sizeof (42)) // set breakpoint 1 here
  fmt.Printf ("%d\n", unsafe.Sizeof (mystring))
}
