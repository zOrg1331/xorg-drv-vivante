##############################################################################
#
#    Copyright (C) 2005 - 2012 by Vivante Corp.
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the license, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program; if not write to the Free Software
#    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#
##############################################################################


#  Copyright 2005 Adam Jackson.
#
#  Permission is hereby granted, free of charge, to any person obtaining a
#  copy of this software and associated documentation files (the "Software"),
#  to deal in the Software without restriction, including without limitation
#  on the rights to use, copy, modify, merge, publish, distribute, sub
#  license, and/or sell copies of the Software, and to permit persons to whom
#  the Software is furnished to do so, subject to the following conditions:
#
#  The above copyright notice and this permission notice (including the next
#  paragraph) shall be included in all copies or substantial portions of the
#  Software.
#
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#  FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
#  ADAM JACKSON BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
#  IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
#  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

# this is obnoxious:
# -module lets us name the module exactly how we want
# -avoid-version prevents gratuitous .0.0.0 version numbers on the end
# _ladir passes a dummy rpath to libtool so the thing will actually link
# TODO: -nostdlib/-Bstatic/-lgcc platform magic, not installing the .a, etc.
VIVSRC_CFLAGS = \
       -I./vivante_gal \
       -I./vivante_fbdev \
       -I./vivante_util \
       -I./vivante_exa

AM_CFLAGS = @XORG_CFLAGS@ $(VIVSRC_CFLAGS) -I/usr/include/drm
vivante_drv_la_LTLIBRARIES = vivante_drv.la
vivante_drv_la_LDFLAGS = -module -avoid-version
vivante_drv_ladir = @moduledir@/drivers

vivante_drv_la_SOURCES = \
    vivante_gal/vivante_gal_blit.c\
    vivante_gal/vivante_gal_gpu.c\
    vivante_gal/vivante_gal_surface.c\
    vivante_fbdev/vivante_fbdev_driver.c\
    vivante_exa/vivante_exa.c\
    vivante_exa/vivante_solid.c\
    vivante_exa/vivante_copy.c\
    vivante_exa/vivante_composite.c\
    vivante_exa/vivante_pixmap.c\
    vivante_fbdev/vivante_dri.c
