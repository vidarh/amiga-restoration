
SnowFall
========

From AmigaLibDisk 303 (Fred Fish) contents file:

SnowFall	Another program in the long tradition of screen hacks for
		the amiga.  Watch the snow fall, get blown around by the
		wind, and collect in realistic heaps.  Includes source.
		Author:  Lars Clausen

**Original code (C) 1989 Lars Clausen**

Updates listed below (C) 2010 Vidar Hokstad 

This code is freely redistributable. No further license information was
provided with the original code. The modifications below are hereby
placed in the public domain.

Modifications
-------------

 - Updated to ANSI C 89
 - Stripped out custom IFF loader; now uses DataTypes.
 - Stripped out use of a second screen as backing store for the loaded image
   (instead uses a bitmap)
 - Added experimental code to scale the bitmap since the original image
   is 320x256.

So far only tested with AROS. Contains a bug that prevents it from exiting
correctly when started in the shell; causes hosted AROS to consume a lot of
CPU after the app has exited. In other words it probably does not correctly
free all resources.

