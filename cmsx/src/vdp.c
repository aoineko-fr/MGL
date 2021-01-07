//-----------------------------------------------------------------------------
//  █▀▀ █▀▄▀█ █▀ ▀▄▀
//  █▄▄ █ ▀ █ ▄█ █ █ v0.2
//-----------------------------------------------------------------------------
/**
 * @file Features to manage the VDP - Source code
 *
 * References:
 * - V9938 Technical Data Book Programmer's Guide
 * - https://www.msx.org/wiki/Category:VDP_Registers
 * - http://map.grauw.nl/articles/
 */
#include "vdp.h"
#include "bios_port.h"

//-----------------------------------------------------------------------------
// VDP Registers

u8 g_VDP_REGSAV[28];
u8 g_VDP_STASAV[10];
struct VDP_Command g_VDP_Command;
struct VDP_Sprite g_VDP_Sprite;
u8  g_ScreenLayoutHigh;		///< Address of the Pattern Layout Table
u16 g_ScreenLayoutLow;		///< Address of the Pattern Layout Table
u8  g_ScreenColorHigh;		///< Address of the Color Table
u16 g_ScreenColorLow;		///< Address of the Color Table
u8  g_ScreenPatternHigh;	///< Address of the Pattern Generator Table
u16 g_ScreenPatternLow;		///< Address of the Pattern Generator Table
u8  g_SpriteAtributeHigh;	///< Address of the Sprite Attribute Table
u16 g_SpriteAtributeLow;	///< Address of the Sprite Attribute Table
u8  g_SpritePatternHigh;	///< Address of the Sprite Pattern Generator Table
u16 g_SpritePatternLow;		///< Address of the Sprite Pattern Generator Table
u8  g_SpriteColorHigh;		///< Address of the Sprite Color Table
u16 g_SpriteColorLow;		///< Address of the Sprite Color Table

//-----------------------------------------------------------------------------
// VDP Registers Flags

#define F_VDP_REG	0x80	// VDP register write port (bit 7=1 in second write)
#define F_VDP_VRAM	0x00	// VRAM address register (bit 7=0 in second write)
#define F_VDP_WRIT	0x40	// bit 6: read/write access (0=read)
#define F_VDP_READ	0x00	//

#define VDP_REG(_r) (F_VDP_REG | _r)

//-----------------------------------------------------------------------------
//
//   M S X   1   F U N C T I O N S
//
//-----------------------------------------------------------------------------

#if USE_VDP_MODE_T1
//-----------------------------------------------------------------------------
/// Data structure to initialize Text 1 screen mode
static const u8 modeT1[] = 
{
	0x00, VDP_REG(0), // Text Mode, No External Video
	0xD0, VDP_REG(1), // 16K, Enable Disp., Disable Int.
	0x02, VDP_REG(2), // Address of Name Table in VRAM = 0800h
	//    VDP_REG(3), // (not used) Color is defined in Reg. 7
	0x00, VDP_REG(4), // Address of Patern Table in VRAM = 0000h
	//    VDP_REG(5), // (not used)
	//    VDP_REG(6), // (not used)
	0xF5, VDP_REG(7), // White Text on Light Blue Background
};
/// Initialize Text 1 screen mode. Use MSX 1 method without incremental register writing.
void VDP_SetModeText1()
{
	// Backup vdp register @todo Can be optimized
	for(u8 i = 0; i < 10; i += 2)
		g_VDP_REGSAV[modeT1[i + 1] & 0x7F] = modeT1[i];

	__asm
		ld		hl, #_modeT1
		ld		bc, #(10 * 0x100 + P_VDP_ADDR) // [num|0x99]
		otir
	__endasm;
}
#endif // USE_VDP_MODE_T1

#if USE_VDP_MODE_MC
//-----------------------------------------------------------------------------
/// Data structure to initialize Multi Color screen mode
static const u8 modeMC[] = 
{
	0x00, VDP_REG(0), // Multicolor Mode, No External Video
	0xCB, VDP_REG(1), // 16K, Enable Disp., Disable Int., 16x16 Sprites, Mag. On
	0x05, VDP_REG(2), // Address of Name Table in VRAM = 1400h
	//    VDP_REG(3), // (not used)
	0x01, VDP_REG(4), // Address of Patern Table in VRAM = 0800h
	0x20, VDP_REG(5), // Address of Sprite Attribute Table in VRAM = 1000h
	0x00, VDP_REG(6), // Address of Sprite Pattern Table in VRAM = 0000h
	0x04, VDP_REG(7), // Backdrop Color = Dark Blue
};
/// Initialize Multi Color screen mode. Use MSX 1 method without incremental register writing.
void VDP_SetModeMultiColor()
{
	// Backup vdp register @todo Can be optimized
	for(u8 i = 0; i < 14; i += 2)
		g_VDP_REGSAV[modeMC[i + 1] & 0x7F] = modeMC[i];

	__asm
		ld		hl, #_modeMC
		ld		bc, #(14 * 0x100 + P_VDP_ADDR) // [num|0x99]
		otir
	__endasm;
}
#endif // USE_VDP_MODE_MC

#if USE_VDP_MODE_G1
//-----------------------------------------------------------------------------
/// Data structure to initialize Graphic 1 screen mode
static const u8 modeG1[] = 
{
	0x00, VDP_REG(0), // Graphics 1 Mode, No External Video
	0xC0, VDP_REG(1), // 16K, Enable display, Disable Int, 8x8 Srpites, Mag. off
	0x05, VDP_REG(2), // Address of Name Table in VRAM = 1400h
	0x80, VDP_REG(3), // Address of Color Table in VRAM = 2000h
	0x01, VDP_REG(4), // Address of Patern Table in VRAM = 0800h
	0x20, VDP_REG(5), // Address of Sprite Attribute Table in VRAM = 1000h
	0x00, VDP_REG(6), // Address of Sprite Pattern Table in VRAM = 0000h
	0x01, VDP_REG(7), // Backdrop Color = Black
};
/// Initialize Graphic 1 screen mode. Use MSX 1 method without incremental register writing.
void VDP_SetModeGraphic1()
{
	// Backup vdp register @todo Can be optimized
	for(u8 i = 0; i < 16; i += 2)
		g_VDP_REGSAV[modeG1[i + 1] & 0x7F] = modeG1[i];

	__asm
		ld		hl, #_modeG1
		ld		bc, #(16 * 0x100 + P_VDP_ADDR) // [num|0x99]
		otir
	__endasm;
}
#endif // USE_VDP_MODE_G1

