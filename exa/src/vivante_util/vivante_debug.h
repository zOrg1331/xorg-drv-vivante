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
 * File:   vivante_debug.h
 * Author: vivante
 *
 * Created on December 24, 2011, 12:50 PM
 */

#ifndef VIVANTE_DEBUG_H
#define VIVANTE_DEBUG_H

#ifdef __cplusplus
extern "C" {
#endif
#include <stdio.h>

    /*******************************************************************************
     *
     * DEBUG Macros (START)
     *
     ******************************************************************************/
    //#define VIVFBDEV_DEBUG
#ifdef VIVFBDEV_DEBUG
#define DEBUGP(x, args ...) fprintf(stderr, "[%s(), %s:%u]\n\n" \
x, __FILE__, __FUNCTION__ ,__LINE__, ## args)
#else
#define DEBUGP(x, args ...)
#endif

#ifdef VIVFBDEV_DEBUG
#define TRACE_ENTER() \
    do {  PrintEnter(); DEBUGP("ENTERED FUNCTION : %s\n", __FUNCTION__); } while (0)
#define TRACE_EXIT(val) \
    do { PrintExit(); DEBUGP("EXITED FUNCTION : %s\n", __FUNCTION__); return val;  } while (0)
#define TRACE_INFO(x, args ...) \
    do { fprintf(stderr, "[INFO : %s(), %s:%u]\n\n" x, __FILE__, __FUNCTION__ ,__LINE__, ## args); } while (0)
#define TRACE_ERROR(x, args ...) \
    do {  fprintf(stderr, "[ERROR : %s(), %s:%u]\n\n" x, __FILE__, __FUNCTION__ ,__LINE__, ## args); } while (0)
#else
#define TRACE_ENTER() \
    do {  ; } while (0)
#define TRACE_EXIT(val) \
    do { return val;  } while (0)
#define TRACE_INFO(x, args ...) \
    do { ;} while (0)
#define TRACE_ERROR(x, args ...) \
    do {  ;} while (0)
#endif



    /*******************************************************************************
     *
     * DEBUG Macros (END)
     *
     ******************************************************************************/

    void PrintEnter();
    void PrintExit();
    void PrintString(const char* str);

#ifdef __cplusplus
}
#endif

#endif	/* VIVANTE_DEBUG_H */

