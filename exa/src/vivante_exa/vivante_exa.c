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


#include "vivante_exa.h"
#include "vivante_common.h"
#include "vivante_priv.h"
#include "vivante_gal.h"
#include "vivante.h"

void
VivEXASync(ScreenPtr pScreen, int marker) {
/*
    TRACE_ENTER();

    VivPtr pViv = VIVPTR_FROM_SCREEN(pScreen);

    if (!VIV2DGPUBlitComplete(&pViv->mGrCtx, TRUE)) {
        TRACE_ERROR("ERROR with gpu sync\n");
    }

    TRACE_EXIT();
*/
}

void
ConvertXAluToOPS(PixmapPtr pPixmap, int alu, Pixel planemask, int *fg, int *bg) {
	switch (alu)
	{
		case GXcopy:
			*fg = 0xCC;
			*bg = 0xCC;
			break;
		case GXclear:
			*fg = 0xCC;
			*bg = 0x00;
			break;
		case GXand:
			*fg = 0xCC;
			*bg = 0x88;
			break;
		case GXandReverse:
			*fg = 0xCC;
			*bg = 0x88;
			break;
		case GXandInverted:
			/* Not supported */
			break;
		case GXnoop:
			*fg = 0xCC;
			*bg = 0xAA;
			break;
		case GXxor:
			*fg = 0xCC;
			*bg = 0x66;
			break;
		case GXor:
			*fg = 0xCC;
			*bg = 0xEE;
			break;
		case GXnor:
			/* Not supported */
			break;
		case GXequiv:
			/* Not supported */
			break;
		case GXinvert:
			*fg = 0xCC;
			*bg = 0x55;
			break;
		case GXorReverse:
			/* Not supported */
			break;
		case GXcopyInverted:
			*fg = 0xCC;
			*bg = 0x33;
			break;
		case GXorInverted:
			/* Not supported */
			break;
		case GXnand:
			/* Not supported */
			break;
		case GXset:
			*fg = 0xCC;
			*bg = 0xFF;
			break;
		default:
			;
	}

}

Bool
CheckBltvalidity(PixmapPtr pPixmap, int alu, Pixel planemask) {

	TRACE_ENTER();

	switch(alu){
		case GXandInverted:
		case GXnor:
		case GXequiv:
		case GXorReverse:
		case GXorInverted:
		case GXnand:
			TRACE_EXIT(FALSE);
		default:
			;
	}

	/*
	if (alu != GXcopy) {
		TRACE_INFO("FALSE: (alu != GXcopy)\n");
		TRACE_EXIT(FALSE);
	}
	*/

	if (!EXA_PM_IS_SOLID(&pPixmap->drawable, planemask)) {
		TRACE_INFO("FALSE: (!EXA_PM_IS_SOLID(&pPixmap->drawable, planemask))\n");
		TRACE_EXIT(FALSE);
	}

	TRACE_EXIT(TRUE);
}

PixmapPtr
GetDrawablePixmap(DrawablePtr pDrawable) {
    /* Make sure there is a drawable. */
    if (NULL == pDrawable) {
        return NULL;
    }

    /* Check for a backing pixmap. */
    if (DRAWABLE_WINDOW == pDrawable->type) {

        WindowPtr pWindow = (WindowPtr) pDrawable;
        return pDrawable->pScreen->GetWindowPixmap(pWindow);
    }

    /* Otherwise, it's a regular pixmap. */
    return (PixmapPtr) pDrawable;
}


/**
 * UploadToScreen() loads a rectangle of data from src into pDst.
 *
 * @param pDst destination pixmap
 * @param x destination X coordinate.
 * @param y destination Y coordinate
 * @param width width of the rectangle to be copied
 * @param height height of the rectangle to be copied
 * @param src pointer to the beginning of the source data
 * @param src_pitch pitch (in bytes) of the lines of source data.
 *
 * UploadToScreen() copies data in system memory beginning at src (with
 * pitch src_pitch) into the destination pixmap from (x, y) to
 * (x + width, y + height).  This is typically done with hostdata uploads,
 * where the CPU sets up a blit command on the hardware with instructions
 * that the blit data will be fed through some sort of aperture on the card.
 *
 * If UploadToScreen() is performed asynchronously, it is up to the driver
 * to call exaMarkSync().  This is in contrast to most other acceleration
 * calls in EXA.
 *
 * UploadToScreen() can aid in pixmap migration, but is most important for
 * the performance of exaGlyphs() (antialiased font drawing) by allowing
 * pipelining of data uploads, avoiding a sync of the card after each glyph.
 *
 * @return TRUE if the driver successfully uploaded the data.  FALSE
 * indicates that EXA should fall back to doing the upload in software.
 *
 * UploadToScreen() is not required, but is recommended if Composite
 * acceleration is supported.
 */