#if USE_VDP_MODE_G2
//-----------------------------------------------------------------------------
/// Data structure to initialize Graphic 2 screen mode
static const u8 modeG2[] = 
{
	0x02, VDP_REG(0), // Graphics 2 Mode, No External Video
	0xC2, VDP_REG(1), // 16K, Enable Disp., Disable Int., 16x16 Sprites, Mag. Off
	0x0E, VDP_REG(2), // Address of Name Table in VRAM = 3800h
	0xFF, VDP_REG(3), // Address of Color Table in VRAM = 2000h
	0x03, VDP_REG(4), // Address of Patern Table in VRAM = 0000h
	0x76, VDP_REG(5), // Address of Sprite Attribute Table in VRAM = 3B00h
	0x03, VDP_REG(6), // Address of Sprite Pattern Table in VRAM = 1800h
	0x0F, VDP_REG(7), // Backdrop Color = White
};
/// Initialize Graphic 2 screen mode. Use MSX 1 method without incremental register writing.
void VDP_SetModeGraphic2()
{
	// Backup vdp register @todo Can be optimized
	for(u8 i = 0; i < 16; i += 2)
		g_VDP_REGSAV[modeG2[i + 1] & 0x7F] = modeG2[i];

	__asm
		ld		hl, #_modeG2
		ld		bc, #(16 * 0x100 + P_VDP_ADDR) // [num|0x99]
		otir
	__endasm;
}
#endif // USE_VDP_MODE_G2

//-----------------------------------------------------------------------------
//
//   M S X   2   F U N C T I O N S
//
//-----------------------------------------------------------------------------
#if (MSX_VERSION >= MSX_2)

//-----------------------------------------------------------------------------
/// Incremental VDP registers writing
/// @param	src		Address of the data to be write into the registers
/// @param	count	Number of registers to be write
/// @param	reg		First register to be write (will be automaticaly incremented at each write)
void VDP_RegIncWrite(u16 src, u8 count, u8 reg)
{
	src, count, reg;
	
	__asm
		push	ix
		ld		ix, #0
		add		ix, sp

		// Backup VDP registers
		//ld		de, #(_g_VDP_REGSAV + 0) // first reg
		ld		hl, #_g_VDP_REGSAV
		ld		b, #0
		ld		c, 7(ix)				// first register
		add		hl, bc
		ld		d, h
		ld		e, l

		ld		b, #0
		ld		c, 6(ix)				// size
		ld		l, 4(ix)				// source address
		ld		h, 5(ix)
		ldir

		// Setup incremental VDP port writing
		ld		a, 7(ix)				// first register
		di //~~~~~~~~~~~~~~~~~~~~~~~~~~
		out		(P_VDP_ADDR), a
		ld		a, #VDP_REG(17)
		ei //~~~~~~~~~~~~~~~~~~~~~~~~~~
		out		(P_VDP_ADDR), a

		// Do incremental VDP port writing
		ld		b, 6(ix)				// size
		ld		c, #P_VDP_IREG
		ld		l, 4(ix)				// source address
		ld		h, 5(ix)
		otir

		pop		ix
	__endasm;
}

#define OUTI_1	\
	ei			\
	outi

#define OUTI_2	\
	OUTI_1		\
	outi

#define OUTI_3	\
	OUTI_2		\
	outi

#define OUTI_4	\
	OUTI_3		\
	outi

#define OUTI_5	\
	OUTI_4		\
	outi

#define OUTI_6	\
	OUTI_5	\
	outi

#define OUTI_7	\
	OUTI_6		\
	outi

#define OUTI_8	\
	OUTI_7		\
	outi

#define OUTI_9	\
	OUTI_8		\
	outi

#define OUTI_10	\
	OUTI_9		\
	outi

//#define OUTI(_x)	OUTI_##_x
#define OUTI(_x)	\
	.rept _x		\
		outi		\
	.endm			\
	ei

/// Fast incremental write to VDP register with backup to RAM
#define ASM_REG_WRITE_INC_BK(_addr, _reg, _count)	\
	__asm											\
		ld		hl, #(_##_addr)						\
		ld		de, #(_g_VDP_REGSAV + _reg)			\
		ld		bc, #(_count)						\
		ldir										\
	__endasm;										\
	ASM_REG_WRITE_INC(_addr, _reg, _count)

/// Fast incremental write to VDP register
#define ASM_REG_WRITE_INC(_addr, _reg, _count)		\
	__asm											\
		ld		a, #(_reg)							\
		di											\
		out		(P_VDP_ADDR), a						\
		ld		a, #VDP_REG(17)						\
		ei											\
		out		(P_VDP_ADDR), a						\
		ld		hl, #(_##_addr)						\
		ld		c, #P_VDP_IREG						\
		di											\
		OUTI(_count) ; 'ei' included				\
	__endasm

#if USE_VDP_MODE_T2
//-----------------------------------------------------------------------------
/// Data structure to initialize Text 2 screen mode
static const u8 modeT2[] = 
{
	0x04, // R#0
	0x70, // R#1
	0x03, // R#2
	0x27, // R#3
	0x02, // R#4
	0x36, // R#5
	0x07, // R#6
	0xF4, // R#7
	0x08, // R#8
	0x02, // R#9
};
/// Initialize Text 2 screen mode. Use MSX 2 incremental VDP registers writing
void VDP_SetModeText2()
{
	ASM_REG_WRITE_INC_BK(modeT2, 0, 10);
}
#endif // USE_VDP_MODE_T2

#if USE_VDP_MODE_G3
//-----------------------------------------------------------------------------
/// Data structure to initialize Graphic 3 screen mode
static const u8 modeG3[] = 
{
	0x04, // R#0
	0x60, // R#1
	0x06, // R#2
	0xFF, // R#3
	0x03, // R#4
	0x3F, // R#5
	0x07, // R#6
	0x04, // R#7
	0x08, // R#8
	0x02, // R#9
};
/// Initialize Graphic 3 screen mode. Use MSX 2 incremental VDP registers writing
void VDP_SetModeGraphic3()
{
	ASM_REG_WRITE_INC_BK(modeG3, 0, 10);
}
#endif // USE_VDP_MODE_G3

