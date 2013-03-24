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

#include "vivante.h"

#include "vivante_priv.h"

/**
 *
 * @param pScreen
 * @param size
 * @param align
 * @return private vivpixmap
 */

void *
VivCreatePixmap(ScreenPtr pScreen, int size, int align) {
	TRACE_ENTER();
	Viv2DPixmapPtr vivpixmap = NULL;
	vivpixmap = malloc(sizeof (Viv2DPixmap));
	if (!vivpixmap) {
		TRACE_EXIT(NULL);
	}
	IGNORE(align);
	IGNORE(size);
	vivpixmap->mVidMemInfo = NULL;
	vivpixmap->mGpuBusy = FALSE;
	vivpixmap->mCpuBusy = FALSE;
	vivpixmap->mSwAnyWay = FALSE;
	vivpixmap->mNextGpuBusyPixmap = NULL;
	vivpixmap->mRef = 0;
	TRACE_EXIT(vivpixmap);
}

/**
 * Destroys the Pixmap
 * @param pScreen
 * @param dPriv
 */
void
VivDestroyPixmap(ScreenPtr pScreen, void *dPriv) {
	TRACE_ENTER();
	Viv2DPixmapPtr priv = (Viv2DPixmapPtr) dPriv;
	VivPtr pViv = VIVPTR_FROM_SCREEN(pScreen);
	if (priv && (priv->mVidMemInfo)) {

		if (!DestroySurface(&pViv->mGrCtx, priv)) {
			TRACE_ERROR("Error on destroying the surface\n");
		}
		/*Removing the container*/
		free(priv);
		priv = NULL;
		dPriv = NULL;
	}
	TRACE_EXIT();
}

/**
 * PixmapIsOffscreen() is an optional driver replacement to
 * exaPixmapHasGpuCopy(). Set to NULL if you want the standard behaviour
 * of exaPixmapHasGpuCopy().
 *
 * @param pPix the pixmap
 * @return TRUE if the given drawable is in framebuffer memory.
 *
 * exaPixmapHasGpuCopy() is used to determine if a pixmap is in offscreen
 * memory, meaning that acceleration could probably be done to it, and that it
 * will need to be wrapped by PrepareAccess()/FinishAccess() when accessing it
 * with the CPU.
 */
Bool
VivPixmapIsOffscreen(PixmapPtr pPixmap) {
	TRACE_ENTER();
	BOOL ret = FALSE;
	Viv2DPixmapPtr vivpixmap = NULL;
	ScreenPtr pScreen = pPixmap->drawable.pScreen;
	vivpixmap = (Viv2DPixmapPtr) exaGetPixmapDriverPrivate(pPixmap);

	/* offscreen means in 'gpu accessible memory', not that it's off the
	* visible screen.
	*/
	if (pScreen->GetScreenPixmap(pScreen) == pPixmap) {
		TRACE_EXIT(TRUE);
	}

	ret = pPixmap->devPrivate.ptr ? FALSE : TRUE;

	TRACE_EXIT(ret);
}

static Bool
VivPrepareSolidWithoutSizeCheck(PixmapPtr pPixmap, int alu, Pixel planemask, Pixel fg) {
	TRACE_ENTER();
	Viv2DPixmapPtr pdst = exaGetPixmapDriverPrivate(pPixmap);
	VivPtr pViv = VIVPTR_FROM_PIXMAP(pPixmap);

	if (!GetDefaultFormat(pPixmap->drawable.bitsPerPixel, &(pViv->mGrCtx.mBlitInfo.mDstSurfInfo.mFormat))) {
		TRACE_EXIT(FALSE);
	}
	/*Populating the information*/
	pViv->mGrCtx.mBlitInfo.mDstSurfInfo.mHeight = pPixmap->drawable.height;
	pViv->mGrCtx.mBlitInfo.mDstSurfInfo.mWidth = pPixmap->drawable.width;
	pViv->mGrCtx.mBlitInfo.mDstSurfInfo.mStride = pPixmap->devKind;
	pViv->mGrCtx.mBlitInfo.mDstSurfInfo.mPriv = pdst;
	pViv->mGrCtx.mBlitInfo.mFgRop = 0xF0;
	pViv->mGrCtx.mBlitInfo.mBgRop = 0xF0;
	pViv->mGrCtx.mBlitInfo.mColorARGB32 = fg;
	pViv->mGrCtx.mBlitInfo.mColorConvert = FALSE;
	pViv->mGrCtx.mBlitInfo.mPlaneMask = planemask;
	pViv->mGrCtx.mBlitInfo.mOperationCode = VIVSOLID;

	TRACE_EXIT(TRUE);

}

