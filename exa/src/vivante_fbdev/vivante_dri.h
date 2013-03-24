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


#ifndef _VIVANTE_DRI_H_
#define _VIVANTE_DRI_H_ 1

/*
 * Datatypes
 */
typedef unsigned int	GLenum;
typedef unsigned char	GLboolean;
typedef unsigned int	GLbitfield;
typedef void		GLvoid;
typedef signed char	GLbyte;		/* 1-byte signed */
typedef short		GLshort;	/* 2-byte signed */
typedef int		GLint;		/* 4-byte signed */
typedef unsigned char	GLubyte;	/* 1-byte unsigned */
typedef unsigned short	GLushort;	/* 2-byte unsigned */
typedef unsigned int	GLuint;		/* 4-byte unsigned */
typedef int		GLsizei;	/* 4-byte signed */
typedef float		GLfloat;	/* single precision float */
typedef float		GLclampf;	/* single precision float in [0,1] */
typedef double		GLdouble;	/* double precision float */
typedef double		GLclampd;	/* double precision float in [0,1] */
typedef int     Bool;

#define _XF86DRI_SERVER_
#include "drm.h"
#include "xf86drm.h"
#include "dri.h"
#include "sarea.h"

#define VIV_DRI_VERSION_MAJOR		4
#define VIV_DRI_VERSION_MINOR		1
#define VIV_MAX_DRAWABLES           256

typedef struct _ScreenConfigRec {
    int virtualX;
    int virtualY;
} screenConfig;

typedef struct _vvtDeviceInfoRec {
    int bufBpp;
    int zbufBpp;
    int backOffset;
    int depthOffset;
    int sareaPrivOffset;
    screenConfig ScrnConf;
} vvtDeviceInfo;

Bool VivDRIScreenInit(ScreenPtr pScreen);
void VivDRICloseScreen(ScreenPtr pScreen);
Bool VivDRIFinishScreenInit(ScreenPtr pScreen);

#endif /* _VIVANTE_DRI_H_ */
