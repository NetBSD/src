package main

import "fmt"

var myst = "Shall we?"

func main () {
  fmt.Println ("Before assignment") // set breakpoint 1 here
  st := "Hello, world!" // this intentionally shadows the global "st"
  fmt.Println (st) // set breakpoint 2 here
  fmt.Println (myst) // set breakpoint 2 here
}