/**
 * Returning a pixmap with non-NULL devPrivate.ptr implies a pixmap which is
 * not offscreen, which will never be accelerated and Prepare/FinishAccess won't
 * be called.
 */
Bool
VivModifyPixmapHeader(PixmapPtr pPixmap, int width, int height,
	int depth, int bitsPerPixel, int devKind,
	pointer pPixData) {
	TRACE_ENTER();
	Bool ret = FALSE;
	GenericSurfacePtr surf;
	Bool isChanged = FALSE;
	int prev_w = pPixmap->drawable.width;
	int prev_h = pPixmap->drawable.height;
	int prev_bpp = pPixmap->drawable.bitsPerPixel;
	VivPtr pViv = VIVPTR_FROM_PIXMAP(pPixmap);
	Viv2DPixmapPtr vivPixmap = exaGetPixmapDriverPrivate(pPixmap);

	if (!pPixmap || !vivPixmap) {
		TRACE_EXIT(FALSE);
	}
	ret = miModifyPixmapHeader(pPixmap, width, height, depth, bitsPerPixel, devKind, pPixData);
	if (!ret) {
		 return ret;
	}

	if (depth <= 0) {
		depth = pPixmap->drawable.depth;
	}

	if (bitsPerPixel <= 0) {
		bitsPerPixel = pPixmap->drawable.bitsPerPixel;
	}

	if (width <= 0) {
		width = pPixmap->drawable.width;
	}

	if (height <= 0) {
		height = pPixmap->drawable.height;
	}

	if (width <= 0 || height <= 0 || depth <= 0) {
		TRACE_EXIT(FALSE);

	}

	isChanged = (!vivPixmap->mVidMemInfo || prev_h != height || prev_w != width || (prev_bpp != bitsPerPixel && bitsPerPixel > 16));


	/* What is the start of screen (and offscreen) memory and its size. */

	CARD8* screenMemoryBegin =

	(CARD8*) (pViv->mFakeExa.mExaDriver->memoryBase);

	CARD8* screenMemoryEnd =screenMemoryBegin + pViv->mFakeExa.mExaDriver->memorySize;

	if ((screenMemoryBegin <= (CARD8*) (pPixData)) &&
		((CARD8*) (pPixData) < screenMemoryEnd)) {

		/* Compute address relative to begin of FB memory. */

		const unsigned long offset =(CARD8*) (pPixData) - screenMemoryBegin;

		/* Store GPU address. */
		const unsigned long physical = pViv->mFB.memPhysBase + offset;
		if (!WrapSurface(pPixmap, pPixData, physical, vivPixmap)) {

			TRACE_ERROR("Frame Buffer Wrapping ERROR\n");
			TRACE_EXIT(FALSE);
		}

		TRACE_EXIT(TRUE);

	} else if (pPixData) {

		TRACE_ERROR("NO ACCERELATION\n");

		/*No acceleration is avalaible*/

		pPixmap->devPrivate.ptr = pPixData;

		pPixmap->devKind = devKind;

		VIV2DCacheOperation(&pViv->mGrCtx, vivPixmap,FLUSH);
		VIV2DGPUBlitComplete(&pViv->mGrCtx, TRUE);
		/*we never want to see this again*/
		if (!DestroySurface(&pViv->mGrCtx, vivPixmap)) {

			TRACE_ERROR("ERROR : DestroySurface\n");
		}

		vivPixmap->mVidMemInfo = NULL;
		TRACE_EXIT(FALSE);

	} else {

		if (isChanged) {

			if ( !ReUseSurface(&pViv->mGrCtx, pPixmap, vivPixmap) )
			{
			if (!DestroySurface(&pViv->mGrCtx, vivPixmap)) {
				TRACE_ERROR("ERROR : Destroying the surface\n");
				fprintf(stderr,"Destroy surface failed\n");
				TRACE_EXIT(FALSE);
			}

			if (!CreateSurface(&pViv->mGrCtx, pPixmap, vivPixmap)) {
				TRACE_ERROR("ERROR : Creating the surface\n");
				fprintf(stderr,"CreateSurface failed\n");
				TRACE_EXIT(FALSE);
				}
			}

			pPixmap->devKind = GetStride(vivPixmap);

			/* Clean the new surface with black color in case the window gets scrambled image when the window is resized */
			/* Use VivPrepareSolidWithoutSizeCheck instead of VivPrepareSolid, if VivPrepareSolid failed to test, no path to go */
			/* So that we can't clear surface */
			if(VivPrepareSolidWithoutSizeCheck(pPixmap,(int)GXcopy,(Pixel)0xFFFFFFFF,(Pixel)0)){
				VivSolid(pPixmap,0,0,pPixmap->drawable.width,pPixmap->drawable.height);
				VivDoneSolid(pPixmap);
			}


		}

	}

	TRACE_EXIT(TRUE);
}

