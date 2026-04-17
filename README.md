Hudson Input/Output (Version 2)
===============================

A decompilation of Hudson's I/O (version 2) library for the Nintendo Wii.  

I got bored and this only took like an hour.  

Debug build is matching but release build is not matching due to an optimization from `__HIO2IsValidHandle` refusing for the functions `HIO2Init` and `HIO2Exit`. Decomp.me scratch links available in `src/hio2.c` for those willing to figure it out.