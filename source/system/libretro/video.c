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

#include <string.h>

#include "misc/log.h"
#include "palette/palette.h"
#include "nes/nes.h"
#include "system/system.h"
#include "system/video.h"
#include "system/win32/resource.h"
#include "system/sdl/console/console.h"
#include "misc/memutil.h"
#include "misc/config.h"
#include "system/common/filters.h"

static const int NesWidth = 256;
static const int NesHeight = 240;
static const int BitsPerPixel = 32;

static const int Rmask = 0x00FF0000;
static const int Gmask = 0x0000FF00;
static const int Bmask = 0x000000FF;

static const int Rshift = 16;
static const int Gshift = 8;
static const int Bshift = 0;

static const int Rloss = 0;
static const int Gloss = 0;
static const int Bloss = 0;

u32 *pixel32; /* video framebuffer */
//system related variables
static int screenw,screenh,screenbpp;
static int screenscale = 1;

//palette with emphasis applied
static u8 palette[8][64 * 3];

//palette data fed to video system
static u32 palette32[8][256];		//32 bit color

//caches of all available colors
static u32 palettecache32[256];

//actual values written to nes palette ram
static u8 palettecache[32];

//pointer to scree and copy of the nes screen
static u32 *screen = 0;
static u8 *nesscreen = 0;

//draw function pointer and pointer to current video filter
static void (*drawfunc)(void*,u32,const void*,u32,u32,u32);		//dest,destpitch,src,srcpitch,width,height
static filter_t *filter;

static int find_drawfunc(int scale,int bpp)
{
	int i;

	for(i=0;filter->modes[i].scale;i++) {
		if(filter->modes[i].scale == scale) {
			switch(bpp) {
				case 32:
					drawfunc = filter->modes[i].draw32;
					return(0);
				case 16:
				case 15:
				//	drawfunc = filter->modes[i].draw16;
				//	return(0);
				default:
					log_printf("find_drawfunc:  unsupported bit depth (%d)\n",bpp);
					return(2);
			}
		}
	}
	return(1);
}

int video_init()
{
	if(nesscreen == 0)
		nesscreen = (u8*)mem_alloc(256 * (240 + 16));

	memset(palettecache32,0,256*sizeof(u32));

	//set screen info
	screenscale = config_get_int("video.scale");
	screenbpp = BitsPerPixel;

	//initialize the video filters
	filter_init();

	//get pointer to video filter
	filter = filter_get((screenscale == 1) ? F_NONE : filter_get_int(config_get_string("video.filter")));

	if(find_drawfunc(screenscale,screenbpp) != 0) {
		log_printf("video_init:  error finding appropriate draw func, using draw1x\n");
		filter = &filter_draw;
		drawfunc = filter->modes[0].draw32;
	}

	//calculate desired screen dimensions
	screenw = filter->minwidth / filter->minscale * screenscale;
	screenh = filter->minheight / filter->minscale * screenscale;

	//initialize surface/window
	pixel32 = (u32*)mem_realloc(pixel32, screenw * sizeof(u32) * screenh);

	//allocate memory for temp screen buffer
	screen = (u32*)mem_realloc(screen,256 * (240 + 16) * (screenbpp / 8) * 4);

	//print information
	log_printf("video initialized:  %dx%dx%d %s\n",screenw,screenh,BitsPerPixel,"windowed");

	return(0);
}

void video_kill()
{
	filter_kill();
	if(screen)
		mem_free(screen);
	if(nesscreen)
		mem_free(nesscreen);
	if(pixel32)
		mem_free(pixel32);
	screen = 0;
	nesscreen = 0;
	pixel32 = 0;
}

int video_reinit()
{
	video_kill();
	return(video_init());
}

void video_startframe()
{
}

void video_endframe()
{
	u64 t;

	//draw everything
	drawfunc(pixel32,screenw*sizeof(u32),screen,256*4,256,240);
	console_draw((u32*)pixel32,screenw*sizeof(u32),screenh);
}

//this handles lines for gui/status messages
void video_updateline(int line,u8 *s)
{
	u32 *dest = screen + (line * 256);
	int i;

	memcpy(nesscreen + (line * 256),s,256);
	if(line >= 8 && line < 232) {
		for(i=0;i<256;i++) {
			*dest++ = palettecache32[*s++];
		}
	}
	else {
		for(i=0;i<256;i++) {
			*dest++ = 0;
		}
	}
}

//this handles pixels coming directly from the nes engine
void video_updatepixel(int line,int pixel,u8 s)
{
	int offset = (line * 256) + pixel;

	nesscreen[offset] = s;
	if(line >= 8 && line < 232) {
		screen[offset] = palettecache32[s];
	}
	else {
		screen[offset] = 0;
	}
}

#define MAKERGB555(pp) \
	(((pp) >> (3 + 0)) << 0) | \
	(((pp) >> (3 + 8)) << 5) | \
	(((pp) >> (3 + 16)) << 10);

void video_updaterawpixel(int line, int pixel, u32 s)
	{
	int offset = (line * 256) + pixel;

	switch (screenbpp) {
	case 15:
	case 16:
		screen[offset] = MAKERGB555(s);
		break;
	}
}

//this handles palette changes from the nes engine
void video_updatepalette(u8 addr,u8 data)
{
	palettecache32[addr+0x00] = palette32[0][data];
	palettecache32[addr+0x20] = palette32[1][data];
	palettecache32[addr+0x40] = palette32[2][data];
	palettecache32[addr+0x60] = palette32[3][data];
	palettecache32[addr+0x80] = palette32[4][data];
	palettecache32[addr+0xA0] = palette32[5][data];
	palettecache32[addr+0xC0] = palette32[6][data];
	palettecache32[addr+0xE0] = palette32[7][data];
	palettecache[addr] = data;
}

//must be called AFTER video_init
void video_setpalette(palette_t *p)
{
	int i,j;
	palentry_t *e;

	for(j=0;j<8;j++) {
		for(i=0;i<64;i++) {
			palette[j][(i * 3) + 0] = p->pal[j][i].r;
			palette[j][(i * 3) + 1] = p->pal[j][i].g;
			palette[j][(i * 3) + 2] = p->pal[j][i].b;
		}
	}

	for(j=0;j<8;j++) {
		for(i=0;i<256;i++) {
			e = &p->pal[j][i & 0x3F];
			palette32[j][i] = (e->r << 16) | (e->g << 8) | (e->b << 0);
		}
	}

	filter_palette_changed();
}

int video_getwidth()			{	return(screenw);			}
int video_getheight()			{	return(screenh);			}
int video_getbpp()				{	return(screenbpp);		}
u8 *video_getscreen()			{	return(nesscreen);		}
u8 *video_getpalette()			{	return((u8*)palette);	}

int video_zapperhit(int x,int y)
{
	int ret = 0;
	u8 *e;
	u8 color;

	color = palettecache[nesscreen[x + y * 256]];
	e = &palette[(color >> 5) & 7][(color & 0x3F) * 3];
	ret += (int)(e[0] * 0.299f);
	ret += (int)(e[1] * 0.587f);
	ret += (int)(e[2] * 0.114f);
	return((ret >= 0x40) ? 1 : 0);
}

//kludge-city!
int video_getxoffset()	{	return(0);	}
int video_getyoffset()	{	return(0);	}
int video_getscale()	{	return(screenscale);	}
