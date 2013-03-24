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
#include "vivante_gal.h"
#include "vivante_priv.h"

/* gcvSURF_CACHEABLE_BITMAP OR gcvSURF_BITMAP */
#define SURFACE_TYPE gcvSURF_CACHEABLE_BITMAP
/* Cacheable = TRUE otherwise FALSE*/
#define SURFACE_CACHEABLE TRUE

/**
 *
 * @param Hal - Hardware abstraction layer object
 * @param Size - Size of the surface in bits
 * @param Pool - To allocate from which pool
 * @param Node - returned allocated video node
 * @return  - result of the process
 */
static gceSTATUS AllocVideoNode(
        IN gcoHAL Hal,
        IN OUT gctUINT_PTR Size,
        IN OUT gcePOOL *Pool,
        IN gceSURF_TYPE surftype,
        OUT gcuVIDMEM_NODE_PTR *Node) {
    gcsHAL_INTERFACE iface;
    gceSTATUS status;

    gcmASSERT(Pool != gcvNULL);
    gcmASSERT(Size != gcvNULL);
    gcmASSERT(Node != gcvNULL);

    iface.command = gcvHAL_ALLOCATE_LINEAR_VIDEO_MEMORY;
    iface.u.AllocateLinearVideoMemory.bytes = *Size;
    iface.u.AllocateLinearVideoMemory.alignment = 64;
    iface.u.AllocateLinearVideoMemory.pool = *Pool;
    iface.u.AllocateLinearVideoMemory.type = surftype;

    /* Call kernel API. */
    gcmONERROR(gcoHAL_Call(Hal, &iface));

    /* Get allocated node in video memory. */
    *Node = iface.u.AllocateLinearVideoMemory.node;
    *Pool = iface.u.AllocateLinearVideoMemory.pool;
    *Size = iface.u.AllocateLinearVideoMemory.bytes;

OnError:

    return status;
}

/**
 *
 * @param Hal - Hardware abstraction layer object
 * @param Node - video node
 * @return result of the process
 */
static gceSTATUS FreeVideoNode(
        IN gcoHAL Hal,
        IN gcuVIDMEM_NODE_PTR Node) {
    gcsHAL_INTERFACE iface;

    gcmASSERT(Node != gcvNULL);

    iface.command = gcvHAL_FREE_VIDEO_MEMORY;
    iface.u.FreeVideoMemory.node = Node;

    /* Call kernel API. */
    return gcoHAL_ScheduleEvent(Hal, &iface);
}

/**
 *  Use for  getting the physical and logical address
 * @param Hal Hardware abstraction layer object
 * @param Node video node
 * @param Address physical address
 * @param Memory logical address
 * @return result of the process
 */
static gceSTATUS LockVideoNode(
        IN gcoHAL Hal,
        IN gcuVIDMEM_NODE_PTR Node,
        IN Bool cacheable,
        OUT gctUINT32 *Address,
        OUT gctPOINTER *Memory) {
    gceSTATUS status;
    gcsHAL_INTERFACE iface;

    gcmASSERT(Address != gcvNULL);
    gcmASSERT(Memory != gcvNULL);
    gcmASSERT(Node != gcvNULL);

    iface.command = gcvHAL_LOCK_VIDEO_MEMORY;
    iface.u.LockVideoMemory.node = Node;
    iface.u.LockVideoMemory.cacheable = cacheable;
    /* Call kernel API. */
    gcmONERROR(gcoHAL_Call(Hal, &iface));

    /* Get allocated node in video memory. */
    *Address = iface.u.LockVideoMemory.address;
    *Memory = iface.u.LockVideoMemory.memory;

OnError:

    return status;
}

/**
 *
 * @param Hal
 * @param Node
 * @return
 */
