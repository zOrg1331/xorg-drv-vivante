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



#include "vivante_common.h"
#include "vivante.h"
#include "vivante_dri.h"

#define VIV_PAGE_SHIFT      (12)
#define VIV_PAGE_SIZE       (1UL << VIV_PAGE_SHIFT)
#define VIV_PAGE_MASK       (~(VIV_PAGE_SIZE - 1))
#define VIV_PAGE_ALIGN(val) (((val) + (VIV_PAGE_SIZE -1)) & (VIV_PAGE_MASK))

static char VivKernelDriverName[] = "vivante";
static char VivClientDriverName[] = "vivante";

/* TODO: xserver receives driver's swapping event and does something
 *       according the data initialized in this function.
 */
static Bool
VivCreateContext(ScreenPtr pScreen, VisualPtr visual,
        drm_context_t hwContext, void *pVisualConfigPriv,
        DRIContextType contextStore) {
    return TRUE;
}

static void
VivDestroyContext(ScreenPtr pScreen, drm_context_t hwContext,
        DRIContextType contextStore) {
}

Bool
VivDRIFinishScreenInit(ScreenPtr pScreen) {
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    VivPtr pViv = GET_VIV_PTR(pScrn);
    DRIInfoPtr pDRIInfo = (DRIInfoPtr) pViv->pDRIInfo;

    pDRIInfo->driverSwapMethod = DRI_HIDE_X_CONTEXT;

    DRIFinishScreenInit(pScreen);

    return TRUE;
}

static void
VivDRISwapContext(ScreenPtr pScreen, DRISyncType syncType,
        DRIContextType oldContextType, void *oldContext,
        DRIContextType newContextType, void *newContext) {
    return;
}

static void
VivDRIInitBuffers(WindowPtr pWin, RegionPtr prgn, CARD32 index) {
    return;
}

static void
VivDRIMoveBuffers(WindowPtr pParent, DDXPointRec ptOldOrg,
        RegionPtr prgnSrc, CARD32 index) {
    return;
}

Bool VivDRIScreenInit(ScreenPtr pScreen) {
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    DRIInfoPtr pDRIInfo;
    VivPtr pViv = GET_VIV_PTR(pScrn);

    /* Check that the GLX, DRI, and DRM modules have been loaded by testing
     * for canonical symbols in each module. */
    if (!xf86LoaderCheckSymbol("GlxSetVisualConfigs")) return FALSE;
    if (!xf86LoaderCheckSymbol("DRIScreenInit")) return FALSE;
    if (!xf86LoaderCheckSymbol("DRIQueryVersion")) {
        xf86DrvMsg(pScreen->myNum, X_ERROR,
                "[dri] VivDRIScreenInit failed (libdri.a too old)\n");
        return FALSE;
    }

    /* Check the DRI version */
    {
        int major, minor, patch;
        DRIQueryVersion(&major, &minor, &patch);
        if (major < 4 || minor < 0) {
            xf86DrvMsg(pScreen->myNum, X_ERROR,
                    "[dri] VivDRIScreenInit failed because of a version mismatch.\n"
                    "[dri] libDRI version is %d.%d.%d but version 4.0.x is needed.\n"
                    "[dri] Disabling DRI.\n",
                    major, minor, patch);
            return FALSE;
        }
    }


    pDRIInfo = DRICreateInfoRec();

    if (!pDRIInfo) return FALSE;

    pViv->pDRIInfo = pDRIInfo;
    pDRIInfo->drmDriverName=VivKernelDriverName;
    pDRIInfo->clientDriverName=VivClientDriverName;
    pDRIInfo->busIdString =(char *)xalloc(64);
    /* use = to copy string and it seems good, but when you free it, it will report invalid pointer, use strcpy instead */
    //pDRIInfo->busIdString="platform:Vivante GCCore";
    strcpy(pDRIInfo->busIdString,"platform:Vivante GCCore");

    pDRIInfo->ddxDriverMajorVersion = VIV_DRI_VERSION_MAJOR;
    pDRIInfo->ddxDriverMinorVersion = VIV_DRI_VERSION_MINOR;
    pDRIInfo->ddxDriverPatchVersion = 0;
    pDRIInfo->frameBufferPhysicalAddress =(pointer)(pViv->mFB.memPhysBase + pViv->mFB.mFBOffset);
    pDRIInfo->frameBufferSize = pScrn->videoRam;

    pDRIInfo->frameBufferStride = (pScrn->displayWidth *
            pScrn->bitsPerPixel / 8);

    pDRIInfo->maxDrawableTableEntry = VIV_MAX_DRAWABLES;

    pDRIInfo->SAREASize = SAREA_MAX;

    pDRIInfo->devPrivate = NULL;
    pDRIInfo->devPrivateSize = 0;
    pDRIInfo->contextSize = 1024;

    pDRIInfo->CreateContext = VivCreateContext;
    pDRIInfo->DestroyContext = VivDestroyContext;
    pDRIInfo->SwapContext = VivDRISwapContext;
    pDRIInfo->InitBuffers = VivDRIInitBuffers;
    pDRIInfo->MoveBuffers = VivDRIMoveBuffers;
    pDRIInfo->bufferRequests = DRI_ALL_WINDOWS;

    /*
     ** drmAddMap required the base and size of target buffer to be page aligned.
     ** While frameBufferSize sometime doesn't, so we force it aligned here, and
     ** restore it back after DRIScreenInit
     */
    pDRIInfo->frameBufferSize = VIV_PAGE_ALIGN(pDRIInfo->frameBufferSize);
    if (!DRIScreenInit(pScreen, pDRIInfo, &pViv->drmSubFD)) {
        xf86DrvMsg(pScreen->myNum, X_ERROR,
                "[dri] DRIScreenInit failed.  Disabling DRI.\n");
        DRIDestroyInfoRec(pViv->pDRIInfo);
        pViv->pDRIInfo = NULL;
        return FALSE;
    }
    pDRIInfo->frameBufferSize = pScrn->videoRam;

    /* Check the Vivante DRM version */
    {
        drmVersionPtr version = drmGetVersion(pViv->drmSubFD);
        if (version) {
            if (version->version_major != 1 ||
                    version->version_minor < 0) {
                /* incompatible drm version */
                xf86DrvMsg(pScreen->myNum, X_ERROR,
                        "[dri] VIVDRIScreenInit failed because of a version mismatch.\n"
                        "[dri] vivante.o kernel module version is %d.%d.%d but version 1.0.x is needed.\n"
                        "[dri] Disabling the DRI.\n",
                        version->version_major,
                        version->version_minor,
                        version->version_patchlevel);
                VivDRICloseScreen(pScreen);
                drmFreeVersion(version);
                return FALSE;
            }
        }
        drmFreeVersion(version);
    }

    return TRUE;
}

void VivDRICloseScreen(ScreenPtr pScreen) {
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    VivPtr pViv = GET_VIV_PTR(pScrn);

    if (pViv->pDRIInfo) {
        DRICloseScreen(pScreen);
        DRIDestroyInfoRec(pViv->pDRIInfo);
        pViv->pDRIInfo = 0;
    }
}

