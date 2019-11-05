MOPTOP source 

This is a copy of the original source code written for moptop testing. This was superceded by
the mopnet code due to reliability issues with multiple cameras Andor controlled by one control computer.

build	- build script, fully optimised
bldslow - build script, no-optimisation
blddual - build script, dual executables moptop1 & 2

moptop.h  - project header
mop_dat.h - global data

moptop.c  - main programme, continuous rotation
mop_cam.c - Andor camera routines
mop_fts.c - FITS file handling
mop_log.c - Logging
mop_opt.c - Run-time options parser
mop_rot.c - PI rotator handler
mopstop.c - main programme, stepped rotation
mop_utl.c - utilities and inter-process comms