#if USE_VDP_MODE_G4
//-----------------------------------------------------------------------------
/// Data structure to initialize Graphic 4 screen mode
static const u8 modeG4[] = 
{
	0x06, // R#0
	0x60, // R#1
	0x1F, // R#2
	0xFF, // R#3
	0x03, // R#4
	0xEF, // R#5
	0x0F, // R#6
	0x04, // R#7
	0x08, // R#8
	0x82, // R#9
};
/// Initialize Graphic 4 screen mode. Use MSX 2 incremental VDP registers writing
void VDP_SetModeGraphic4()
{
	ASM_REG_WRITE_INC_BK(modeG4, 0, 10);
}
#endif // USE_VDP_MODE_G4

#if USE_VDP_MODE_G5
//-----------------------------------------------------------------------------
/// Data structure to initialize Graphic 5 screen mode
static const u8 modeG5[] = 
{
	0x08, // R#0
	0x60, // R#1
	0x1F, // R#2
	0xFF, // R#3
	0x03, // R#4
	0xEF, // R#5
	0x0F, // R#6
	0x04, // R#7
	0x08, // R#8
	0x82, // R#9
};
/// Initialize Graphic 5 screen mode. Use MSX 2 incremental VDP registers writing
void VDP_SetModeGraphic5()
{
	ASM_REG_WRITE_INC_BK(modeG5, 0, 10);
}
#endif // USE_VDP_MODE_G5

#if USE_VDP_MODE_G6
//-----------------------------------------------------------------------------
/// Data structure to initialize Graphic 6 screen mode
static const u8 modeG6[] = 
{
	0x0A, // R#0
	0x60, // R#1
	0x1F, // R#2
	0xFF, // R#3
	0x03, // R#4
	0xF7, // R#5
	0x1E, // R#6
	0x04, // R#7
	0x08, // R#8
	0x82, // R#9
};
/// Initialize Graphic 6 screen mode. Use MSX 2 incremental VDP registers writing
void VDP_SetModeGraphic6()
{
	ASM_REG_WRITE_INC_BK(modeG6, 0, 10);
}
#endif // USE_VDP_MODE_G6

#if USE_VDP_MODE_G7
//-----------------------------------------------------------------------------
/// Data structure to initialize Graphic 7 screen mode
static const u8 modeG7[] = 
{
	0x0E, // R#0
	0x60, // R#1
	0x1F, // R#2
	0xFF, // R#3
	0x03, // R#4
	0xF7, // R#5
	0x1E, // R#6
	0x04, // R#7
	0x08, // R#8
	0x82, // R#9
};
/// Initialize Graphic 7 screen mode. Use MSX 2 incremental VDP registers writing
void VDP_SetModeGraphic7()
{
	ASM_REG_WRITE_INC_BK(modeG7, 0, 10);
}
#endif // USE_VDP_MODE_G7

#endif // (MSX_VERSION >= MSX_2)

//-----------------------------------------------------------------------------
//
//   M S X   2 +   F U N C T I O N S
//
//-----------------------------------------------------------------------------






//-----------------------------------------------------------------------------
//
//   C O M M O N   F U N C T I O N S
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//
void VDP_SetScreen(const u8 mode) __FASTCALL
{
	switch(mode)
	{
#if USE_VDP_MODE_T1
	// case VDP_MODE_SCREEN0:
	// case VDP_MODE_SCREEN0_W40:
	case VDP_MODE_TEXT1:
		VDP_SetModeText1();
		return;
#endif // USE_VDP_MODE_T1
	
#if USE_VDP_MODE_MC
	// case VDP_MODE_SCREEN3:
	case VDP_MODE_MULTICOLOR:
		VDP_SetModeMultiColor();
		return;
#endif // USE_VDP_MODE_MC

#if USE_VDP_MODE_G1
	// case VDP_MODE_SCREEN1:
	case VDP_MODE_GRAPHIC1:
		VDP_SetModeGraphic1();
		return;
#endif // USE_VDP_MODE_G1

#if USE_VDP_MODE_G2
	// case VDP_MODE_SCREEN2:
	case VDP_MODE_GRAPHIC2:
		VDP_SetModeGraphic2();
		return;
#endif // USE_VDP_MODE_G2

#if (MSX_VERSION >= MSX_2)

#if USE_VDP_MODE_T2
	// case VDP_MODE_SCREEN0_W80:
	case VDP_MODE_TEXT2:
		VDP_SetModeText2();
		return;
#endif // USE_VDP_MODE_T2²

#if USE_VDP_MODE_G3
	// case VDP_MODE_SCREEN4:
	case VDP_MODE_GRAPHIC3:
		VDP_SetModeGraphic3();
		return;
#endif // USE_VDP_MODE_G3
	
#if USE_VDP_MODE_G4
	// case VDP_MODE_SCREEN5:
	case VDP_MODE_GRAPHIC4:
		VDP_SetModeGraphic4();
		return;
#endif // USE_VDP_MODE_G4
	
#if USE_VDP_MODE_G5
	// case VDP_MODE_SCREEN6:
	case VDP_MODE_GRAPHIC5:
		VDP_SetModeGraphic5();
		return;
#endif // USE_VDP_MODE_G5
	
#if USE_VDP_MODE_G6
	// case VDP_MODE_SCREEN7:
	case VDP_MODE_GRAPHIC6:
		VDP_SetModeGraphic6();
		return;
#endif // USE_VDP_MODE_G6
		
#if USE_VDP_MODE_G7
	// case VDP_MODE_SCREEN8:
	case VDP_MODE_GRAPHIC7:
		VDP_SetModeGraphic7();
		return;
#endif // USE_VDP_MODE_G7
		
#if USE_VDP_MODE_G5
	// case VDP_MODE_SCREEN9:
	case VDP_MODE_SCREEN9_40:
		VDP_SetModeGraphic5();
		// @todo Further setting needed
		return;
#endif // USE_VDP_MODE_G5
	
#if USE_VDP_MODE_G4
	case VDP_MODE_SCREEN9_80:
		VDP_SetModeGraphic4();
		// @todo Further setting needed
		return;
#endif // USE_VDP_MODE_G4

#endif // (MSX_VERSION >= MSX_2)

#if (MSX_VERSION >= MSX_2Plus)

#if USE_VDP_MODE_G7
	case VDP_MODE_SCREEN10:
		VDP_SetModeGraphic7();
		// @todo Further setting needed
		return;

	case VDP_MODE_SCREEN11:
		VDP_SetModeGraphic7();
		// @todo Further setting needed
		return;

	case VDP_MODE_SCREEN12:
		VDP_SetModeGraphic7();
		// @todo Further setting needed
		return;
#endif // USE_VDP_MODE_G7

#endif // (MSX_VERSION >= MSX_2Plus)
	}
}