static gceSTATUS UnlockVideoNode(
	IN gcoHAL Hal,
	IN gcuVIDMEM_NODE_PTR Node,
	IN gceSURF_TYPE surftype) {

	gcsHAL_INTERFACE iface;
	gceSTATUS status;

	gcmASSERT(Node != gcvNULL);

	iface.command = gcvHAL_UNLOCK_VIDEO_MEMORY;
	iface.u.UnlockVideoMemory.node = Node;
	iface.u.UnlockVideoMemory.type = surftype;
	iface.u.UnlockVideoMemory.asynchroneous = gcvTRUE;

	/* Call the kernel. */
	gcmONERROR(gcoOS_DeviceControl(
				gcvNULL,
				IOCTL_GCHAL_INTERFACE,
				&iface, gcmSIZEOF(iface),
				&iface, gcmSIZEOF(iface)
	));

	/* Success? */
	gcmONERROR(iface.status);

	/* Do we need to schedule an event for the unlock? */
	if (iface.u.UnlockVideoMemory.asynchroneous)
	{
		iface.u.UnlockVideoMemory.asynchroneous = gcvFALSE;
		gcmONERROR(gcoHAL_ScheduleEvent(Hal, &iface));
	}

OnError:
    /* Call kernel API. */
    return gcvSTATUS_FALSE;

}

#define AL_WIDTH IMX_EXA_NONCACHESURF_WIDTH
#define AL_HEIGHT IMX_EXA_NONCACHESURF_HEIGHT
#define AL_SWIDTH 500
#define AL_SHEIGHT 500
typedef struct _gsurfpool {
struct _gsurfpool *pnext;
struct _gsurfpool *prev;
GenericSurfacePtr surface;
} GSURFPOOL,*PGSURFPOOL;

typedef struct _gpoolhead {
gctUINT num;
PGSURFPOOL pfirst;
PGSURFPOOL plast;
}GPOOLHEAD;

static GPOOLHEAD __gsmallpoolhead = {0, NULL, NULL};
static GPOOLHEAD __gmidpoolhead = {0, NULL, NULL};
static GPOOLHEAD __gbigpoolhead = {0, NULL, NULL};

static GPOOLHEAD *__gpoolhead = &__gsmallpoolhead;

//#define TEST_POOL 1

#define MAX_BNODE 3
#define MAX_MNODE 3
#define MAX_SNODE 3


static gctUINT MAX_NODE = MAX_SNODE;

#define SETPOOL(alwidth,alheight,bytesPerPixel) do {	\
											if (( alwidth * alheight * bytesPerPixel ) >= ( AL_WIDTH * AL_HEIGHT *bytesPerPixel))	\
											{							\
												__gpoolhead = &__gbigpoolhead;	\
												MAX_NODE = MAX_BNODE;	\
												break;							\
											}								\
											if ( ( alwidth * alheight * bytesPerPixel ) <= ( AL_SWIDTH * AL_SHEIGHT * bytesPerPixel ) ) \
											{	\
												__gpoolhead = &__gsmallpoolhead; \
												MAX_NODE = MAX_SNODE;	\
												break ;						\
											}	\
											__gpoolhead = &__gmidpoolhead; \
											MAX_NODE = MAX_MNODE; \
										} while( 0 )


static Bool SurfaceInPool(GenericSurfacePtr psurface)
{
	PGSURFPOOL poolnode = NULL;

	if (psurface == NULL )
		TRACE_EXIT(gcvFALSE);

	SETPOOL((psurface->mAlignedWidth), (psurface->mAlignedHeight), (psurface->mBytesPerPixel));
	if ( __gpoolhead->pfirst == NULL)	TRACE_EXIT(gcvFALSE);

	poolnode = __gpoolhead->pfirst;

	while ( poolnode )
	{

		if ( poolnode->surface == psurface )
			TRACE_EXIT(gcvTRUE);

		poolnode = poolnode->pnext;

	}


	TRACE_EXIT(gcvFALSE);
}

/* only for debug */
static Bool TestSurfPool()
{

	gctUINT num = 0;
	PGSURFPOOL poolnode = NULL;

	if ( __gpoolhead->pfirst == NULL )
	{
		if ( __gpoolhead->num == 0)
			TRACE_EXIT(gcvTRUE);

		TRACE_EXIT(gcvFALSE);
	}

	poolnode = __gpoolhead->pfirst;

	while ( poolnode ) {
		num++;
		poolnode = poolnode->pnext;
	}

	if ( num != __gpoolhead->num)
		TRACE_EXIT(gcvFALSE);

	TRACE_EXIT(gcvTRUE);

}

