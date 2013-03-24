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
#include "vivante_exa.h"

/************************************************************************
 * MACROS FOR VERSIONING & INFORMATION (START)
 ************************************************************************/
#define VIVANTE_VERSION           1000
#define VIVANTE_VERSION_MAJOR      1
#define VIVANTE_VERSION_MINOR      0
#define VIVANTE_VERSION_PATCHLEVEL 0
#define VIVANTE_VERSION_STRING	"1.0"

#define VIVANTE_NAME              "VIVANTE"
#define VIVANTE_DRIVER_NAME       "vivante"
/************************************************************************
 * MACROS FOR VERSIONING & INFORMATION (END)
 ************************************************************************/

/************************************************************************
 * Core Functions To FrameBuffer Driver
 * 1) AvailableOptions
 * 2) Identify
 * 3) Probe
 * 4) PreInit
 * 5) ScreenInit
 * 6) CloseScreen
 * 7) DriverFunction(Opt)
 ************************************************************************/

static const OptionInfoRec *VivAvailableOptions(int chipid, int busid);
static void VivIdentify(int flags);
static Bool VivProbe(DriverPtr drv, int flags);
static Bool VivPreInit(ScrnInfoPtr pScrn, int flags);
static Bool VivScreenInit(int Index, ScreenPtr pScreen, int argc,
        char **argv);
static Bool VivCloseScreen(int scrnIndex, ScreenPtr pScreen);
static Bool VivDriverFunc(ScrnInfoPtr pScrn, xorgDriverFuncOp op,
        pointer ptr);

/************************************************************************
 * END OF  Core Functions To FrameBuffer Driver
 ************************************************************************/
static void *
VivWindowLinear(ScreenPtr pScreen, CARD32 row, CARD32 offset, int mode,
        CARD32 *size, void *closure);

/************************************************************************
 * SUPPORTED CHIPSETS (START)
 ************************************************************************/

typedef enum _chipSetID {
    GC500_ID = 0x33,
    GC2100_ID,
    GCCORE_ID
} CHIPSETID;

/*CHIP NAMES*/
#define GCCORE_STR "VivanteGCCORE"
#define GC500_STR  "VivanteGC500"
#define GC2100_STR "VivanteGC2100"

/************************************************************************
 * SUPPORTED CHIPSETS (END)
 ************************************************************************/
/*
 * This is intentionally screen-independent.  It indicates the binding
 * choice made in the first PreInit.
 */
static int pix24bpp = 0;


/************************************************************************
 * X Window System Registration (START)
 ************************************************************************/
_X_EXPORT DriverRec VIV = {
    VIVANTE_VERSION,
    VIVANTE_DRIVER_NAME,
    VivIdentify,
    VivProbe,
    VivAvailableOptions,
    NULL,
    0,
    VivDriverFunc,
};

/* Supported "chipsets" */
static SymTabRec VivChipsets[] = {
    {GC500_ID, GC500_STR},
    {GC2100_ID, GC2100_STR},
    {GCCORE_ID, GCCORE_STR},
    {-1, NULL}
};


static const OptionInfoRec VivOptions[] = {
    { OPTION_VIV, "vivante", OPTV_STRING,
        {0}, FALSE},
    { OPTION_NOACCEL, "NoAccel", OPTV_BOOLEAN,
        {0}, FALSE},
    { OPTION_ACCELMETHOD, "AccelMethod", OPTV_STRING,
        {0}, FALSE},
    { -1, NULL, OPTV_NONE,
        {0}, FALSE}
};

/* -------------------------------------------------------------------- */

#ifdef XFree86LOADER

MODULESETUPPROTO(VivSetup);

static XF86ModuleVersionInfo VivVersRec = {
    VIVANTE_DRIVER_NAME,
    MODULEVENDORSTRING,
    MODINFOSTRING1,
    MODINFOSTRING2,
    XORG_VERSION_CURRENT,
    VIVANTE_VERSION_MAJOR,
    VIVANTE_VERSION_MINOR,
    VIVANTE_VERSION_PATCHLEVEL,
    ABI_CLASS_VIDEODRV,
    ABI_VIDEODRV_VERSION,
    NULL,
    {0, 0, 0, 0}
};

