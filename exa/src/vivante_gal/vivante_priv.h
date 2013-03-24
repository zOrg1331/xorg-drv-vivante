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
 * File:   vivante_priv.h
 * Author: vivante
 *
 * Created on February 23, 2012, 12:33 PM
 */

#ifndef VIVANTE_PRIV_H
#define	VIVANTE_PRIV_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAL/gc_hal.h"
#include "HAL/gc_hal_raster.h"
#include "HAL/gc_hal_base.h"

    /************************************************************************
     * PIXMAP_HANDLING_STUFF(START)
     ************************************************************************/
    typedef struct {
        gcuVIDMEM_NODE_PTR mNode;
        gcePOOL mPool;
        gctUINT mSizeInBytes;
        gctUINT32 mPhysicalAddr;
        gctPOINTER mLogicalAddr;
    } VideoNode, *VideoNodePtr;

    typedef struct {
        gctBOOL mIsWrapped;
        gceSURF_ROTATION mRotation;
        gceTILING mTiling;
        gctUINT32 mAlignedWidth;
        gctUINT32 mAlignedHeight;
        gctUINT32 mBytesPerPixel;
        gctPOINTER mLogicalAddr;
        gctUINT32 mStride;
        VideoNode mVideoNode;
        gctPOINTER mData;
    } GenericSurface, *GenericSurfacePtr;

    /************************************************************************
     * PIXMAP_HANDLING_STUFF (END)
     ************************************************************************/

    /**************************************************************************
     * DRIVER & DEVICE  Structs (START)
     *************************************************************************/
    typedef struct _viv2DDriver {
        /*Base Objects*/
        gcoOS mOs;
        gcoHAL mHal;
        gco2D m2DEngine;
        gcoBRUSH mBrush;

        /*video memory mapping*/
        gctPHYS_ADDR g_InternalPhysical, g_ExternalPhysical, g_ContiguousPhysical;
        gctSIZE_T g_InternalSize, g_ExternalSize, g_ContiguousSize;
        gctPOINTER g_Internal, g_External, g_Contiguous;

        /* HW specific features. */
        gctBOOL mIsSeperated;
        gctBOOL mIsPe20Supported;
        gctBOOL mIsMultiSrcBltSupported;
        gctBOOL mIsMultiSrcBltExSupported;
        gctUINT mMaxSourceForMultiSrcOpt;
    } Viv2DDriver, *Viv2DDriverPtr;

    typedef struct _viv2DDevice {
        gceCHIPMODEL mChipModel; /*chip model */
        unsigned int mChipRevision; /* chip revision */
        unsigned int mChipFeatures; /* chip features */
        unsigned int mChipMinorFeatures; /* chip minor features */
    } Viv2DDevice, *Viv2DDevicePtr;

    typedef struct _vivanteGpu {
        Viv2DDriverPtr mDriver;
        Viv2DDevicePtr mDevice;
    } VIVGPU, *VIVGPUPtr;

    /**************************************************************************
     * DRIVER & DEVICE  Structs (END)
     *************************************************************************/

#ifdef __cplusplus
}
#endif

#endif	/* VIVANTE_PRIV_H */