/* When surface will be destroyed, call this to add it into pool */
/* Return non-null, which means the user has to destroy the ret surf */
/* Return null, which means the user has not to do the next destroy procedure */
static GenericSurfacePtr AddGSurfIntoPool(GenericSurfacePtr psurface)
{
	PGSURFPOOL poolnode = NULL;
	PGSURFPOOL pnextnode = NULL;
	GenericSurfacePtr pretsurf = NULL;

	if ( psurface == NULL) return NULL;


	SETPOOL((psurface->mAlignedWidth), (psurface->mAlignedHeight), (psurface->mBytesPerPixel));

	gcmASSERT(__gpoolhead->num <= MAX_NODE);
	gcmASSERT( 3 <= MAX_NODE);

	if ( __gpoolhead->num == MAX_NODE )
	{
		if ( __gpoolhead->plast->surface->mVideoNode.mSizeInBytes >= psurface->mVideoNode.mSizeInBytes )
			return psurface;

		poolnode = __gpoolhead->plast;

		pretsurf = poolnode->surface;

		__gpoolhead->plast->prev->pnext = NULL;
		__gpoolhead->plast = __gpoolhead->plast->prev;
		__gpoolhead->num--;

		free(poolnode);
	}

	poolnode = (PGSURFPOOL)malloc(sizeof(GSURFPOOL));
	poolnode->surface = psurface;
	poolnode->pnext = NULL;
	poolnode->prev= NULL;

	if ( __gpoolhead->pfirst == NULL) {

		poolnode->prev = poolnode;
		__gpoolhead->pfirst = poolnode;
		__gpoolhead->plast = poolnode;
		__gpoolhead->num = 1;
		return NULL;
	}

	/* if pfirst is not NULL, plast must be non-null */
	pnextnode = __gpoolhead->pfirst;

	while ( pnextnode )
	{
		if ( pnextnode->surface->mVideoNode.mSizeInBytes > psurface->mVideoNode.mSizeInBytes )
		{
			pnextnode = pnextnode->pnext;
			continue;
		}
		break;
	}

	if ( pnextnode == NULL )
	{
		poolnode->prev= __gpoolhead->plast;
		__gpoolhead->plast->pnext = poolnode;
		__gpoolhead->plast =poolnode;
	} else {
		if ( pnextnode == __gpoolhead->pfirst )
		{
			poolnode->pnext = pnextnode;
			pnextnode->prev = poolnode;
			__gpoolhead->pfirst = poolnode;
			poolnode->prev = __gpoolhead->pfirst;
		} else {
			poolnode->pnext = pnextnode;
			poolnode->prev = pnextnode->prev;
			pnextnode->prev->pnext = poolnode;
			pnextnode->prev = poolnode;
		}
	}

	__gpoolhead->num++;


#ifdef TEST_POOL
	if ( !TestSurfPool() )
		fprintf(stderr,"Surf Pool Gets ERR when adding node \n");
#endif

	return pretsurf;

}

/* Grab a surf from the pool, if return null, You have to allocate the new surface */
/* Otherwise you get surface from the pool, you have not to allocate the surface */
static GenericSurfacePtr GrabSurfFromPool(gctUINT alignedwidth, gctUINT alignedheight, gctUINT bytesPerPixel)
{
	PGSURFPOOL pnextnode = NULL;
	GenericSurfacePtr pret = NULL;
	gctUINT size = 0;


	SETPOOL(alignedwidth, alignedheight, bytesPerPixel);

	if ( __gpoolhead->pfirst == NULL )
	{
		gcmASSERT( __gpoolhead->num == 0 );
		return NULL;
	}

	size = alignedwidth * alignedheight * bytesPerPixel;

	pnextnode = __gpoolhead->pfirst;
	while ( pnextnode )
	{
		if ( pnextnode->surface->mVideoNode.mSizeInBytes >= size )
		{
			if ( pnextnode->pnext )
				pnextnode->pnext->prev = pnextnode->prev;

			pnextnode->prev->pnext = pnextnode->pnext;

			if ( pnextnode == __gpoolhead->pfirst)
			{
				__gpoolhead->pfirst = pnextnode->pnext;
				if ( pnextnode->pnext)
				pnextnode->pnext->prev = __gpoolhead->pfirst;
			}

			if (pnextnode == __gpoolhead->plast )
				if ( __gpoolhead->pfirst == NULL)
					__gpoolhead->plast = NULL;
				else
					__gpoolhead->plast = pnextnode->prev;

			pret = pnextnode->surface;

			__gpoolhead->num--;

			free(pnextnode);
			break;
		}

		pnextnode = pnextnode->pnext;

	}

#ifdef TEST_POOL
	if ( !TestSurfPool() )
		fprintf(stderr,"Surf Pool Gets ERR when grabbing node \n");
#endif

	return pret;

}