//-----------------------------------------------------------------------------
// Wait for VBlank flag trigger
/*void VDP_WaitVBlank()
{
	__asm
	WAIT_RETRACE:
		in		a, (P_VDP_STAT)
		// rla
		// jr		nc, WAIT_RETRACE
		// jr		nc, WAIT_RETRACE
		and		#0x80
		cp		#0
		jr		z, WAIT_RETRACE
	__endasm;
}*/

//-----------------------------------------------------------------------------
// Read default S#0 register
u8 VDP_ReadDefaultStatus()
{
	__asm
		in		a, (P_VDP_STAT)
		ld		l, a
	__endasm;
}

//-----------------------------------------------------------------------------
// Read a given status register then reset status register to default (0)
#if (MSX_VERSION >= MSX_2)
u8 VDP_ReadStatus(u8 stat) __FASTCALL
{
	stat;
	// FastCall
	//	ld		l, stat
	__asm
		ld		a, l
		di //~~~~~~~~~~~~~~~~~~~~~~~~~~
		out		(P_VDP_ADDR), a
		ld		a, #VDP_REG(15)
		out		(P_VDP_ADDR), a
		in		a, (P_VDP_STAT)
		ld		l, a				// return value
		// Reset R#15 to default status register S#0
		xor		a           		// ld a, 0
		out		(P_VDP_ADDR), a
		ld		a, #VDP_REG(15)
		ei //~~~~~~~~~~~~~~~~~~~~~~~~~~
		out		(P_VDP_ADDR),a
	__endasm;
}
#endif // (MSX_VERSION >= MSX_2)

#if (MSX_VERSION == MSX_1)
//-----------------------------------------------------------------------------
/// Write data from RAM to VRAM
void VDP_WriteVRAM1(const u8* src, u16 dest, u16 count)
{
	src, dest, count;
	__asm
		push	ix
		ld		ix, #0
		add		ix, sp
		// Setup address register 
		ld		a, 6 (ix)
		ei //~~~~~~~~~~~~~~~~~~~~~~~~~~
		out		(P_VDP_ADDR), a			// RegPort = (dest & 0xFF)
		ld		a, 7 (ix)
		and		a, #0x3f
		add		a, #F_VDP_WRIT
		ei //~~~~~~~~~~~~~~~~~~~~~~~~~~
		out		(P_VDP_ADDR), a			// RegPort = ((dest >> 8) & 0x3F) + F_VDP_WRIT
		// while(count--) DataPort = *src++;
		ld		l, 4 (ix)				// source address
		ld		h, 5 (ix)
		ld		c, #P_VDP_DATA			// data register
		// Handle count LSB
		ld		a, 8 (ix)				// count LSB
		cp		a, #0
		jp		z, wrt_loop_init		// skip LSB
		ld		b, a					// send (count & 0x00FF) bytes
		otir
		// Handle count MSB
	wrt_loop_init:
		ld		a, 9 (ix)				// count MSB
	wrt_loop_start:
		cp		a, #0
		jp		z, wrt_loop_end			// finished
		ld		b, #0					// send 256 bytes packages
		otir
		dec		a
		jp		wrt_loop_start

	wrt_loop_end:
		pop	ix
	__endasm;
}
#else // (MSX_VERSION >= MSX_2)
//-----------------------------------------------------------------------------
/// Write data from RAM to VRAM
void VDP_WriteVRAM2(const u8* src, u16 destLow, u8 destHigh, u16 count)
{
	src, destLow, destHigh, count;
	__asm
		push	ix
		ld		ix, #0
		add		ix, sp
		// Setup address register 
		ld		a, 8 (ix)
		add		a, a
		add		a, a
		ld		c, a
		ld		a, 7 (ix)
		rlca
		rlca
		and		a, #0x03
		add		a, c
		di //~~~~~~~~~~~~~~~~~~~~~~~~~~
		out		(P_VDP_ADDR), a			// RegPort = (page << 2) + (dest >> 14)
		ld		a, #VDP_REG(14)
		out		(P_VDP_REG), a			// RegPort = VDP_REG(14)
		ld		a, 6 (ix)
		out		(P_VDP_ADDR), a			// RegPort = (dest & 0xFF)
		ld		a, 7 (ix)
		and		a, #0x3f
		add		a, #F_VDP_WRIT
		ei //~~~~~~~~~~~~~~~~~~~~~~~~~~
		out		(P_VDP_ADDR), a			// RegPort = ((dest >> 8) & 0x3F) + F_VDP_WRIT
		// while(count--) DataPort = *src++;
		ld		l, 4 (ix)				// source address
		ld		h, 5 (ix)
		ld		c, #P_VDP_DATA			// data register
		// Handle count LSB
		ld		a, 9 (ix)				// count LSB
		cp		a, #0
		jp		z, wrt_loop_init		// skip LSB
		ld		b, a					// send (count & 0x00FF) bytes
		otir
		// Handle count MSB
	wrt_loop_init:
		ld		a, 10 (ix)				// count MSB
	wrt_loop_start:
		cp		a, #0
		jp		z, wrt_loop_end			// finished
		ld		b, #0					// send 256 bytes packages
		otir
		dec		a
		jp		wrt_loop_start

	wrt_loop_end:
		pop	ix
	__endasm;
}
#endif // (MSX_VERSION >= MSX_2)


