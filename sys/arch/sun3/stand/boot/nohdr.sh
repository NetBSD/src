#!/bin/sh

dd ibs=32 skip=1 | dd conv=sync