/************************************************************************
 * PIXMAP RELATED (START)
 ************************************************************************/
static gctBOOL FreeGPUSurface(VIVGPUPtr gpuctx, Viv2DPixmapPtr ppriv) {
    TRACE_ENTER();
    gceSTATUS status = gcvSTATUS_OK;
    GenericSurfacePtr surf = gcvNULL;
    gceSURF_TYPE surftype;
    Bool cacheable;

    surf = (GenericSurfacePtr) (ppriv->mVidMemInfo);
    if (surf->mIsWrapped) {
        goto delete_wrapper;
    }
    TRACE_INFO("DESTROYED SURFACE ADDRESS = %x - %x\n", surf, ppriv->mVidMemInfo);

    surf = AddGSurfIntoPool(surf);

    if ( surf ==NULL )
    {
        ppriv->mVidMemInfo = NULL;
        TRACE_EXIT(gcvTRUE);
    }

    if ( surf->mData )
        pixman_image_unref( (pixman_image_t *)surf->mData );

    surf->mData = gcvNULL;

    if ( surf->mAlignedWidth >= IMX_EXA_NONCACHESURF_WIDTH && surf->mAlignedHeight >= IMX_EXA_NONCACHESURF_HEIGHT )
    {
        surftype = gcvSURF_BITMAP;
        cacheable = FALSE;
    } else {
        surftype = SURFACE_TYPE;
        cacheable = SURFACE_CACHEABLE;
    }

    if (surf->mVideoNode.mNode != gcvNULL) {
        if (surf->mVideoNode.mLogicalAddr != gcvNULL) {
            status = UnlockVideoNode(gpuctx->mDriver->mHal, surf->mVideoNode.mNode, surftype);
            if (status != gcvSTATUS_OK) {
                TRACE_ERROR("Unable to UnLock video node\n");
                TRACE_EXIT(gcvFALSE);
            }
        }
        status = FreeVideoNode(gpuctx->mDriver->mHal, surf->mVideoNode.mNode);
        if (status != gcvSTATUS_OK) {
            TRACE_ERROR("Unable to Free video node\n");
            TRACE_EXIT(gcvFALSE);
        }
delete_wrapper:
        status = gcoOS_Free(gcvNULL, surf);
        if (status != gcvSTATUS_OK) {
            TRACE_ERROR("Unable to Free surface\n");
            TRACE_EXIT(gcvFALSE);
        }
        ppriv->mVidMemInfo = NULL;
    }
    TRACE_EXIT(gcvTRUE);
}