#if (MSX_VERSION == MSX_1)
//-----------------------------------------------------------------------------
/// Fill VRAM area with a given value
void VDP_FillVRAM1(u8 value, u16 dest, u16 count)
{
	dest, value, count;
	__asm
		push	ix
		ld		ix, #0
		add		ix, sp
		// Setup address register
		ld		a, 5 (ix)
		di //~~~~~~~~~~~~~~~~~~~~~~~~~~
		out		(P_VDP_ADDR), a			// RegPort = (dest & 0xFF);
		ld		a, 6 (ix)
		and		a, #0x3f
		add		a, #F_VDP_WRIT
		ei //~~~~~~~~~~~~~~~~~~~~~~~~~~
		out		(P_VDP_ADDR), a			// RegPort = ((dest >> 8) & 0x3F) + F_VDP_WRIT;
		// while(count--) DataPort = value;
		ld		e, 7 (ix)				// count
		ld		d, 8 (ix)
		ld		a, 4 (ix)				// value
		// fast 16-bits loop
		ld		b, e					// number of loops is in DE
		dec		de						// calculate DB value (destroys B, D and E)
		inc		d
	fll_loop_start:
		out		(P_VDP_DATA), a			// fill
		djnz	fll_loop_start
		dec		d
		jp		nz, fll_loop_start

		pop	ix
	__endasm;
}
#else // (MSX_VERSION >= MSX_2)
//-----------------------------------------------------------------------------
/// Fill VRAM area with a given value
void VDP_FillVRAM2(u8 value, u16 destLow, u8 destHigh, u16 count)
{
	destLow, destHigh, value, count;
	__asm
		push	ix
		ld		ix, #0
		add		ix, sp
		// Setup address register 
		ld		a, 7 (ix)
		add		a, a
		add		a, a
		ld		c, a
		ld		a, 6 (ix)
		rlca
		rlca
		and		a, #0x03
		add		a, c
		di //~~~~~~~~~~~~~~~~~~~~~~~~~~
		out		(P_VDP_ADDR), a			// RegPort = (page << 2) + (dest >> 14);
		ld		a, #VDP_REG(14)
		out		(P_VDP_ADDR), a			// RegPort = VDP_REG(14);
		ld		a, 5 (ix)
		out		(P_VDP_ADDR), a			// RegPort = (dest & 0xFF);
		ld		a, 6 (ix)
		and		a, #0x3f
		add		a, #F_VDP_WRIT
		ei //~~~~~~~~~~~~~~~~~~~~~~~~~~
		out		(P_VDP_ADDR), a			// RegPort = ((dest >> 8) & 0x3F) + F_VDP_WRIT;
		// while(count--) DataPort = value;
		ld		e, 8 (ix)				// count
		ld		d, 9 (ix)
		ld		a, 4 (ix)				// value
		// fast 16-bits loop
		ld		b, e					// number of loops is in DE
		dec		de						// calculate DB value (destroys B, D and E)
		inc		d
	fll_loop_start:
		out		(P_VDP_DATA), a			// fill
		djnz	fll_loop_start
		dec		d
		jp		nz, fll_loop_start

		pop	ix
	__endasm;
}
#endif // (MSX_VERSION >= MSX_2)

#if (MSX_VERSION == MSX_1)
//-----------------------------------------------------------------------------
/// Read data from VRAM to RAM
void VDP_ReadVRAM1(u16 src, u8* dest, u16 count)
{
}
#else // (MSX_VERSION >= MSX_2)
//-----------------------------------------------------------------------------
/// Read data from VRAM to RAM
void VDP_ReadVRAM2(u16 srcLow, u8 srcHigh, u8* dest, u16 count)
{
	srcLow, srcHigh, dest, count;
	__asm
		push	ix
		ld		ix, #0
		add		ix, sp
		// Setup address register 	
		ld		a, 6 (ix)
		add		a, a
		add		a, a
		ld		c, a
		ld		a, 5 (ix)
		rlca
		rlca
		and		a, #0x03
		add		a, c
		di //~~~~~~~~~~~~~~~~~~~~~~~~~~
		out		(P_VDP_ADDR), a			// AddrPort = (srcHigh << 2) + (srcLow >> 14)
		ld		a, #VDP_REG(14)
		out		(P_VDP_REG), a			// RegPort  = (u8)(VDP_REG(14))
		ld		a, 4 (ix)
		out		(P_VDP_ADDR), a			// AddrPort = (srcLow & 0xFF)
		ld		a, 5 (ix)
		and		a, #0x3f
		add		a, #F_VDP_READ
		ei //~~~~~~~~~~~~~~~~~~~~~~~~~~
		out		(P_VDP_ADDR), a			// AddrPort = ((srcLow >> 8) & 0x3F) + F_VDP_READ
		// while(count--) *src++ = DataPort;
		ld		l, 7 (ix)				// source address
		ld		h, 8 (ix)
		ld		c, #P_VDP_DATA			// data register
		// Handle count LSB
		ld		a, 9 (ix)				// count LSB
		cp		a, #0
		jp		z, rd_loop_init			// skip LSB
		ld		b, a					// retreive (count & 0x00FF) bytes
		inir		
		// Handle count MSB		
	rd_loop_init:
		ld		a, 10 (ix)				// count MSB
	rd_loop_start:
		cp		a, #0
		jp		z, rd_loop_end			// finished
		ld		b, #0					// retreive 256 bytes packages
		inir
		dec		a
		jp		rd_loop_start
		
	rd_loop_end:
		pop	ix
	__endasm;
}
#endif // (MSX_VERSION >= MSX_2)

//-----------------------------------------------------------------------------
void VDP_RegWriteFC(u16 reg_value) __FASTCALL
{
	reg_value;
	// FastCall
	//	ld		h, reg
	//	ld		l, value
	__asm
		ld		a, l
		di //~~~~~~~~~~~~~~~~~~~~~~~~~~
		out		(P_VDP_ADDR), a
		ld		a, h
		add		#0x80
		ei //~~~~~~~~~~~~~~~~~~~~~~~~~~
		out		(P_VDP_ADDR), a
	__endasm;
}

//-----------------------------------------------------------------------------
void VDP_RegWriteBakFC(u16 reg_value) __FASTCALL
{
	reg_value;
	// FastCall
	//	ld		h, reg
	//	ld		l, value
	__asm
		ld		a, l
		ld		c, h
		// Backup
		ld		b, #0
		ld		hl, #_g_VDP_REGSAV
		add		hl, bc
		ld		(hl), a
		// Write to register
		di //~~~~~~~~~~~~~~~~~~~~~~~~~~
		out		(P_VDP_ADDR), a
		ld		a, c
		add		#0x80
		ei //~~~~~~~~~~~~~~~~~~~~~~~~~~
		out		(P_VDP_ADDR), a
	__endasm;
}

