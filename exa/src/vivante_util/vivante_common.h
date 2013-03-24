/****************************************************************************
*
*    Copyright (C) 2005 - 2012 by Vivante Corp.
*
*    This program is free software; you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation; either version 2 of the license, or
*    (at your option) any later version.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program; if not write to the Free Software
*    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*
*****************************************************************************/


/*
 * File:   vivante_common.h
 * Author: vivante
 *
 * Created on February 18, 2012, 7:15 PM
 */

#ifndef VIVANTE_COMMON_H
#define	VIVANTE_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif
    /*Normal Includes*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


    /* X Window */
#include "xorg-server.h"
#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86xv.h"
#include "xf86RandR12.h"
#include "xorg-server.h"


#include "mipointer.h"
#include "mibstore.h"
#include "micmap.h"
#include "mipointrst.h"
#include "inputstr.h"
#include "colormapst.h"
#include "xf86cmap.h"
#include "shadow.h"
#include "dgaproc.h"

    /* for visuals */
#include "fb.h"

#if GET_ABI_MAJOR(ABI_VIDEODRV_VERSION) < 6
#include "xf86Resources.h"
#include "xf86RAC.h"
#endif

    /*FrameBuffer*/
#include "fbdevhw.h"
    /*For EXA*/
#include "exa.h"

    /*For Cursor*/
#include "xf86.h"
#include "xf86Cursor.h"
#include "xf86Crtc.h"
#include "cursorstr.h"

    /* System API compatability */
#include "compat-api.h"

    /*Debug*/
#include "vivante_debug.h"

#define V_MIN(a,b) ((a)>(b)?(b):(a))
#define V_MAX(a,b) ((a)>(b)?(a):(b))

#define USE_GPU_FB_MEM_MAP 0

#define WIDTH_ALIGNMENT 8
#define HEIGHT_ALIGNMENT 1
#define BITSTOBYTES(x) (((x)+7)/8)

#define	IMX_EXA_NONCACHESURF_WIDTH 650
#define	IMX_EXA_NONCACHESURF_HEIGHT 650

#define	SURF_SIZE_FOR_SW(sw,sh) do {	\
						if ( gcmALIGN(sw, WIDTH_ALIGNMENT) < IMX_EXA_NONCACHESURF_WIDTH	\
						|| gcmALIGN(sh, HEIGHT_ALIGNMENT) < IMX_EXA_NONCACHESURF_HEIGHT)	\
						TRACE_EXIT(FALSE);	\
				} while ( 0 )


#ifdef __cplusplus
}
#endif

#endif	/* VIVANTE_COMMON_H */

