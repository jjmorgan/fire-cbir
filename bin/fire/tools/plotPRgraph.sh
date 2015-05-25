#!/bin/bash

cat $1 | grep PRgraph | tr '[(,)]' '   ' | awk '{for(i=2;i<=NF;i+=2) printf "%f %f\n",$i,$(i+1)}'