//-----------------------------------------------------------------------------
// Fast write to VDP register
#define ASM_REG_WRITE(_reg, _val)					\
	__asm											\
		ld		a, _val								\
		di											\
		out		(P_VDP_ADDR), a						\
		ld		a, #VDP_REG(_reg)					\
		ei											\
		out		(P_VDP_ADDR), a						\
	__endasm

//-----------------------------------------------------------------------------
// Fast write to VDP register with backup to RAM
#define ASM_REG_WRITE_BK(_reg, _val)				\
	__asm											\
		ld		a, _val								\
		ld		(_g_VDP_REGSAV + 7), a				\
	__endasm;										\
	ASM_REG_WRITE(_reg, _val)

//-----------------------------------------------------------------------------
// Enable/disable horizontal interruption
void VDP_EnableHBlank(u8 enable) __FASTCALL
{
	u8 reg = g_VDP_REGSAV[0];
	reg &= ~R00_IE1;
	if(enable)
		reg |= R00_IE1;
	VDP_RegWriteBak(0, reg);
}

//-----------------------------------------------------------------------------
//
void VDP_SetHBlankLine(u8 line) __FASTCALL
{
	VDP_RegWrite(19, line);
}

//-----------------------------------------------------------------------------
// 
void VDP_SetVerticalOffset(u8 offset) __FASTCALL
{
	VDP_RegWrite(23, offset);
}

//-----------------------------------------------------------------------------
// Enable/disable vertical interruption
void VDP_EnableVBlank(u8 enable) __FASTCALL
{
	u8 reg = g_VDP_REGSAV[1];
	reg &= ~R01_IE0;
	if(enable)
		reg |= R01_IE0;	
	VDP_RegWriteBak(1, reg);
}

//-----------------------------------------------------------------------------
// Enable/disable screen display
void VDP_EnableDisplay(u8 enable) __FASTCALL
{
	u8 reg = g_VDP_REGSAV[1];
	reg &= ~R01_BL;
	if(enable)
		reg |= R01_BL;	
	VDP_RegWriteBak(1, reg);
}

#if (MSX_VERSION >= MSX_2)
//-----------------------------------------------------------------------------
// Enable/disable grayscale
void VDP_SetGrayScale(u8 enable) __FASTCALL
{
	u8 reg = g_VDP_REGSAV[8];
	reg &= ~R08_BW;
	if(enable)
		reg |= R08_BW;
	VDP_RegWriteBak(8, reg);
}
#endif

#if (MSX_VERSION >= MSX_2)
//-----------------------------------------------------------------------------
// Change VDP frequency
void VDP_SetFrequency(u8 freq) __FASTCALL
{
	u8 reg = g_VDP_REGSAV[9];
	reg &= ~R09_NT;
	reg |= freq;
	VDP_RegWriteBak(9, reg);
}
#endif

//-----------------------------------------------------------------------------
// Set current VRAM page
void VDP_SetPage(u8 page) __FASTCALL
{
	u8 reg = g_VDP_REGSAV[2];
	reg &= 0x9F;
	reg |= page << 5;
	VDP_RegWriteBak(2, reg);
}

//-----------------------------------------------------------------------------
// Set text and border default color
void VDP_SetColor(u8 color) __FASTCALL
{
	VDP_RegWrite(7, color);
}

#if (MSX_VERSION >= MSX_2)
//-----------------------------------------------------------------------------
/// Set a new palette from index 1
/// Format : [red|blue][0|green]
void VDP_SetPalette(const u8* pal) __FASTCALL
{
	pal;
	__asm
//		ld		hl, pal				// FastCall
		ld		a, #1
		di  //~~~~~~~~~~~~~~~~~~~~~~~~~~
		out		(P_VDP_ADDR), a
		ld		a, #VDP_REG(16)
		ei  //~~~~~~~~~~~~~~~~~~~~~~~~~~
		out		(P_VDP_ADDR), a

//		ld		hl, #_g_PlayerSprite_palette
		ld		c, #P_VDP_PAL
		ld		b, #30
		otir
	__endasm;
}
#endif

#if (MSX_VERSION >= MSX_2)
//-----------------------------------------------------------------------------
/// Set a new palette [red|blue][0|green]
void VDP_SetPaletteEntry(u8 index, u16 color)
{
	g_VDP_RegPort = index;
	g_VDP_RegPort = VDP_REG(16);
	g_VDP_PalPort = color & 0x00FF;
	g_VDP_PalPort = color >> 8;
}
#endif

#if (MSX_VERSION >= MSX_2)
//-----------------------------------------------------------------------------
/// Set line count for the current screen mode
void VDP_SetLineCount(u8 lines) __FASTCALL
{
	u8 reg = g_VDP_REGSAV[9];
	reg &= ~R09_LN;
	reg |= lines;
	VDP_RegWriteBak(9, reg);
}
#endif

#if (MSX_VERSION >= MSX_2)
//-----------------------------------------------------------------------------
/// Enable or disable interlace mode
void VDP_SetInterlace(u8 enable) __FASTCALL
{
	u8 reg = g_VDP_REGSAV[9];
	reg &= ~R09_IL;
	if(enable)
		reg |= R09_IL;
	VDP_RegWriteBak(9, reg);
}
#endif

#if (MSX_VERSION >= MSX_2)
//-----------------------------------------------------------------------------
/// Enable automatic page switch on even/odd frames
void VDP_SetPageAlternance(u8 enable) __FASTCALL
{
	u8 reg = g_VDP_REGSAV[9];
	reg &= ~R09_EO;
	if(enable)
		reg |= R09_EO;
	VDP_RegWriteBak(9, reg);
}
#endif

//-----------------------------------------------------------------------------
//
//   S P R I T E S
//
//-----------------------------------------------------------------------------

#if (MSX_VERSION >= MSX_2)
//-----------------------------------------------------------------------------
/// Enable/disable sprite rendering
void VDP_EnableSprite(u8 enable) __FASTCALL
{
	u8 reg = g_VDP_REGSAV[8];
	reg &= ~R08_SPD;
	if(!enable)
		reg |= R08_SPD;
	VDP_RegWriteBak(8, reg);
}
#endif

//-----------------------------------------------------------------------------
/// Set sprite parameters
void VDP_SetSpriteFlag(u8 flag) __FASTCALL
{
	u8 reg = g_VDP_REGSAV[1];
	reg &= ~(R01_ST | R01_MAG);
	if(flag & R01_ST)
		reg |= R01_ST;
	if(flag & R01_MAG)
		reg |= R01_MAG;
	VDP_RegWriteBak(1, reg);
}