static gctBOOL VIV2DGPUSurfaceAlloc(VIVGPUPtr gpuctx, gctUINT alignedWidth, gctUINT alignedHeight,
        gctUINT bytesPerPixel, GenericSurfacePtr * surface) {
    TRACE_ENTER();
    gceSTATUS status = gcvSTATUS_OK;
    GenericSurfacePtr surf = gcvNULL;
    gctPOINTER mHandle = gcvNULL;
	gceSURF_TYPE surftype;
	Bool cacheable;
	surf = GrabSurfFromPool(alignedWidth, alignedHeight, bytesPerPixel);

	if ( surf ==NULL )
	{

    status = gcoOS_Allocate(gcvNULL, sizeof (GenericSurface), &mHandle);
    if (status != gcvSTATUS_OK) {
        TRACE_ERROR("Unable to allocate generic surface\n");
        TRACE_EXIT(FALSE);
    }

    memset(mHandle, 0, sizeof (GenericSurface));
    surf = (GenericSurfacePtr) mHandle;

    surf->mVideoNode.mSizeInBytes = alignedWidth * bytesPerPixel * alignedHeight;
    surf->mVideoNode.mPool = gcvPOOL_DEFAULT;

		if ( alignedWidth >= IMX_EXA_NONCACHESURF_WIDTH && alignedHeight >= IMX_EXA_NONCACHESURF_HEIGHT )
		{
			surftype = gcvSURF_BITMAP;
			cacheable = FALSE;
		} else {
			surftype = SURFACE_TYPE;
			cacheable = SURFACE_CACHEABLE;
		}

		status = AllocVideoNode(gpuctx->mDriver->mHal, &surf->mVideoNode.mSizeInBytes, &surf->mVideoNode.mPool, surftype, &surf->mVideoNode.mNode);
    if (status != gcvSTATUS_OK) {
        TRACE_ERROR("Unable to allocate video node\n");
        TRACE_EXIT(FALSE);
    }

		status = LockVideoNode(gpuctx->mDriver->mHal, surf->mVideoNode.mNode, cacheable, &surf->mVideoNode.mPhysicalAddr, &surf->mVideoNode.mLogicalAddr);
    if (status != gcvSTATUS_OK) {
        TRACE_ERROR("Unable to Lock video node\n");
        TRACE_EXIT(FALSE);
    }

    TRACE_INFO("VIDEO NODE CREATED =>  LOGICAL = %d  PHYSICAL = %d  SIZE = %d\n", surf->mVideoNode.mLogicalAddr, surf->mVideoNode.mPhysicalAddr, surf->mVideoNode.mSizeInBytes);
	}

    surf->mTiling = gcvLINEAR;
    surf->mAlignedWidth = alignedWidth;
    surf->mAlignedHeight = alignedHeight;
	surf->mBytesPerPixel = bytesPerPixel;
    surf->mStride = alignedWidth * bytesPerPixel;
    surf->mRotation = gcvSURF_0_DEGREE;
    surf->mLogicalAddr = surf->mVideoNode.mLogicalAddr;
    surf->mIsWrapped = gcvFALSE;
    surf->mData = gcvNULL;
    *surface = surf;

	TRACE_EXIT(TRUE);
}

Bool ReUseSurface(GALINFOPTR galInfo, PixmapPtr pPixmap, Viv2DPixmapPtr toBeUpdatedpPix)
{

	GenericSurfacePtr surf = gcvNULL;
	gctUINT alignedWidth, alignedHeight;
	gctUINT bytesPerPixel;
	alignedWidth = gcmALIGN(pPixmap->drawable.width, WIDTH_ALIGNMENT);
	alignedHeight = gcmALIGN(pPixmap->drawable.height, HEIGHT_ALIGNMENT);
	bytesPerPixel = BITSTOBYTES(pPixmap->drawable.bitsPerPixel);

	/* The same as CreatSurface */
	if (bytesPerPixel < 2) {
		bytesPerPixel = 2;
	}

	surf = (GenericSurfacePtr)toBeUpdatedpPix->mVidMemInfo;
	if ( surf && surf->mVideoNode.mSizeInBytes >= (alignedWidth * alignedHeight * bytesPerPixel))
	{
		surf->mTiling = gcvLINEAR;
		surf->mAlignedWidth = alignedWidth;
		surf->mAlignedHeight = alignedHeight;
		surf->mStride = alignedWidth * bytesPerPixel;
		surf->mRotation = gcvSURF_0_DEGREE;
		surf->mLogicalAddr = surf->mVideoNode.mLogicalAddr;
		surf->mIsWrapped = gcvFALSE;

		if ( surf->mData )
			pixman_image_unref( (pixman_image_t *)surf->mData );

		surf->mData = gcvNULL;
    TRACE_EXIT(TRUE);
}

	TRACE_EXIT(FALSE);
}

