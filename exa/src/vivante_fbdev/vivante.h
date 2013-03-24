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
 * File:   vivante.h
 * Author: vivante
 *
 * Created on November 9, 2011, 6:58 PM
 */

#ifndef VIVANTE_H
#define	VIVANTE_H

#ifdef __cplusplus
extern "C" {
#endif

    /*GAL*/
#include "vivante_gal.h"

#define VIV_MAX_WIDTH   (1 <<11)
#define VIV_MAX_HEIGHT (1 <<11)
#define PIXMAP_PITCH_ALIGN    64

    /********************************************************************************
     *  Rectangle Structs (START)
     ********************************************************************************/

    /* Supported options */
    typedef enum {
        OPTION_VIV,
        OPTION_NOACCEL,
        OPTION_ACCELMETHOD
    } VivOpts;

    typedef struct _vivFakeExa {
        ExaDriverPtr mExaDriver;
        /*Feature Switches  For Exa*/
        Bool mNoAccelFlag;
        Bool mUseExaFlag;
        Bool mIsInited;
        /*Fake EXA Operations*/
        int op;
        PicturePtr pSrcPicture, pMaskPicture, pDstPicture;
        PixmapPtr pDst, pSrc, pMask;
        GCPtr pGC;
        CARD32 copytmpval[2];
        CARD32 solidtmpval[3];
    } VivFakeExa, *VivFakeExaPtr;

    typedef struct _fbInfo {
        void * mMappingInfo;
        unsigned long memPhysBase;
        unsigned char* mFBStart; /*logical memory start address*/
        unsigned char* mFBMemory; /*memory  address*/
        int mFBOffset; /*framebuffer offset*/
    } FBINFO, *FBINFOPTR;

    typedef struct _vivRec {
        /*Graphics Context*/
        GALINFO mGrCtx;
        /*FBINFO*/
        FBINFO mFB;
        /*EXA STUFF*/
        VivFakeExa mFakeExa;
        /*Entity & Options*/
        EntityInfoPtr mEntity; /*Entity To Be Used with this driver*/
        OptionInfoPtr mSupportedOptions; /*Options to be parsed in xorg.conf*/
        /*Funct Pointers*/
        CloseScreenProcPtr CloseScreen; /*Screen Close Function*/
        CreateScreenResourcesProcPtr CreateScreenResources;

        /* DRI information */
        void * pDRIInfo;
        int drmSubFD;
    } VivRec, * VivPtr;

    /********************************************************************************
     *  Rectangle Structs (END)
     ********************************************************************************/
#define GET_VIV_PTR(p) ((VivPtr)((p)->driverPrivate))

#define VIVPTR_FROM_PIXMAP(x)		\
		GET_VIV_PTR(xf86Screens[(x)->drawable.pScreen->myNum])
#define VIVPTR_FROM_SCREEN(x)		\
		GET_VIV_PTR(xf86Screens[(x)->myNum])
#define VIVPTR_FROM_PICTURE(x)	\
		GET_VIV_PTR(xf86Screens[(x)->pDrawable->pScreen->myNum])

    /********************************************************************************
     *
     *  Macros For Access (END)
     *
     ********************************************************************************/

#ifdef __cplusplus
}
#endif

#endif	/* VIVANTE_H */

