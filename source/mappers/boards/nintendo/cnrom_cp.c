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

#include "mappers/mapperinc.h"
#include "mappers/chips/latch.h"

static cache_t dummy[1024];
static int type;

static u8 readchr(u32 addr)
{
	return(0xFF);
}

static void sync()
{
	int i;
	int chrEnable;

	switch(type) {
	case B_NINTENDO_CNROM_CP_SEICROSSV2:
		chrEnable = ((latch_data & 1) == 0); /* 7 */
		break;
	default:
	case B_NINTENDO_CNROM_CP:
		chrEnable = ((latch_data & 3) != 0) && (latch_data != 0x13); /* 1, 2, 3, 4, 5, 6 */
		break;
	}

	//chr enabled
	if(chrEnable) {
		mem_setchr8(0,0);
		for(i=0;i<8;i++)
			nes->ppu.readfuncs[i] = 0;
	}

	//chr disabled
	else {
		mem_unsetppu8(0);
		for(i=0;i<8;i++) {
			nes->ppu.readfuncs[i] = readchr;
			/* hack to avoid reading from inaccessible pointer locatiom */
			nes->ppu.cachepages[i] = dummy;
			nes->ppu.cachepages_hflip[i] = dummy;
		}
	}
}

static void reset(int t, int hard)
{
	type = t;
	latch_reset(sync,hard);
	mem_setprg32(0x8,0);
}

static void reset_cnrom_cp(int hard) {
	reset(B_NINTENDO_CNROM_CP, hard);
}

static void reset_cnrom_seicrossv2(int hard) {
	reset(B_NINTENDO_CNROM_CP_SEICROSSV2, hard);
}

MAPPER(B_NINTENDO_CNROM_CP,reset_cnrom_cp,0,0,latch_state);
MAPPER(B_NINTENDO_CNROM_CP_SEICROSSV2,reset_cnrom_seicrossv2,0,0,latch_state);
