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
 * File:   vivante_exa.h
 * Author: vivante
 *
 * Created on February 23, 2012, 11:10 AM
 */

#ifndef VIVANTE_EXA_H
#define	VIVANTE_EXA_H

#ifdef __cplusplus
extern "C" {
#endif

#include "vivante_common.h"

#define VIV_EXA_FLUSH_2D_CMD_ENABLE         1
#define VIV_EXA_SOLID_SIZE_CHECK_ENABLE     1
#define VIV_EXA_COPY_SIZE_CHECK_ENABLE      1

#define	IMX_EXA_MIN_PIXEL_AREA_SOLID        422500
#define	IMX_EXA_MIN_PIXEL_AREA_COPY         14400
#define	IMX_EXA_MIN_PIXEL_AREA_COMPOSITE    150

    /************************************************************************
     * EXA COPY  (START)
     ************************************************************************/
    Bool
    VivPrepareCopy(PixmapPtr pSrcPixmap, PixmapPtr pDstPixmap,
            int xdir, int ydir, int alu, Pixel planemask);

    void
    VivCopy(PixmapPtr pDstPixmap, int srcX, int srcY, int dstX, int dstY, int width, int height);

    void
    VivDoneCopy(PixmapPtr pDstPixmap);

    /************************************************************************
     * EXA COPY (FINISH)
     ************************************************************************/

    /************************************************************************
     * EXA SOLID  (START)
     ************************************************************************/
    Bool
    VivPrepareSolid(PixmapPtr pPixmap, int alu, Pixel planemask, Pixel fg);

    void
    VivSolid(PixmapPtr pPixmap, int x1, int y1, int x2, int y2);

    void
    VivDoneSolid(PixmapPtr pPixmap);

    /************************************************************************
     * EXA SOLID (FINISH)
     ************************************************************************/


    /************************************************************************
     * EXA COMPOSITE  (START)
     ************************************************************************/
    Bool
    VivCheckComposite(int op, PicturePtr pSrcPicture, PicturePtr pMaskPicture, PicturePtr pDstPicture);
    Bool
    VivPrepareComposite(int op, PicturePtr pSrcPicture, PicturePtr pMaskPicture, PicturePtr pDstPicture,
            PixmapPtr pSrc, PixmapPtr pMask, PixmapPtr pDst);
    void
    VivComposite(PixmapPtr pDst, int srcX, int srcY, int maskX, int maskY,
            int dstX, int dstY, int width, int height);
    void
    VivDoneComposite(PixmapPtr pDst);

    /************************************************************************
     * EXA COMPOSITE (FINISH)
     ************************************************************************/

    /************************************************************************
     * EXA PIXMAP  (START)
     ************************************************************************/
    Bool
    VivPrepareAccess(PixmapPtr pPix, int index);

    void
    VivFinishAccess(PixmapPtr pPix, int index);

    void *
    VivCreatePixmap(ScreenPtr pScreen, int size, int align);

    void
    VivDestroyPixmap(ScreenPtr pScreen, void *dPriv);

    Bool
    VivPixmapIsOffscreen(PixmapPtr pPixmap);

    Bool
    VivModifyPixmapHeader(PixmapPtr pPixmap, int width, int height,
            int depth, int bitsPerPixel, int devKind,
            pointer pPixData);

    /************************************************************************
     * EXA PIXMAP (FINISH)
     ************************************************************************/

    /************************************************************************
     * EXA OTHER FUNCTIONS  (START)
     ************************************************************************/
    void
    VivEXASync(ScreenPtr pScreen, int marker);

    Bool
    VivUploadToScreen(PixmapPtr pDst, int x, int y, int w, int h, char *src, int src_pitch);

    Bool
    VivDownloadFromScreen(PixmapPtr pSrc, int x, int y, int w, int h, char *dst, int dst_pitch);
    /************************************************************************
     * EXA OTHER FUNCTIONS (END)
     ************************************************************************/

    /************************************************************************
     * UTILITY FUNCTIONS  (START)
     ************************************************************************/
    Bool CheckBltvalidity(PixmapPtr pPixmap, int alu, Pixel planemask);
    void ConvertXAluToOPS(PixmapPtr pPixmap, int alu, Pixel planemask, int *fg, int *bg);
    PixmapPtr GetDrawablePixmap(DrawablePtr pDrawable);
    /************************************************************************
     * UTILITY FUNCTIONS (END)
     ************************************************************************/

#ifdef __cplusplus
}
#endif

#endif	/* VIVANTE_EXA_H */

