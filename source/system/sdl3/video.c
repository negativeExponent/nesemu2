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

#include <SDL3/SDL.h>
#include "misc/log.h"
#include "palette/palette.h"
#include "nes/nes.h"
#include "system/system.h"
#include "system/video.h"
#include "system/win32/resource.h"
#include "system/sdl2/console/console.h"
#include "misc/memutil.h"
#include "misc/config.h"
#include "system/common/filters.h"

//system related variables
static SDL_Window *window = 0;
static SDL_Renderer *renderer = 0;
static SDL_Texture *texture = 0;
static SDL_Surface *surface = 0;

static int flags = SDL_WINDOW_OPENGL;
static int screenw,screenh,screenbpp;
static int screenscale;

//palette with emphasis applied
static u8 palette[8][64 * 3];

//palette data fed to video system
static u16 palette15[8][256];		//15 bit color
static u16 palette16[8][256];		//16 bit color
static u32 palette32[8][256];		//32 bit color

//caches of all available colors
static u16 palettecache16[256];
static u32 palettecache32[256];

//actual values written to nes palette ram
static u8 palettecache[32];

//for frame limiting
static double interval = 0;
static u64 lasttime = 0;

//pointer to scree and copy of the nes screen
static u32 *screen = 0;
static u8 *nesscreen = 0;

//draw function pointer and pointer to current video filter
static void (*drawfunc)(void*,u32,const void*,u32,u32,u32);		//dest,destpitch,src,srcpitch,width,height
static filter_t *filter;

//for correct colors
static int rshift,gshift,bshift;
static int rloss,gloss,bloss;

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

static void get_surface_info(SDL_Surface *s)
{
	SDL_PixelFormatDetails *vi = SDL_GetPixelFormatDetails(s->format);

	log_printf("get_surface_info:  sdl surface info:\n");
	log_printf("  bits per pixel:  %d\n", vi->bits_per_pixel);
	log_printf("    red:    mask:  %08X    shift:  %2d    loss: %d\n",vi->Rmask,vi->Rshift,vi->Rbits);
	log_printf("    green:  mask:  %08X    shift:  %2d    loss: %d\n",vi->Gmask,vi->Gshift,vi->Gbits);
	log_printf("    blue:   mask:  %08X    shift:  %2d    loss: %d\n",vi->Bmask,vi->Bshift,vi->Bbits);

	rshift = vi->Rshift;
	gshift = vi->Gshift;
	bshift = vi->Bshift;
	rloss = 0;
	gloss = 0;
	bloss = 0;
}

int video_init()
{
	if(SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
		log_printf("video_init: Initialization failed. SDL error: %s\n", SDL_GetError());
		return(1);
	}

	if(nesscreen == 0)
		nesscreen = (u8*)mem_alloc(256 * (240 + 16));

	//setup timer to limit frames
	interval = (double)system_getfrequency() / 60.0f;
	lasttime = system_gettick();

	//clear palette caches
	memset(palettecache16,0,256*sizeof(u16));
	memset(palettecache32,0,256*sizeof(u32));

	//set screen info
	flags &= ~SDL_WINDOW_FULLSCREEN;
	flags |= config_get_bool("video.fullscreen") ? SDL_WINDOW_FULLSCREEN : 0;
	screenscale = config_get_int("video.scale");

	//fullscreen mode
	if(flags & SDL_WINDOW_FULLSCREEN) {
		screenscale = (screenscale < 2) ? 2 : screenscale;
		screenbpp = 32;
	}

	//windowed mode
	else {
		screenbpp = 32;
	}

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
	window   = SDL_CreateWindow("nesemu2", screenw, screenh, 0);
	renderer = SDL_CreateRenderer(window, NULL);
	texture  = SDL_CreateTexture(renderer,SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, screenw, screenh);
	surface  = SDL_CreateSurface(screenw, screenh, SDL_PIXELFORMAT_XRGB8888);

	if (window == NULL || renderer == NULL || texture == NULL || surface == NULL) {
		log_printf("video_init: Initialization failed. SDL error: %s\n", SDL_GetError());
		return(1);
	}

	SDL_SetWindowFullscreen(window, (flags & SDL_WINDOW_FULLSCREEN) != 0);
	SDL_SetRenderLogicalPresentation(renderer, screenw, screenh, SDL_LOGICAL_PRESENTATION_LETTERBOX);

	/*SDL_ShowCursor(0);*/
	get_surface_info(surface);

	//allocate memory for temp screen buffer
	screen = (u32*)mem_realloc(screen,256 * (240 + 16) * (screenbpp / 8) * 4);

	//print information
	log_printf("video initialized:  %dx%dx%d %s\n",surface->w,surface->h,
		SDL_GetPixelFormatDetails(surface->format)->bits_per_pixel, flags & SDL_WINDOW_FULLSCREEN ? "fullscreen" : "windows");
	
	return(0);
}

void video_kill()
{
	filter_kill();
	/*SDL_ShowCursor(1);*/
	if(screen)
		mem_free(screen);
	if(nesscreen)
		mem_free(nesscreen);
	screen = 0;
	nesscreen = 0;
	
	SDL_DestroySurface(surface);
	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	surface = 0;
	texture = 0;
	renderer = 0;
	window = 0;
}

int video_reinit()
{
	video_kill();
	return(video_init());
}

void video_startframe()
{
	//SDL_LockSurface(surface);
}

void video_endframe()
{
	u64 t;

	//draw everything
	drawfunc(surface->pixels,surface->pitch,screen,256*4,256,240);
	console_draw(surface->pixels,surface->pitch,screenh);

	//SDL_UnlockSurface(surface);
	SDL_UpdateTexture(texture, NULL, surface->pixels,surface->pitch);
	SDL_RenderClear(renderer);
	SDL_RenderTexture(renderer, texture, NULL, NULL);
	SDL_RenderPresent(renderer);

	//simple frame limiter
	if(config_get_bool("video.framelimit")) {
		do {
			t = system_gettick();
		} while((double)(t - lasttime) < interval);
		lasttime = t;
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

	log_printf("%d %d %d\n", rshift, gshift, bshift);

	for(j=0;j<8;j++) {
		for(i=0;i<256;i++) {
			e = &p->pal[j][i & 0x3F];
			palette32[j][i] = (e->r << rshift) | (e->g << gshift) | (e->b << bshift);
		}
	}

	filter_palette_changed();
}

int video_getwidth()				{	return(screenw);			}
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
int video_getscale()		{	return(screenscale);	}