_X_EXPORT XF86ModuleData vivanteModuleData = {&VivVersRec, VivSetup, NULL};

pointer
VivSetup(pointer module, pointer opts, int *errmaj, int *errmin) {
    TRACE_ENTER();
    pointer ret;
    static Bool setupDone = FALSE;

    if (!setupDone) {
        setupDone = TRUE;
        xf86AddDriver(&VIV, module, HaveDriverFuncs);
        ret = (pointer) 1;

    } else {
        if (errmaj) *errmaj = LDR_ONCEONLY;
        ret = (pointer) 0;

    }
    TRACE_EXIT(ret);
}

#endif /* XFree86LOADER */

/************************************************************************
 * X Window System Registration (START)
 ************************************************************************/

#define UPLOAD_FUNC_ENABLED 1

static Bool InitExaLayer(ScreenPtr pScreen) {
    ExaDriverPtr pExa;
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    VivPtr pViv = GET_VIV_PTR(pScrn);

    TRACE_ENTER();

    xf86DrvMsg(pScreen->myNum, X_INFO, "test Initializing EXA\n");

    /*Initing EXA*/
    pExa = exaDriverAlloc();
    if (!pExa) {
        TRACE_ERROR("Unable to allocate exa driver");
        pViv->mFakeExa.mNoAccelFlag = TRUE;
        TRACE_EXIT(FALSE);
    }

    pViv->mFakeExa.mExaDriver = pExa;
    /*Exa version*/
    pExa->exa_major = EXA_VERSION_MAJOR;
    pExa->exa_minor = EXA_VERSION_MINOR;

    /* 12 bit coordinates */
    pExa->maxX = VIV_MAX_WIDTH;
    pExa->maxY = VIV_MAX_HEIGHT;

    /*Memory Manager*/
    pExa->memoryBase = pViv->mFB.mFBStart; /*logical*/
    pExa->memorySize = pScrn->videoRam;
    pExa->offScreenBase = pScrn->virtualY * pScrn->displayWidth * (pScrn->bitsPerPixel >> 3);

#if USE_GPU_FB_MEM_MAP
    if (!VIV2DGPUUserMemMap((char*) pExa->memoryBase, pScrn->memPhysBase, pExa->memorySize, &pViv->mFB.mMappingInfo, &pViv->mFB.memPhysBase)) {
        TRACE_ERROR("ERROR ON MAPPING FB\n");
        TRACE_EXIT(FALSE);
    }
#endif


    /*flags*/
    pExa->flags = EXA_HANDLES_PIXMAPS | EXA_SUPPORTS_PREPARE_AUX | EXA_OFFSCREEN_PIXMAPS;

    /*Subject to change*/
    pExa->pixmapOffsetAlign = 8;
    /*This is for sure*/
    pExa->pixmapPitchAlign = PIXMAP_PITCH_ALIGN;

    pExa->WaitMarker = VivEXASync;

#ifndef DISABLE_SOLID
    pExa->PrepareSolid = VivPrepareSolid;
    pExa->Solid = VivSolid;
    pExa->DoneSolid = VivDoneSolid;
#endif

#ifndef DISABLE_COPY
    pExa->PrepareCopy = VivPrepareCopy;
    pExa->Copy = VivCopy;
    pExa->DoneCopy = VivDoneCopy;
#endif

#if UPLOAD_FUNC_ENABLED
    pExa->UploadToScreen = VivUploadToScreen;
#endif



#ifndef DISABLE_COMPOSITE
    pExa->CheckComposite = VivCheckComposite;
    pExa->PrepareComposite = VivPrepareComposite;
    pExa->Composite = VivComposite;
    pExa->DoneComposite = VivDoneComposite;
#endif

    pExa->CreatePixmap = VivCreatePixmap;
    pExa->DestroyPixmap = VivDestroyPixmap;
    pExa->ModifyPixmapHeader = VivModifyPixmapHeader;
    pExa->PixmapIsOffscreen = VivPixmapIsOffscreen;
    pExa->PrepareAccess = VivPrepareAccess;
    pExa->FinishAccess = VivFinishAccess;

    if (!exaDriverInit(pScreen, pExa)) {
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                "exaDriverinit failed.\n");
        TRACE_EXIT(FALSE);
    }

    if (!VIV2DGPUCtxInit(&pViv->mGrCtx)) {
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                "internal error: GPU Ctx Init Failed\n");
        TRACE_EXIT(FALSE);
    }

    pViv->mFakeExa.mIsInited = TRUE;
    TRACE_EXIT(TRUE);
}