/**
 * PrepareAccess() is called before CPU access to an offscreen pixmap.
 *
 * @param pPix the pixmap being accessed
 * @param index the index of the pixmap being accessed.
 *
 * PrepareAccess() will be called before CPU access to an offscreen pixmap.
 * This can be used to set up hardware surfaces for byteswapping or
 * untiling, or to adjust the pixmap's devPrivate.ptr for the purpose of
 * making CPU access use a different aperture.
 *
 * The index is one of #EXA_PREPARE_DEST, #EXA_PREPARE_SRC,
 * #EXA_PREPARE_MASK, #EXA_PREPARE_AUX_DEST, #EXA_PREPARE_AUX_SRC, or
 * #EXA_PREPARE_AUX_MASK. Since only up to #EXA_NUM_PREPARE_INDICES pixmaps
 * will have PrepareAccess() called on them per operation, drivers can have
 * a small, statically-allocated space to maintain state for PrepareAccess()
 * and FinishAccess() in.  Note that PrepareAccess() is only called once per
 * pixmap and operation, regardless of whether the pixmap is used as a
 * destination and/or source, and the index may not reflect the usage.
 *
 * PrepareAccess() may fail.  An example might be the case of hardware that
 * can set up 1 or 2 surfaces for CPU access, but not 3.  If PrepareAccess()
 * fails, EXA will migrate the pixmap to system memory.
 * DownloadFromScreen() must be implemented and must not fail if a driver
 * wishes to fail in PrepareAccess().  PrepareAccess() must not fail when
 * pPix is the visible screen, because the visible screen can not be
 * migrated.
 *
 * @return TRUE if PrepareAccess() successfully prepared the pixmap for CPU
 * drawing.
 * @return FALSE if PrepareAccess() is unsuccessful and EXA should use
 * DownloadFromScreen() to migrate the pixmap out.
 */
Bool
VivPrepareAccess(PixmapPtr pPix, int index) {
	TRACE_ENTER();
	Viv2DPixmapPtr vivpixmap = exaGetPixmapDriverPrivate(pPix);
	VivPtr pViv = VIVPTR_FROM_PIXMAP(pPix);

	if (vivpixmap->mRef == 0) {
		pPix->devPrivate.ptr = MapSurface(vivpixmap);
	}

	vivpixmap->mRef++;

	if (pPix->devPrivate.ptr == NULL) {

		TRACE_ERROR("Logical Address is not set\n");
		TRACE_EXIT(FALSE);

	}

	vivpixmap->mCpuBusy=TRUE;
	TRACE_EXIT(TRUE);
}

/**
 * FinishAccess() is called after CPU access to an offscreen pixmap.
 *
 * @param pPix the pixmap being accessed
 * @param index the index of the pixmap being accessed.
 *
 * FinishAccess() will be called after finishing CPU access of an offscreen
 * pixmap set up by PrepareAccess().  Note that the FinishAccess() will not be
 * called if PrepareAccess() failed and the pixmap was migrated out.
 */
void
VivFinishAccess(PixmapPtr pPix, int index) {
	TRACE_ENTER();
	Viv2DPixmapPtr vivpixmap = exaGetPixmapDriverPrivate(pPix);
	VivPtr pViv = VIVPTR_FROM_PIXMAP(pPix);
	IGNORE(index);


	if (vivpixmap->mRef == 1) {

		pPix->devPrivate.ptr = NULL;

	}
	vivpixmap->mRef--;
	TRACE_EXIT();
}


