############################################################
## This file is generated automatically by Vivado HLS.
## Please DO NOT edit it.
## Copyright (C) 1986-2016 Xilinx, Inc. All Rights Reserved.
############################################################

# open the project, don't forget to reset
open_project -reset proj0
set_top simple_pflow_parallel_hwopt
add_files src/simple_pflow.cpp
add_files -tb simple_pflow_test.cpp 
add_files -tb simple_pflow_ref.cpp
add_files -tb DiscretePFInputs.h -cflags "-std=c++0x"
add_files -tb DiscretePFInputs_IO.h -cflags "-std=c++0x"
add_files -tb data/regions_TTbar_PU140.dump

# reset the solution
open_solution -reset "solution1"
#set_part {xcku9p-ffve900-2-i-EVAL}
#set_part {xc7vx690tffg1927-2}
#set_part {xcku5p-sfvb784-3-e}
set_part {xcku115-flvf1924-2-i}
create_clock -period 5 -name default
#source "./nb1/solution1/directives.tcl"

# do stuff
csim_design
csynth_design
#cosim_design -trace_level all
#export_design -format ip_catalog

# exit Vivado HLS
exit