static Bool DestroyExaLayer(ScreenPtr pScreen) {
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    VivPtr pViv = GET_VIV_PTR(pScrn);
    TRACE_ENTER();
    xf86DrvMsg(pScreen->myNum, X_INFO, "Shutdown EXA\n");
#if USE_GPU_FB_MEM_MAP
    ExaDriverPtr pExa = pViv->mFakeExa.mExaDriver;
    if (!VIV2DGPUUserMemUnMap((char*) pExa->memoryBase, pExa->memorySize, pViv->mFB.mMappingInfo, pViv->mFB.memPhysBase)) {
        TRACE_ERROR("Unmapping User memory Failed\n");
    }
#endif

    exaDriverFini(pScreen);

    if (!VIV2DGPUCtxDeInit(&pViv->mGrCtx)) {
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                "internal error: GPU Ctx DeInit Failed\n");
        TRACE_EXIT(FALSE);
    }
    TRACE_EXIT(TRUE);
}

/************************************************************************
 *  Const/Dest Functions (START)
 ************************************************************************/
static Bool
VivGetRec(ScrnInfoPtr pScrn) {
    if (pScrn->driverPrivate != NULL) {
        TRACE_EXIT(TRUE);
    }
    pScrn->driverPrivate = malloc(sizeof (VivRec));
    TRACE_EXIT(TRUE);
}

static void
VivFreeRec(ScrnInfoPtr pScrn) {
    if (pScrn->driverPrivate == NULL) {
        TRACE_EXIT();
    }
    free(pScrn->driverPrivate);
    pScrn->driverPrivate = NULL;
    TRACE_EXIT();
}

/************************************************************************
 * R Const/Dest Functions (END)
 ************************************************************************/

/************************************************************************
 * START OF THE IMPLEMENTATION FOR CORE FUNCTIONS
 ************************************************************************/

static const OptionInfoRec *
VivAvailableOptions(int chipid, int busid) {
    /*Chip id may also be used for special cases*/
    TRACE_ENTER();
    TRACE_EXIT(VivOptions);
}

static void
VivIdentify(int flags) {
    TRACE_ENTER();
    xf86PrintChipsets(VIVANTE_NAME, "fb driver for vivante", VivChipsets);
    TRACE_EXIT();
}

static Bool
VivProbe(DriverPtr drv, int flags) {
    int i;
    ScrnInfoPtr pScrn;
    GDevPtr *devSections;
    int numDevSections;
    char *dev;
    int entity;
    Bool foundScreen = FALSE;

    TRACE_ENTER();

    /* For now, just bail out for PROBE_DETECT. */
    if (flags & PROBE_DETECT) {
        /*Look into PROBE_DETECT*/
        TRACE_EXIT(FALSE);
    }

    /*
     * int xf86MatchDevice(char * drivername, GDevPtr ** sectlist)
     * with its driver name. The function allocates an array of GDevPtr and
     * returns this via sectlist and returns the number of elements in
     * this list as return value. 0 means none found, -1 means fatal error.
     *
     * It can figure out which of the Device sections to use for which card
     * (using things like the Card statement, etc). For single headed servers
     * there will of course be just one such Device section.
     */
    numDevSections = xf86MatchDevice(VIVANTE_DRIVER_NAME, &devSections);
    if (numDevSections <= 0) {
        TRACE_ERROR("No matching device\n");
        TRACE_EXIT(FALSE);
    }

    if (!xf86LoadDrvSubModule(drv, "fbdevhw")) {
        TRACE_ERROR("Unable to load fbdev module\n");
        TRACE_EXIT(FALSE);
    }

    for (i = 0; i < numDevSections; i++) {
        dev = xf86FindOptionValue(devSections[i]->options, "vivante");
        if (fbdevHWProbe(NULL, dev, NULL)) {
            pScrn = NULL;
            entity = xf86ClaimFbSlot(drv, 0,
                    devSections[i], TRUE);
            pScrn = xf86ConfigFbEntity(NULL, 0, entity,
                    NULL, NULL, NULL, NULL);
            if (pScrn) {

                foundScreen = TRUE;

                pScrn->driverVersion = VIVANTE_VERSION;
                pScrn->driverName = VIVANTE_DRIVER_NAME;
                pScrn->name = VIVANTE_NAME;
                pScrn->Probe = VivProbe;
                pScrn->PreInit = VivPreInit;
                pScrn->ScreenInit = VivScreenInit;
                pScrn->SwitchMode = fbdevHWSwitchModeWeak();
                pScrn->AdjustFrame = fbdevHWAdjustFrameWeak();
                pScrn->EnterVT = fbdevHWEnterVTWeak();
                pScrn->LeaveVT = fbdevHWLeaveVTWeak();
                pScrn->ValidMode = fbdevHWValidModeWeak();

                xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                        "using %s\n", dev ? dev : "default device");
            }
        }
    }
    free(devSections);
    TRACE_EXIT(foundScreen);
}

