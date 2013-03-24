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
 * File:   vivante_gal.h
 * Author: vivante
 *
 * Created on February 23, 2012, 11:58 AM
 */

#ifndef VIVANTE_GAL_H
#define	VIVANTE_GAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAL/gc_hal.h"

#include "HAL/gc_hal_raster.h"

#include "HAL/gc_hal_base.h"

    /*******************************************************************************
     *
     * Utility Macros (START)
     *
     ******************************************************************************/
#define IGNORE(a)  (a=a)
#define VIV_ALIGN( value, base ) (((value) + ((base) - 1)) & ~((base) - 1))
#define ARRAY_SIZE(a) (sizeof((a)) / (sizeof(*(a))))
#define NO_PICT_FORMAT -1
    /*******************************************************************************
     *
     * Utility Macros (END)
     *
     *******************************************************************************/

    /************************************************************************
     * STRUCTS & ENUMS(START)
     ************************************************************************/

    /*Memory Map Info*/
    typedef struct _mmInfo {
        unsigned int mSize;
        void * mUserAddr;
        void* mapping;
        unsigned int physical;
    } MemMapInfo, *MemMapInfoPtr;

    /*Cache Ops*/
    typedef enum _cacheOps {
        CLEAN,
        INVALIDATE,
        FLUSH,
        MEMORY_BARRIER
    } VIVFLUSHTYPE;

    /*Blit Code*/
    typedef enum _blitOpCode {
        VIVSOLID = 0,
        VIVSIMCOPY,
        VIVCOPY,
        VIVCOMPOSITE_MASKED_SRC_REPEAT_PIXEL_ONLY_PATTERN,
        VIVCOMPOSITE_MASKED_SRC_REPEAT_ARBITRARY_SIZE_PATTERN,
        VIVCOMPOSITE_MASKED_SIMPLE,
        VIVCOMPOSITE_SRC_REPEAT_PIXEL_ONLY_PATTERN,
        VIVCOMPOSITE_SRC_REPEAT_ARBITRARY_SIZE_PATTERN,
        VIVCOMPOSITE_SIMPLE
    } BlitCode;

    /*Format information*/
    typedef struct _vivPictFormats {
        int mExaFmt;
        int mBpp;
        unsigned int mVivFmt;
        int mAlphaBits;
    } VivPictFormat, *VivPictFmtPtr;

    /*Blending Operations*/
    typedef struct _vivBlendOps {
        int mOp;
        int mSrcBlendingFactor;
        int mDstBlendingFactor;
    } VivBlendOp, *VivBlendOpPtr;

    /*Rectangle*/
    typedef struct _vivBox {
        int x1;
        int y1;

        union {

            struct {
                int x2;
                int y2;
            };

            struct {
                int width;
                int height;
            };
        };
    } VivBox, *VivBoxPtr;

    /*Prv Pixmap Structure*/
    typedef struct _vivPixmapPriv Viv2DPixmap;
    typedef Viv2DPixmap * Viv2DPixmapPtr;

    struct _vivPixmapPriv {
        /*Video Memory*/
        void * mVidMemInfo;
        /* Tracks pixmaps busy with GPU operation since last GPU sync. */
        Bool mGpuBusy;
        Bool mCpuBusy;
        Bool mSwAnyWay;
        Viv2DPixmapPtr mNextGpuBusyPixmap;
        /*reference*/
        int mRef;
    };

    /*Surface Info*/
    typedef struct _vivSurfInfo {
        Viv2DPixmapPtr mPriv;
        VivPictFormat mFormat;
        unsigned int mWidth;
        unsigned int mHeight;
        unsigned int mStride;
        unsigned int repeat;
        unsigned int repeatType;
    } VIV2DSURFINFO;

    /*Blit Info*/
    typedef struct _viv2DBlitInfo {
        /*Destination*/
        VIV2DSURFINFO mDstSurfInfo;
        /*Source*/
        VIV2DSURFINFO mSrcSurfInfo;
        /*Mask*/
        VIV2DSURFINFO mMskSurfInfo;
        /*BlitCode*/
        BlitCode mOperationCode;
        /*Operation Related*/
        VivBox mSrcBox;
        VivBox mDstBox;
        VivBox mMskBox;
        /*Foreground and Background ROP*/
        int mFgRop;
        int mBgRop;
        /*Blending opeartion*/
        VivBlendOp mBlendOp;
        /*Transformation for source*/
        PictTransformPtr mTransform;
        Pixel mColorARGB32; /*A8R8G8B8*/
        Bool mColorConvert;
        unsigned long mPlaneMask;
        /*Rotation for source*/
        gceSURF_ROTATION mRotation;
        Bool mSwcpy;
        Bool mSwsolid;
        Bool mSwcmp;
        Bool mIsNotStretched;
        /* record old srcBox and dstBox */
        VivBox mOSrcBox;
        VivBox mODstBox;
    } VIV2DBLITINFO, *VIV2DBLITINFOPTR;

    /*Gal Encapsulation*/
    typedef struct _GALINFO {
        /*Encapsulated blit info*/
        VIV2DBLITINFO mBlitInfo;
        /*Gpu busy pixmap linked list */
        Viv2DPixmapPtr mBusyPixmapLinkList;
        /*Gpu related*/
        void * mGpu;
    } GALINFO, *GALINFOPTR;


    /* Format convertor */
    Bool VIVTransformSupported(PictTransform *ptransform,Bool *stretchflag);
    gceSURF_ROTATION VIVGetRotation(PictTransform *ptransform);
    void VIVGetSourceWH(PictTransform *ptransform, gctUINT32 deswidth, gctUINT32 desheight, gctUINT32 *srcwidth, gctUINT32 *srcheight );
    /************************************************************************
     * STRUCTS & ENUMS (END)
     ************************************************************************/

    /************************************************************************
     * PIXMAP RELATED (START)
     ************************************************************************/
    /*Creating and Destroying Functions*/
    Bool CreateSurface(GALINFOPTR galInfo, PixmapPtr pPixmap, Viv2DPixmapPtr toBeUpdatedpPix);
    Bool WrapSurface(PixmapPtr pPixmap, void * logical, unsigned int physical, Viv2DPixmapPtr toBeUpdatedpPix);
    Bool ReUseSurface(GALINFOPTR galInfo, PixmapPtr pPixmap, Viv2DPixmapPtr toBeUpdatedpPix);
    Bool DestroySurface(GALINFOPTR galInfo, Viv2DPixmapPtr ppriv);
    unsigned int GetStride(Viv2DPixmapPtr pixmap);
    /*Mapping Functions*/
    void * MapSurface(Viv2DPixmapPtr priv);
    void UnMapSurface(Viv2DPixmapPtr priv);
    /************************************************************************
     * PIXMAP RELATED (END)
     ************************************************************************/

    /************************************************************************
     * EXA RELATED UTILITY (START)
     ************************************************************************/
    Bool GetVivPictureFormat(int exa_fmt, VivPictFmtPtr viv);
    Bool GetDefaultFormat(int bpp, VivPictFmtPtr format);
    char *MapViv2DPixmap(Viv2DPixmapPtr pdst);
    Bool VGetSurfAddrBy16(GALINFOPTR galInfo, int maxsize, int *phyaddr, int *lgaddr, int *width, int *height, int *stride);
    Bool VGetSurfAddrBy32(GALINFOPTR galInfo, int maxsize, int *phyaddr, int *lgaddr, int *width, int *height, int *stride);
    void VDestroySurf();
    /************************************************************************
     *EXA RELATED UTILITY (END)
     ************************************************************************/

    /************************************************************************
     * GPU RELATED (START)
     ************************************************************************/
    Bool VIV2DGPUBlitComplete(GALINFOPTR galInfo, Bool wait);
    Bool VIV2DGPUFlushGraphicsPipe(GALINFOPTR galInfo);
    Bool VIV2DGPUCtxInit(GALINFOPTR galInfo);
    Bool VIV2DGPUCtxDeInit(GALINFOPTR galInfo);
    Bool VIV2DCacheOperation(GALINFOPTR galInfo, Viv2DPixmapPtr ppix, VIVFLUSHTYPE flush_type);