/*Creating and Destroying Functions*/
Bool CreateSurface(GALINFOPTR galInfo, PixmapPtr pPixmap, Viv2DPixmapPtr pPix) {
    GenericSurfacePtr surf = gcvNULL;
    VIVGPUPtr gpuctx = (VIVGPUPtr) galInfo->mGpu;
    gctUINT alignedWidth, alignedHeight;
    gctUINT bytesPerPixel;
    alignedWidth = gcmALIGN(pPixmap->drawable.width, WIDTH_ALIGNMENT);
    alignedHeight = gcmALIGN(pPixmap->drawable.height, HEIGHT_ALIGNMENT);
    bytesPerPixel = BITSTOBYTES(pPixmap->drawable.bitsPerPixel);

    /*QUICK FIX*/
    if (bytesPerPixel < 2) {
        bytesPerPixel = 2;
    }

    if (!VIV2DGPUSurfaceAlloc(gpuctx, alignedWidth, alignedHeight, bytesPerPixel, &surf)) {
        TRACE_ERROR("Surface Creation Error\n");
        TRACE_EXIT(FALSE);
    }

    pPix->mVidMemInfo = surf;
    TRACE_EXIT(TRUE);
}

Bool WrapSurface(PixmapPtr pPixmap, void * logical, unsigned int physical, Viv2DPixmapPtr pPix) {
    gceSTATUS status = gcvSTATUS_OK;
    gctUINT alignedWidth, alignedHeight;
    gctUINT bytesPerPixel;
    GenericSurfacePtr surf = gcvNULL;
    gctPOINTER mHandle = gcvNULL;
    status = gcoOS_Allocate(gcvNULL, sizeof (GenericSurface), &mHandle);
    if (status != gcvSTATUS_OK) {
        TRACE_ERROR("Unable to allocate generic surface\n");
        TRACE_EXIT(FALSE);
    }
    memset(mHandle, 0, sizeof (GenericSurface));
    surf = (GenericSurfacePtr) mHandle;

    alignedWidth = gcmALIGN(pPixmap->drawable.width, WIDTH_ALIGNMENT);
    alignedHeight = gcmALIGN(pPixmap->drawable.height, HEIGHT_ALIGNMENT);
    bytesPerPixel = BITSTOBYTES(pPixmap->drawable.bitsPerPixel);

    surf->mVideoNode.mSizeInBytes = alignedWidth * bytesPerPixel * alignedHeight;
    surf->mVideoNode.mPool = gcvPOOL_USER;

    surf->mVideoNode.mPhysicalAddr = physical;
    surf->mVideoNode.mLogicalAddr = (gctPOINTER) logical;

    surf->mBytesPerPixel = bytesPerPixel;
    surf->mTiling = gcvLINEAR;
    surf->mAlignedWidth = alignedWidth;
    surf->mAlignedHeight = alignedHeight;
    surf->mStride = alignedWidth * bytesPerPixel;
    surf->mRotation = gcvSURF_0_DEGREE;
    surf->mLogicalAddr = surf->mVideoNode.mLogicalAddr;
    surf->mIsWrapped = gcvTRUE;

    pPix->mVidMemInfo = surf;
    TRACE_EXIT(TRUE);
}

Bool DestroySurface(GALINFOPTR galInfo, Viv2DPixmapPtr ppix) {
    TRACE_ENTER();
    VIVGPUPtr gpuctx = (VIVGPUPtr) galInfo->mGpu;
    if (ppix->mVidMemInfo == NULL) {
        TRACE_INFO("NOT GPU GENERATED SURFACE\n");
        TRACE_EXIT(TRUE);
    }
    if (!FreeGPUSurface(gpuctx, ppix)) {
        TRACE_ERROR("Unable to free gpu surface\n");
        TRACE_EXIT(FALSE);
    }
    TRACE_EXIT(TRUE);
}