static Bool
VivPreInit(ScrnInfoPtr pScrn, int flags) {
    VivPtr fPtr;
    int default_depth, fbbpp;
    const char *s;
    int type;
    char *dev_node;

    TRACE_ENTER();

    if (flags & PROBE_DETECT) {
        TRACE_EXIT(FALSE);
    }

    /* Check the number of entities, and fail if it isn't one. */
    if (pScrn->numEntities != 1) {
        TRACE_ERROR("Not Just One Entry");
        TRACE_EXIT(FALSE);
    }

    /*Setting the monitor*/
    pScrn->monitor = pScrn->confScreen->monitor;
    /*Allocating the rectangle structure*/
    VivGetRec(pScrn);
    /*Getting a pointer to Rectangle Structure*/
    fPtr = GET_VIV_PTR(pScrn);

    /*Fetching the entity*/
    fPtr->mEntity = xf86GetEntityInfo(pScrn->entityList[0]);

    /*Getting the device path*/
    dev_node = xf86FindOptionValue(fPtr->mEntity->device->options, "vivante_fbdev");
    if (!dev_node) {
        dev_node = "/dev/fb2";
    }

    /* open device */
    if (!fbdevHWInit(pScrn, NULL, dev_node)) {
        TRACE_ERROR("CAN'T OPEN THE FRAMEBUFFER DEVICE");
        TRACE_EXIT(FALSE);
    }

    /*Get the default depth*/
    default_depth = fbdevHWGetDepth(pScrn, &fbbpp);
    /*Setting the depth Info*/
    if (!xf86SetDepthBpp(pScrn, default_depth, default_depth, fbbpp,
            Support24bppFb | Support32bppFb)) {
        TRACE_ERROR("DEPTH IS NOT SUPPORTED");
        TRACE_EXIT(FALSE);
    }

    /*Printing as info*/
    xf86PrintDepthBpp(pScrn);

    /* Get the depth24 pixmap format */
    if (pScrn->depth == 24 && pix24bpp == 0)
        pix24bpp = xf86GetBppFromDepth(pScrn, 24);

    /* color weight */
    if (pScrn->depth > 8) {
        rgb zeros = {0, 0, 0};
        if (!xf86SetWeight(pScrn, zeros, zeros)) {
            TRACE_ERROR("Color weight ");
            TRACE_EXIT(FALSE);
        }
    }

    /* visual init */
    if (!xf86SetDefaultVisual(pScrn, -1)) {
        TRACE_ERROR("Visual Init Problem");
        TRACE_EXIT(FALSE);
    }

    /* We don't currently support DirectColor at > 8bpp */
    if (pScrn->depth > 8 && pScrn->defaultVisual != TrueColor) {
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "requested default visual"
                " (%s) is not supported at depth %d\n",
                xf86GetVisualName(pScrn->defaultVisual), pScrn->depth);
        TRACE_EXIT(FALSE);
    }

    {
        Gamma zeros = {0.0, 0.0, 0.0};

        if (!xf86SetGamma(pScrn, zeros)) {
            TRACE_ERROR("Unable to Set Gamma\n");
            TRACE_EXIT(FALSE);
        }
    }

    pScrn->progClock = TRUE; /*clock is programmable*/
    if (pScrn->depth == 8) {
        pScrn->rgbBits = 8; /* 8 bits in r/g/b */
    }
    pScrn->chipset = "vivante";
    pScrn->videoRam = fbdevHWGetVidmem(pScrn);

    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "hardware: %s (video memory:"
            " %dkB)\n", fbdevHWGetName(pScrn), pScrn->videoRam / 1024);

    /* handle options */
    xf86CollectOptions(pScrn, NULL);
    if (!(fPtr->mSupportedOptions = malloc(sizeof (VivOptions)))) {
        TRACE_ERROR("Unable to allocate the supported Options\n");
        TRACE_EXIT(FALSE);
    }

    memcpy(fPtr->mSupportedOptions, VivOptions, sizeof (VivOptions));
    xf86ProcessOptions(pScrn->scrnIndex, fPtr->mEntity->device->options, fPtr->mSupportedOptions);


    fPtr->mFakeExa.mNoAccelFlag = xf86ReturnOptValBool(fPtr->mSupportedOptions, OPTION_NOACCEL, FALSE);

    if (fPtr->mFakeExa.mNoAccelFlag) {
        xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Acceleration disabled\n");
    } else {
        char *s = xf86GetOptValString(fPtr->mSupportedOptions, OPTION_ACCELMETHOD);
        fPtr->mFakeExa.mNoAccelFlag = FALSE;
        fPtr->mFakeExa.mUseExaFlag = TRUE;
        if (s && xf86NameCmp(s, "EXA")) {
            fPtr->mFakeExa.mUseExaFlag = FALSE;
            fPtr->mFakeExa.mNoAccelFlag = TRUE;
        }
    }

    /* select video modes */

    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "checking modes against framebuffer device...\n");
    fbdevHWSetVideoModes(pScrn);

    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "checking modes against monitor...\n");
    {
        DisplayModePtr mode, first = mode = pScrn->modes;

        if (mode != NULL) do {
                mode->status = xf86CheckModeForMonitor(mode, pScrn->monitor);
                mode = mode->next;
            } while (mode != NULL && mode != first);

        xf86PruneDriverModes(pScrn);
    }

    if (NULL == pScrn->modes) {
        fbdevHWUseBuildinMode(pScrn);
    }
    /*setting current mode*/
    pScrn->currentMode = pScrn->modes;

    /* First approximation, may be refined in ScreenInit */
    pScrn->displayWidth = pScrn->virtualX;

    xf86PrintModes(pScrn);

    /* Set display resolution */
    xf86SetDpi(pScrn, 0, 0);

    if (xf86LoadSubModule(pScrn, "fb") == NULL) {
        VivFreeRec(pScrn);
        TRACE_ERROR("Unable to load fb submodule\n");
        TRACE_EXIT(FALSE);
    }


    /* Load EXA acceleration if needed */
    if (fPtr->mFakeExa.mUseExaFlag) {
        if (!xf86LoadSubModule(pScrn, "exa")) {
            VivFreeRec(pScrn);
            xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Error on loading exa submodule\n");
            TRACE_EXIT(FALSE);
        }
    }


    TRACE_EXIT(TRUE);
}