#define MAXSIZE_FORSWTOSCREEN  160000
typedef Bool(*FUpToScreen)(PixmapPtr pDst, int x, int y, int w, int h, char *src, int src_pitch);

static Bool DoneByNothing(PixmapPtr pDst, int x, int y, int w,
        int h, char *src, int src_pitch) {
    TRACE_EXIT(FALSE);
}

static Bool DoneBySWCPY(PixmapPtr pDst, int x, int y, int w,
        int h, char *src, int src_pitch) {
    VivPtr pViv = VIVPTR_FROM_PIXMAP(pDst);
    Viv2DPixmapPtr pdst = exaGetPixmapDriverPrivate(pDst);
    char * mDestAddr;
    int stride;
    int cpp = (pDst->drawable.bitsPerPixel + 7) / 8;
    int i;

    if (pViv == NULL || pdst == NULL)
        TRACE_EXIT(FALSE);

    stride = GetStride(pdst);
    mDestAddr = (char*) MapViv2DPixmap(pdst);

    if (mDestAddr == NULL)
        TRACE_EXIT(FALSE);

    mDestAddr += y * stride + x *cpp;

    for (i = 0; i < h; i++) {
        memcpy(mDestAddr, src, w * cpp);
        mDestAddr += stride;
        src += src_pitch;
    }

//    pdst->mSwAnyWay = TRUE;
    pdst->mCpuBusy = TRUE;

    TRACE_EXIT(TRUE);
}

static Bool DoneByVSurf(PixmapPtr pDst, int x, int y, int w,
        int h, char *src, int src_pitch) {
    TRACE_ENTER();
    VivPtr pViv = VIVPTR_FROM_PIXMAP(pDst);
    Viv2DPixmapPtr pdst = exaGetPixmapDriverPrivate(pDst);
    VIV2DBLITINFOPTR pBltInfo = &(pViv->mGrCtx.mBlitInfo);
    MemMapInfo mmap;
    int aligned_pitch;
    int height = h;
    int aligned_height;
    int aligned_width;
    char *aligned_start;
    int bytesperpixel;
    int maxsize;
    BOOL retvsurf = TRUE;

    if (pDst->drawable.bitsPerPixel < 16) {
        TRACE_EXIT(FALSE);
    }

    maxsize = (w > h) ? w : h;

    switch (pDst->drawable.bitsPerPixel) {

        case 16:
            bytesperpixel = 2;
            retvsurf = VGetSurfAddrBy16(&pViv->mGrCtx, maxsize, (int *) (&mmap.physical), (int *) (&(mmap.mUserAddr)), &aligned_width, &aligned_height, &aligned_pitch);

            break;
        case 32:
            bytesperpixel = 4;
            retvsurf = VGetSurfAddrBy32(&pViv->mGrCtx, maxsize, (int *) (&mmap.physical), (int *) (&(mmap.mUserAddr)), &aligned_width, &aligned_height, &aligned_pitch);
            break;
        default:
            return FALSE;
    }

    if (retvsurf == FALSE)
        TRACE_EXIT(FALSE);

    mmap.mapping = NULL;
    mmap.mSize = aligned_pitch*aligned_width;

    aligned_start = (char *) mmap.mUserAddr;

    while (height--) {
        memcpy(aligned_start, src, w * bytesperpixel);
        src += src_pitch;
        aligned_start += aligned_pitch;
    }

    if (!GetDefaultFormat(pDst->drawable.bitsPerPixel, &(pBltInfo->mDstSurfInfo.mFormat))) {
        TRACE_EXIT(FALSE);
    }
    pBltInfo->mSrcSurfInfo.mFormat = pBltInfo->mDstSurfInfo.mFormat;

    pBltInfo->mDstBox.x1 = x;
    pBltInfo->mDstBox.y1 = y;
    pBltInfo->mDstBox.x2 = x + w;
    pBltInfo->mDstBox.y2 = y + h;

    pBltInfo->mSrcBox.x1 = 0;
    pBltInfo->mSrcBox.y1 = 0;
    pBltInfo->mSrcBox.x2 = w;
    pBltInfo->mSrcBox.y2 = h;

    pBltInfo->mDstSurfInfo.mHeight = pDst->drawable.height;
    pBltInfo->mDstSurfInfo.mWidth = pDst->drawable.width;
    pBltInfo->mDstSurfInfo.mStride = pDst->devKind;
    pBltInfo->mDstSurfInfo.mPriv = pdst;

    pBltInfo->mSrcSurfInfo.mStride = aligned_pitch;
    pBltInfo->mSrcSurfInfo.mWidth = aligned_width;
    pBltInfo->mSrcSurfInfo.mHeight = aligned_height;

    pBltInfo->mBgRop = 0xCC;
    pBltInfo->mFgRop = 0xCC;

/*
    if (pdst->mCpuBusy) {
       VIV2DCacheOperation(&pViv->mGrCtx,pdst,FLUSH);
    }
*/
    if (!CopyBlitFromHost(&mmap, &pViv->mGrCtx)) {
        TRACE_ERROR("Copy Blit From Host Failed\n");
        TRACE_EXIT(FALSE);
    }

    VIV2DGPUBlitComplete(&pViv->mGrCtx, TRUE);
/*
    if (pdst->mCpuBusy) {
       VIV2DCacheOperation(&pViv->mGrCtx,pdst,INVALIDATE);
       pdst->mCpuBusy = FALSE;
    }
*/
}