/*Mapping Functions*/
void * MapSurface(Viv2DPixmapPtr priv) {
    TRACE_ENTER();
    void * returnaddr = NULL;
    GenericSurfacePtr surf;
    surf = (GenericSurfacePtr) priv->mVidMemInfo;

    if ( surf == NULL )
	TRACE_EXIT(0);

    returnaddr = surf->mLogicalAddr;
    TRACE_EXIT(returnaddr);
}

void UnMapSurface(Viv2DPixmapPtr priv) {
    TRACE_ENTER();
    TRACE_EXIT();
}

char *MapViv2DPixmap(Viv2DPixmapPtr pdst ){

	GenericSurfacePtr surf = (GenericSurfacePtr) pdst->mVidMemInfo;

	return (surf ? surf->mVideoNode.mLogicalAddr:NULL);
}

unsigned int GetStride(Viv2DPixmapPtr pixmap) {
    TRACE_ENTER();
    GenericSurfacePtr surf = (GenericSurfacePtr) pixmap->mVidMemInfo;
    TRACE_EXIT(surf->mStride);
}

/************************************************************************
 * PIXMAP RELATED (END)
 ************************************************************************/

Bool MapUserMemToGPU(GALINFOPTR galInfo, MemMapInfoPtr mmInfo) {
    TRACE_ENTER();
    gceSTATUS status = gcvSTATUS_OK;
    gctPOINTER logical = (gctPOINTER) mmInfo->mUserAddr;
    gctSIZE_T size = (gctSIZE_T) (mmInfo->mSize);
    gctPOINTER mappingInfo = NULL;
    gctUINT32 physical = 0;
    VIVGPUPtr gpuctx = (VIVGPUPtr) (galInfo->mGpu);

    status = gcoOS_MapUserMemory(gpuctx->mDriver->mOs, logical, size, &mappingInfo, &physical);
    if (status < 0) {
        TRACE_ERROR("Mapping Failed\n");
        gcoOS_UnmapUserMemory(gpuctx->mDriver->mOs, logical, size, mappingInfo, physical);
        mmInfo->physical = 0;
        mmInfo->mapping = NULL;
        TRACE_EXIT(FALSE);
    }
    mmInfo->physical = physical;
    mmInfo->mapping = mappingInfo;
    TRACE_EXIT(TRUE);
}

void UnmapUserMem(GALINFOPTR galInfo, MemMapInfoPtr mmInfo) {
    TRACE_ENTER();
    gceSTATUS status = gcvSTATUS_OK;
    gctPOINTER logical = (gctPOINTER) mmInfo->mUserAddr;
    gctSIZE_T size = (gctSIZE_T) (mmInfo->mSize);
    gctPOINTER mappingInfo = (gctPOINTER) mmInfo->mapping;
    gctUINT32 physical = (gctUINT32) mmInfo->physical;
    VIVGPUPtr gpuctx = (VIVGPUPtr) (galInfo->mGpu);
    status = gcoOS_UnmapUserMemory(gpuctx->mDriver->mOs, logical, size, mappingInfo, physical);
    if (status < 0) {
        TRACE_ERROR("UnMapping Failed\n");
    }
    mmInfo->physical = 0;
    mmInfo->mapping = NULL;
    TRACE_EXIT();
}


#define  MAX_WIDTH		1024
#define  MAX_HEIGHT		1024

typedef struct _IVSURF {
gcoSURF	surf;
int		lineaddr;
}IVSURF,*PIVSURF;

static IVSURF _vsurf16={NULL,0};
static IVSURF _vsurf32={NULL,0};

static Bool VDestroySurf16() {
	gceSTATUS status = gcvSTATUS_OK;

	if (_vsurf16.surf==NULL) TRACE_EXIT(TRUE);

 	status=gcoSURF_Unlock(_vsurf16.surf, &(_vsurf16.lineaddr));

	if (status!=gcvSTATUS_OK)
		TRACE_EXIT(FALSE);

	status=gcoSURF_Destroy(_vsurf16.surf);

	_vsurf16.surf=NULL;

	TRACE_EXIT(TRUE);
}