static Bool
VivCreateScreenResources(ScreenPtr pScreen) {
    PixmapPtr pPixmap;
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    VivPtr fPtr = GET_VIV_PTR(pScrn);
    Bool ret;

    TRACE_ENTER();

    /*Setting up*/
    pScreen->CreateScreenResources = fPtr->CreateScreenResources;
    ret = pScreen->CreateScreenResources(pScreen);
    pScreen->CreateScreenResources = VivCreateScreenResources;

    if (!ret) {
        TRACE_ERROR("Unable to create screen resources\n");
        TRACE_EXIT(FALSE);
    }

    pPixmap = pScreen->GetScreenPixmap(pScreen);

    TRACE_EXIT(TRUE);
}

static Bool
VivScreenInit(int scrnIndex, ScreenPtr pScreen, int argc, char **argv) {
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    VivPtr fPtr = GET_VIV_PTR(pScrn);
    VisualPtr visual;
    int init_picture = 0;
    int ret, flags;

    TRACE_ENTER();

    DEBUGP("\tbitsPerPixel=%d, depth=%d, defaultVisual=%s\n"
            "\tmask: %lu,%lu,%lu, offset: %lu,%lu,%lu\n",
            pScrn->bitsPerPixel,
            pScrn->depth,
            xf86GetVisualName(pScrn->defaultVisual),
            pScrn->mask.red, pScrn->mask.green, pScrn->mask.blue,
            pScrn->offset.red, pScrn->offset.green, pScrn->offset.blue);

    /*Mapping the Video memory*/
    if (NULL == (fPtr->mFB.mFBMemory = fbdevHWMapVidmem(pScrn))) {
        xf86DrvMsg(scrnIndex, X_ERROR, "mapping of video memory"
                " failed\n");
        TRACE_EXIT(FALSE);
    }
    /*Getting the  linear offset*/
    fPtr->mFB.mFBOffset = fbdevHWLinearOffset(pScrn);

    /*Setting the physcal addr*/
    fPtr->mFB.memPhysBase = pScrn->memPhysBase;


    /*Save Configuration*/
    fbdevHWSave(pScrn);

    /*Init the hardware in current mode*/
    if (!fbdevHWModeInit(pScrn, pScrn->currentMode)) {
        xf86DrvMsg(scrnIndex, X_ERROR, "mode initialization failed\n");
        TRACE_EXIT(FALSE);
    }
    fbdevHWSaveScreen(pScreen, SCREEN_SAVER_ON);
    fbdevHWAdjustFrame(scrnIndex, 0, 0, 0);



    /* mi layer */
    miClearVisualTypes();
    if (pScrn->bitsPerPixel > 8) {
        if (!miSetVisualTypes(pScrn->depth, TrueColorMask, pScrn->rgbBits, TrueColor)) {
            xf86DrvMsg(scrnIndex, X_ERROR, "visual type setup failed"
                    " for %d bits per pixel [1]\n",
                    pScrn->bitsPerPixel);
            TRACE_EXIT(FALSE);
        }
    } else {
        if (!miSetVisualTypes(pScrn->depth,
                miGetDefaultVisualMask(pScrn->depth),
                pScrn->rgbBits, pScrn->defaultVisual)) {
            xf86DrvMsg(scrnIndex, X_ERROR, "visual type setup failed"
                    " for %d bits per pixel [2]\n",
                    pScrn->bitsPerPixel);
            TRACE_EXIT(FALSE);
        }
    }
    if (!miSetPixmapDepths()) {
        xf86DrvMsg(scrnIndex, X_ERROR, "pixmap depth setup failed\n");
        return FALSE;
    }


    /*Pitch*/
    pScrn->displayWidth = fbdevHWGetLineLength(pScrn) /
            (pScrn->bitsPerPixel / 8);
    if (pScrn->displayWidth != pScrn->virtualX) {
        xf86DrvMsg(scrnIndex, X_INFO,
                "Pitch updated to %d after ModeInit\n",
                pScrn->displayWidth);
    }
    /*Logical start address*/
    fPtr->mFB.mFBStart = fPtr->mFB.mFBMemory + fPtr->mFB.mFBOffset;

    xf86DrvMsg(scrnIndex, X_INFO,
            "FB Start = %p  FB Base = %p  FB Offset = %p\n",
            fPtr->mFB.mFBStart, fPtr->mFB.mFBMemory, fPtr->mFB.mFBOffset);

    switch (pScrn->bitsPerPixel) {
        case 8:
        case 16:
        case 24:
        case 32:
            ret = fbScreenInit(pScreen, fPtr->mFB.mFBStart, pScrn->virtualX,
                    pScrn->virtualY, pScrn->xDpi,
                    pScrn->yDpi, pScrn->displayWidth,
                    pScrn->bitsPerPixel);

            init_picture = 1;
            break;
        default:
            xf86DrvMsg(scrnIndex, X_ERROR,
                    "internal error: invalid number of bits per"
                    " pixel (%d) encountered in"
                    " VivScreenInit()\n", pScrn->bitsPerPixel);
            ret = FALSE;
            break;
    }
    /* Fixup RGB ordering */
    if (pScrn->bitsPerPixel > 8) {
        visual = pScreen->visuals + pScreen->numVisuals;
        while (--visual >= pScreen->visuals) {
            if ((visual->class | DynamicClass) == DirectColor) {
                visual->offsetRed = pScrn->offset.red;
                visual->offsetGreen = pScrn->offset.green;
                visual->offsetBlue = pScrn->offset.blue;
                visual->redMask = pScrn->mask.red;
                visual->greenMask = pScrn->mask.green;
                visual->blueMask = pScrn->mask.blue;
            }
        }
    }

    /* must be after RGB ordering fixed */
    if (init_picture && !fbPictureInit(pScreen, NULL, 0)) {
        xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
                "Render extension initialisation failed\n");
    }

    fPtr->mFakeExa.mIsInited = FALSE;
    if (fPtr->mFakeExa.mUseExaFlag) {
        TRACE_INFO("Loading EXA");
        if (!InitExaLayer(pScreen)) {
            xf86DrvMsg(scrnIndex, X_ERROR,
                    "internal error: initExaLayer failed "
                    "in VivScreenInit()\n");
        }
    }


    xf86SetBlackWhitePixels(pScreen);
    miInitializeBackingStore(pScreen);
    xf86SetBackingStore(pScreen);

    pScrn->vtSema = TRUE;

    /* software cursor */
    miDCInitialize(pScreen, xf86GetPointerScreenFuncs());


    /* colormap */
    if (!miCreateDefColormap(pScreen)) {
        xf86DrvMsg(scrnIndex, X_ERROR,
                "internal error: miCreateDefColormap failed "
                "in VivScreenInit()\n");
        TRACE_EXIT(FALSE);
    }

    flags = CMAP_PALETTED_TRUECOLOR;
    if (!xf86HandleColormaps(pScreen, 256, 8, fbdevHWLoadPaletteWeak(),
            NULL, flags)) {
        TRACE_EXIT(FALSE);
    }

    xf86DPMSInit(pScreen, fbdevHWDPMSSetWeak(), 0);

    pScreen->SaveScreen = fbdevHWSaveScreenWeak();

    /* Wrap the current CloseScreen function */
    fPtr->CloseScreen = pScreen->CloseScreen;
    pScreen->CloseScreen = VivCloseScreen;

    {
        XF86VideoAdaptorPtr *ptr;
        TRACE_INFO("Getting Adaptor");
        int n = xf86XVListGenericAdaptors(pScrn, &ptr);
        TRACE_INFO("Generic adaptor list = %d\n", n);
        if (n) {
            TRACE_INFO("BEFORE : pScreen= %p, ptr = %p n= %d\n", pScreen, ptr, n);
            xf86XVScreenInit(pScreen, ptr, n);
            TRACE_INFO("AFTER: pScreen= %p, ptr = %p n= %d\n", pScreen, ptr, n);
        }
    }

    if (VivDRIScreenInit(pScreen)) {
        VivDRIFinishScreenInit(pScreen);
    }

    TRACE_EXIT(TRUE);
}