//-----------------------------------------------------------------------------
/// Set sprite attribute table address (bit#16 to bit#1)
/// @param		addr		VRAM address (only the lowest 17 bits are used)
void VDP_SetSpriteAttributeTable(u32 addr) __FASTCALL
{
	g_SpriteAtributeHigh = addr >> 8;
	g_SpriteAtributeLow = (u16)addr;
	addr -= 0x200;
	g_SpriteColorHigh = addr >> 8;
	g_SpriteColorLow = (u16)addr;
	
#if (MSX_VERSION == MSX_1)
	u8 reg;
	reg = (u8)(addr >> 7);
	VDP_RegWrite(5, reg);
#else // (MSX_VERSION >= MSX_2)
	u8 reg;
	reg = (u8)(addr >> 7) | 0b111;
	VDP_RegWrite(5, reg);
	reg = (u8)(addr >> 15);
	VDP_RegWrite(11, reg);
#endif
}

//-----------------------------------------------------------------------------
/// Set sprite pattern table address
/// @param		addr		1-bit right shifted VRAM address (bit#16 to bit#1)
void VDP_SetSpritePatternTable(u32 addr) __FASTCALL __naked
{
	__asm
//		ld		de, addr.hi			// FastCall
//		ld		hl, addr.lo			// FastCall
		ld		(#_g_SpritePatternLow), hl	// g_SpritePatternLow = (u16)addr;
		ld		a, h
		ld		hl, #_g_SpritePatternHigh	// g_SpritePatternHigh = addr >> 8;
		ld		(hl), e
		sra		e							// put bit#0 in carry
		rra									// shift right and get carry to bit#7
		sra		a							// shift right
		sra		a							// shift right
		ld		l, a						// set register value
		ld		a, #0x06
		ld		h, a						// set register id
		jp		_VDP_RegWriteFC
	__endasm;

	/*g_SpritePatternHigh = addr >> 8;
	g_SpritePatternLow  = (u16)addr;
	u8 reg = addr >> 11;
	VDP_RegWrite(6, reg);*/
}

//-----------------------------------------------------------------------------
/// Set sprite table address (bit#16 to bit#1)
/// @param		pattern		1-bit right shifted VRAM address (bit#16 to bit#1)
/// @param		attrib		1-bit right shifted VRAM address (bit#16 to bit#1)
void VDP_SetSpriteTables(u32 pattern, u32 attrib)
{
	VDP_SetSpritePatternTable(pattern);
	VDP_SetSpriteAttributeTable(attrib);
}

//-----------------------------------------------------------------------------
/// Load pattern data into VRAM
void VDP_LoadSpritePattern(const u8* addr, u8 index, u8 count)
{
	u16 low = g_SpritePatternLow;
	low += (index * 8);
	VDP_WriteVRAM(addr, low, g_SpritePatternHigh, count * 8);
}

//-----------------------------------------------------------------------------
/// Set sprite attribute for Sprite Mode 1 (MSX1)
void VDP_SetSpriteSM1(u8 index, u8 x, u8 y, u8 shape, u8 color)
{
	g_VDP_Sprite.Y = y;				// Y coordinate on screen (all lower priority sprite will be disable if equal to 216 or 0xD0)
    g_VDP_Sprite.X = x;				// X coordinate of the sprite
    g_VDP_Sprite.Pattern = shape;	// Pattern index
    g_VDP_Sprite.Color = color;		// Color index (Sprite Mode 1 only) + Early clock

	u16 low = g_SpriteAtributeLow;
	low += (index * 4);
	VDP_WriteVRAM((u8*)&g_VDP_Sprite, low, g_SpriteAtributeHigh, 4);
}

//-----------------------------------------------------------------------------
/// Set sprite attribute for Sprite Mode 2
void VDP_SetSprite(u8 index, u8 x, u8 y, u8 shape)
{
	g_VDP_Sprite.Y = y;				// Y coordinate on screen (all lower priority sprite will be disable if equal to 216 or 0xD0)
    g_VDP_Sprite.X = x;				// X coordinate of the sprite
    g_VDP_Sprite.Pattern = shape;	// Pattern index

	u16 low = g_SpriteAtributeLow;
	low += (index * 4);
	VDP_WriteVRAM((u8*)&g_VDP_Sprite, low, g_SpriteAtributeHigh, 3);
}

//-----------------------------------------------------------------------------
/// Set sprite attribute for Sprite Mode 2 and fill color table with color data
void VDP_SetSpriteExMultiColor(u8 index, u8 x, u8 y, u8 shape, const u8* ram)
{
	u16 col = g_SpriteColorLow;
	col += (index * 16);
	VDP_WriteVRAM(ram, col, g_SpriteColorHigh, 16);

	g_VDP_Sprite.X = x;				// Y coordinate on screen (all lower priority sprite will be disable if equal to 216 or 0xD0)
	g_VDP_Sprite.Y = y;				// X coordinate of the sprite
	g_VDP_Sprite.Pattern = shape;	// Pattern index
	u16 attr = g_SpriteAtributeLow;
	attr += (index * 4);
	VDP_WriteVRAM((u8*)&g_VDP_Sprite, attr, g_SpriteAtributeHigh, 3);
}

//-----------------------------------------------------------------------------
/// Set sprite attribute for Sprite Mode 2 and fill color table with unique color
void VDP_SetSpriteExUniColor(u8 index, u8 x, u8 y, u8 shape, u8 color)
{
	u16 col = g_SpriteColorLow;
	col += (index * 16);
	VDP_FillVRAM(color, col, g_SpriteColorHigh, 16);

	g_VDP_Sprite.X = x;				// Y coordinate on screen (all lower priority sprite will be disable if equal to 216 or 0xD0)
	g_VDP_Sprite.Y = y;				// X coordinate of the sprite
	g_VDP_Sprite.Pattern = shape;	// Pattern index
	u16 attr = g_SpriteAtributeLow;
	attr += (index * 4);
	VDP_WriteVRAM((u8*)&g_VDP_Sprite, attr, g_SpriteAtributeHigh, 3);
}

//-----------------------------------------------------------------------------
/// Update sprite position
void VDP_SetSpritePosition(u8 index, u8 x, u8 y)
{
	g_VDP_Sprite.Y = y;				// Y coordinate on screen (all lower priority sprite will be disable if equal to 216 or 0xD0)
    g_VDP_Sprite.X = x;				// X coordinate of the sprite

	u16 low = g_SpriteAtributeLow;
	low += (index * 4);
	VDP_WriteVRAM((u8*)&g_VDP_Sprite, low, g_SpriteAtributeHigh, 2);
}

//-----------------------------------------------------------------------------
/// Update sprite pattern
void VDP_SetSpritePattern(u8 index, u8 shape)
{
    g_VDP_Sprite.Pattern = shape;	// Pattern index

	u16 low = g_SpriteAtributeLow + 2;
	low += (index * 4);
	VDP_WriteVRAM((u8*)&g_VDP_Sprite.Pattern, low, g_SpriteAtributeHigh, 1);
}

//-----------------------------------------------------------------------------
/// Update sprite pattern (Shader mode 1)
void VDP_SetSpriteColorSM1(u8 index, u8 color)
{
    g_VDP_Sprite.Color = color;	// Color index (Sprite Mode 1 only) + Early clock

	u16 low = g_SpriteAtributeLow + 3;
	low += (index * 4);
	VDP_WriteVRAM((u8*)&g_VDP_Sprite.Pattern, low, g_SpriteAtributeHigh, 1);
}

//-----------------------------------------------------------------------------
/// Update sprite color (Uni-color)
void VDP_SetSpriteUniColor(u8 index, u8 color)
{
	u16 col = g_SpriteColorLow;
	col += (index * 16);
	VDP_FillVRAM(color, col, g_SpriteColorHigh, 16);
}

//-----------------------------------------------------------------------------
/// Update sprite color (Multi-color)
void VDP_SetSpriteMultiColor(u8 index, const u8* ram)
{
	u16 col = g_SpriteColorLow;
	col += (index * 16);
	VDP_WriteVRAM(ram, col, g_SpriteColorHigh, 16);	
}

//-----------------------------------------------------------------------------
///
void VDP_SetSpriteData(u8 index, const u8* data)
{
	u16 low = g_SpriteAtributeLow;
	low += (index * 4);
	VDP_WriteVRAM(data, low, g_SpriteAtributeHigh, 3);
}

//-----------------------------------------------------------------------------
///
void VDP_HideSpriteFrom(u8 index) __FASTCALL
{
	g_VDP_Sprite.Y = 216;
 	u16 low = g_SpriteAtributeLow;
	low += (index * 4);
	VDP_WriteVRAM((u8*)&g_VDP_Sprite, low, g_SpriteAtributeHigh, 1);
}


/*
//-----------------------------------------------------------------------------
/// Set sprite attribute
void VDP_SendSpriteAttribute(u8 index) __FASTCALL
{
	VDP_CommandWait();
	__asm
		// Compute sprite attribute VRAM address
		ld		a, l			// Sprite index
		rlca
		rlca
		ld		hl, (#M_ATRBAS)
		add		a, l
		ld		l, a
		adc		a, h 
		sub		l 
		ld		h, a 
		
		ld		hl, #(_g_VDP_Sprite)
		ld		c, #P_VDP_DATA 
		di
		outi
		outi
		outi
		ei
		outi
	__endasm;
}
*/

//-----------------------------------------------------------------------------
//
//   V D P   C O M M A N D S
//
//-----------------------------------------------------------------------------

#if (MSX_VERSION >= MSX_2)
//-----------------------------------------------------------------------------
// Wait for previous VDP command to be finished
void VDP_CommandWait()
{
	__asm
	wait_vdp_ready:
		// Set current status ragister to S#2
		ld		a, #2
		di //~~~~~~~~~~~~~~~~~~~~~~~~~~
		out		(P_VDP_REG), a
		ld		a, #VDP_REG(15)
		out		(P_VDP_REG), a
		in		a, (P_VDP_STAT)		// get S#2 value
		rra							// check CE (bit#0)
		// Reset current status register to S#0 before enabling interruptions
		ld		a, #0				// don't use XOR here to keep C flag from RRA alive
		out		(P_VDP_REG), a
		ld		a, #VDP_REG(15)
		ei //~~~~~~~~~~~~~~~~~~~~~~~~~~
		out		(P_VDP_REG), a
		jp		c, wait_vdp_ready	// CE==1 ? command not finished
	__endasm;
}

//-----------------------------------------------------------------------------
/// Send VDP command (form registres 32 to 46)
void VPD_CommandSetupR32()
{
	VDP_CommandWait();
	ASM_REG_WRITE_INC(g_VDP_Command, 32, 15);
}

//-----------------------------------------------------------------------------
/// Send VDP command (form registres 36 to 46)
void VPD_CommandSetupR36()
{
	VDP_CommandWait();
	ASM_REG_WRITE_INC(g_VDP_Command + 4, 36, 11);
}

//-----------------------------------------------------------------------------
/// Write to VRAM command loop
void VPD_CommandWriteLoop(const u8* addr) __FASTCALL
{
	addr;
	
	__asm
 //		ld		hl, address			// FastCall
		// Set indirect register write to R#44
		ld  	a, #VDP_REG(44)
		di //~~~~~~~~~~~~~~~~~~~~~~~~~~
		out 	(P_VDP_REG), a
		ld  	a, #VDP_REG(17)
		out 	(P_VDP_REG), a		
		// Set current status ragister to S#2
		ld  	a, #2
		out 	(P_VDP_REG), a
		ld  	a, #VDP_REG(15)
		out 	(P_VDP_REG), a
		// Setup outi loop (value of register B don't matter)
		inc 	hl
		ld		c, #P_VDP_IREG
	write_loop:
		// Read S#2 to check CE flag (no need to check TR (bit#7) while write loop is longer than worse case VDP write duration (~29cc)
		in  	a, (P_VDP_STAT)
		rra							// check CE (bit#0)
		jp		nc, write_finished	// CE==0 ? command finished
		outi						// write a byte from HL to port VDP_IREG
		jp		write_loop

	write_finished:
		// Reset current status register to S#0
		xor 	a	
		out 	(P_VDP_REG), a
		ld  	a, #VDP_REG(15)
		ei //~~~~~~~~~~~~~~~~~~~~~~~~~~
		out 	(P_VDP_REG), a
	__endasm;
}
#endif // (MSX_VERSION >= MSX_2)