#if USE_GPU_FB_MEM_MAP
    Bool VIV2DGPUUserMemMap(char* logical, unsigned int physical, unsigned int size, void ** mappingInfo, unsigned int * gpuAddress);
    Bool VIV2DGPUUserMemUnMap(char* logical, unsigned int size, void * mappingInfo, unsigned int gpuAddress);
#endif
    Bool MapUserMemToGPU(GALINFOPTR galInfo, MemMapInfoPtr mmInfo);
    void UnmapUserMem(GALINFOPTR galInfo, MemMapInfoPtr mmInfo);
    /************************************************************************
     * GPU RELATED (END)
     ************************************************************************/

    /************************************************************************
     * 2D Operations (START)
     ************************************************************************/
    Bool SetSolidBrush(GALINFOPTR galInfo);
    Bool SetDestinationSurface(GALINFOPTR galInfo);
    Bool SetSourceSurface(GALINFOPTR galInfo);
    Bool SetClipping(GALINFOPTR galInfo);
    void VIVSWComposite(PixmapPtr pxDst, int srcX, int srcY, int maskX, int maskY, int dstX, int dstY, int width, int height);
    Bool DoCompositeBlit(GALINFOPTR galInfo, VivBoxPtr opbox);
    Bool DoSolidBlit(GALINFOPTR galInfo);
    Bool DoCopyBlit(GALINFOPTR galInfo);
    Bool CopyBlitFromHost(MemMapInfoPtr mmInfo, GALINFOPTR galInfo);
    /************************************************************************
     * 2D Operations (END)
     ************************************************************************/
#ifdef __cplusplus
}
#endif

#endif	/* VIVANTE_GAL_H */