static Bool
VivCloseScreen(int scrnIndex, ScreenPtr pScreen) {
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    VivPtr fPtr = GET_VIV_PTR(pScrn);
    Bool ret = FALSE;
    TRACE_ENTER();

    VivDRICloseScreen(pScreen);

    if (fPtr->mFakeExa.mUseExaFlag) {
        DEBUGP("UnLoading EXA");
        if (fPtr->mFakeExa.mIsInited && !DestroyExaLayer(pScreen)) {
            xf86DrvMsg(scrnIndex, X_ERROR,
                    "internal error: DestroyExaLayer failed "
                    "in VivCloseScreen()\n");
        }
    }

    fbdevHWRestore(pScrn);
    fbdevHWUnmapVidmem(pScrn);

    pScrn->vtSema = FALSE;

    pScreen->CreateScreenResources = fPtr->CreateScreenResources;
    pScreen->CloseScreen = fPtr->CloseScreen;
    ret = (*pScreen->CloseScreen)(scrnIndex, pScreen);
    TRACE_EXIT(ret);
}

static Bool VivDriverFunc(ScrnInfoPtr pScrn, xorgDriverFuncOp op,
        pointer ptr) {
    TRACE_ENTER();
    xorgHWFlags *flag;

    switch (op) {
        case GET_REQUIRED_HW_INTERFACES:
            flag = (CARD32*) ptr;
            (*flag) = 0;
            TRACE_EXIT(TRUE);
        default:
            TRACE_EXIT(FALSE);
    }
}
