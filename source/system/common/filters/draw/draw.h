/***************************************************************************
 *   Copyright (C) 2013 by James Holodnak                                  *
 *   jamesholodnak@gmail.com                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef __draw_h__
#define __draw_h__

#include "types.h"

void draw1x(void *void_dst,u32 destp,const void *void_src,u32 srcp,u32 w,u32 h);
void draw2x(void *void_dst,u32 destp,const void *void_src,u32 srcp,u32 w,u32 h);
void draw3x(void *void_dst,u32 destp,const void *void_src,u32 srcp,u32 w,u32 h);
void draw4x(void *void_dst,u32 destp,const void *void_src,u32 srcp,u32 w,u32 h);
void draw5x(void *void_dst,u32 destp,const void *void_src,u32 srcp,u32 w,u32 h);
void draw6x(void *void_dst,u32 destp,const void *void_src,u32 srcp,u32 w,u32 h);

#endif