static Bool VDestroySurf32() {

	gceSTATUS status = gcvSTATUS_OK;

	if (_vsurf32.surf==NULL) TRACE_EXIT(TRUE);

	status=gcoSURF_Unlock(_vsurf32.surf, &(_vsurf32.lineaddr));

	if (status!=gcvSTATUS_OK)
		TRACE_EXIT(FALSE);

	status=gcoSURF_Destroy(_vsurf32.surf);

	_vsurf32.surf=NULL;

	TRACE_EXIT(TRUE);

}

Bool  VGetSurfAddrBy16(GALINFOPTR galInfo,int maxsize,int *phyaddr,int *lgaddr,int *width,int *height,int *stride)
 {

	static int gphyaddr;
	static int glgaddr;
	static int gwidth;
	static int gheight;
	static int gstride;
	static int lastmaxsize=0;

	gceSTATUS status = gcvSTATUS_OK;

    	VIVGPUPtr gpuctx = (VIVGPUPtr) (galInfo->mGpu);

	if (maxsize <MAX_WIDTH)
		maxsize=MAX_WIDTH;

	if (_vsurf16.surf && (maxsize >lastmaxsize)) {
		if (VDestroySurf16()!=TRUE)
			TRACE_EXIT(FALSE);
		lastmaxsize=maxsize;
	}

	if (_vsurf16.surf==NULL) {

		lastmaxsize=maxsize;
		status=gcoSURF_Construct(gpuctx->mDriver->mHal,maxsize,maxsize,1,gcvSURF_BITMAP,gcvSURF_R5G6B5,gcvPOOL_DEFAULT,&(_vsurf16.surf));

		if (status!=gcvSTATUS_OK)
			TRACE_EXIT(FALSE);

		status=gcoSURF_GetAlignedSize(_vsurf16.surf,&gwidth,&gheight,&gstride);

		if (status!=gcvSTATUS_OK)
			TRACE_EXIT(FALSE);

		status=gcoSURF_Lock(_vsurf16.surf,  &gphyaddr, (void *)&glgaddr);

		_vsurf16.lineaddr=glgaddr;

	}

	*phyaddr=gphyaddr;
	*lgaddr=glgaddr;
	*width=gwidth;
	*height=gheight;
	*stride=gstride;

	TRACE_EXIT(TRUE);

 }


 Bool  VGetSurfAddrBy32(GALINFOPTR galInfo,int maxsize, int *phyaddr,int *lgaddr,int *width,int *height,int *stride)
 {

	static int gphyaddr;
	static int glgaddr;
	static int gwidth;
	static int gheight;
	static int gstride;
	static int lastmaxsize=0;
	gceSTATUS status = gcvSTATUS_OK;

	VIVGPUPtr gpuctx = (VIVGPUPtr) (galInfo->mGpu);

	if (maxsize <MAX_WIDTH)
		maxsize=MAX_WIDTH;

	if (_vsurf32.surf && (maxsize >lastmaxsize)) {
		if (VDestroySurf32()!=TRUE)
			TRACE_EXIT(FALSE);
		lastmaxsize=maxsize;
	}


	if (_vsurf32.surf==NULL) {

		lastmaxsize=maxsize;
		status=gcoSURF_Construct(gpuctx->mDriver->mHal,maxsize,maxsize,1,gcvSURF_BITMAP,gcvSURF_A8R8G8B8,gcvPOOL_DEFAULT,&(_vsurf32.surf));

		if (status!=gcvSTATUS_OK)
			TRACE_EXIT(FALSE);

		status=gcoSURF_GetAlignedSize(_vsurf32.surf,&gwidth,&gheight,&gstride);

		if (status!=gcvSTATUS_OK)
			TRACE_EXIT(FALSE);

		status=gcoSURF_Lock(_vsurf32.surf,  &gphyaddr, (void *)&glgaddr);

		_vsurf32.lineaddr=glgaddr;

	}

	*phyaddr=gphyaddr;
	*lgaddr=glgaddr;
	*width=gwidth;
	*height=gheight;
	*stride=gstride;

	TRACE_EXIT(TRUE);

}

void  VDestroySurf()
{
	VDestroySurf16();
	VDestroySurf32();
}
