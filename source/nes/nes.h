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

#ifndef __nes_h__
#define __nes_h__

#include "types.h"
#include "nes/movie.h"
#include "nes/cpu/cpu.h"
#include "nes/ppu/ppu.h"
#include "nes/apu/apu.h"
#include "nes/cart/cart.h"
#include "mappers/mappers.h"
#include "inputdev/inputdev.h"

//movie modes
#define MOVIE_PLAY	0x01
#define MOVIE_RECORD	0x02

//irq masks
#define IRQ_TIMER		0x01		//fds
#define IRQ_DISK		0x02		//fds
#define IRQ_MAPPER	0x10		//mappers
#define IRQ_MAPPER2	0x20		//mappers
#define IRQ_FRAME		0x40		//apu
#define IRQ_DPCM		0x80		//apu

//timing
#define LINECYCLES	nes->ppu.linecycles
#define SCANLINE		nes->ppu.scanline
#define FRAMES			nes->ppu.frames

//scroll registers
#define SCROLLX		nes->ppu.scrollx
#define SCROLL			nes->ppu.scroll
#define TMPSCROLL		nes->ppu.tmpscroll
#define TOGGLE			nes->ppu.toggle

//ppu io
#define IOADDR			nes->ppu.ioaddr
#define IODATA			nes->ppu.iodata
#define IOMODE			nes->ppu.iomode

//registers
#define PPUCONTROL	nes->ppu.control0		//'correct' names
#define PPUMASK		nes->ppu.control1
#define CONTROL0		nes->ppu.control0		//old names
#define CONTROL1		nes->ppu.control1
#define STATUS			nes->ppu.status

typedef struct nes_s {

	//2a03/2c02 data
	cpu_t			cpu;
	apu_t			apu;
	ppu_t			ppu;

	//cartridge inserted
	cart_t		*cart;

	//mapper functions
	mapper_t		*mapper;

	//input strobe and connected devices
	u8				strobe;
	inputdev_t	*inputdev[2],*expdev;

	//movie support
	movie_t		movie;

} nes_t;

extern nes_t *nes;

int nes_init();
void nes_kill();
int nes_load_cart(cart_t *c);
int nes_load(char *filename);
int nes_load_patched(char *filename,char *patchfilename);
void nes_unload();
void nes_set_inputdev(int n,int id);
void nes_reset(int hard);
void nes_frame();
void nes_state(int mode,u8 *data);
void nes_savestate(char *filename);
void nes_loadstate(char *filename);

#endif