static Bool DoneByMapFuncs(PixmapPtr pDst, int x, int y, int w,
        int h, char *src, int src_pitch) {
    TRACE_ENTER();
    VivPtr pViv = VIVPTR_FROM_PIXMAP(pDst);
    Viv2DPixmapPtr pdst = exaGetPixmapDriverPrivate(pDst);
    VIV2DBLITINFOPTR pBltInfo = &(pViv->mGrCtx.mBlitInfo);
    MemMapInfo mmap;
    int width_in_bytes = w * bits_to_bytes(pDst->drawable.bitsPerPixel);
    int aligned_pitch = VIV_ALIGN(width_in_bytes, PIXMAP_PITCH_ALIGN);
    int height = h;
    int aligned_width = VIV_ALIGN(w, 8);
    void * start;
    char *aligned_start;
    mmap.mSize = h * aligned_pitch;
    mmap.mapping = NULL;
    mmap.physical = 0;
    start = calloc(1, mmap.mSize + 64);
    mmap.mUserAddr = aligned_start = (char*) VIV_ALIGN(((int) start), 64);

    while (height--) {
        memcpy(aligned_start, src, width_in_bytes);
        src += src_pitch;
        aligned_start += aligned_pitch;
    }

    if (!GetDefaultFormat(pDst->drawable.bitsPerPixel, &(pBltInfo->mDstSurfInfo.mFormat))) {
        TRACE_EXIT(FALSE);
    }
    pBltInfo->mSrcSurfInfo.mFormat = pBltInfo->mDstSurfInfo.mFormat;

    pBltInfo->mDstBox.x1 = x;
    pBltInfo->mDstBox.y1 = y;
    pBltInfo->mDstBox.x2 = x + w;
    pBltInfo->mDstBox.y2 = y + h;

    pBltInfo->mSrcBox.x1 = 0;
    pBltInfo->mSrcBox.y1 = 0;
    pBltInfo->mSrcBox.x2 = w;
    pBltInfo->mSrcBox.y2 = h;

    pBltInfo->mDstSurfInfo.mHeight = pDst->drawable.height;
    pBltInfo->mDstSurfInfo.mWidth = pDst->drawable.width;
    pBltInfo->mDstSurfInfo.mStride = pDst->devKind;
    pBltInfo->mDstSurfInfo.mPriv = pdst;

    pBltInfo->mSrcSurfInfo.mStride = aligned_pitch;
    pBltInfo->mSrcSurfInfo.mWidth = aligned_width;
    pBltInfo->mSrcSurfInfo.mHeight = h;

    pBltInfo->mBgRop = 0xCC;
    pBltInfo->mFgRop = 0xCC;

    if (!MapUserMemToGPU(&pViv->mGrCtx, &mmap)) {
        TRACE_EXIT(FALSE);
    }

    if (!CopyBlitFromHost(&mmap, &pViv->mGrCtx)) {
        UnmapUserMem(&pViv->mGrCtx, &mmap);
        TRACE_ERROR("Copy Blit From Host Failed\n");
        TRACE_EXIT(FALSE);
    }

    VIV2DGPUBlitComplete(&pViv->mGrCtx, TRUE);
    UnmapUserMem(&pViv->mGrCtx, &mmap);
    //exaMarkSync(pDst->drawable.pScreen);
    free(start);
    TRACE_EXIT(TRUE);
}

typedef enum {
    DONE_BY_NOTHING, DONE_BY_SWCPY, DONE_BY_VSURF, DONE_BY_MAPFUNCS
} FUPSCREENTYPE;

static FUpToScreen _fptoscreen[4] = {DoneByNothing, DoneBySWCPY, DoneByVSurf, DoneByMapFuncs};

static FUPSCREENTYPE ftype = /*DONE_BY_SWCPY*/DONE_BY_VSURF;
Bool
VivUploadToScreen(PixmapPtr pDst, int x, int y, int w,
	int h, char *src, int src_pitch) {

	if ( ( w*h ) < MAXSIZE_FORSWTOSCREEN )
		ftype = DONE_BY_SWCPY;
	else
		ftype = DONE_BY_VSURF;

	return _fptoscreen[ftype](pDst, x, y, w, h, src, src_pitch);
}
