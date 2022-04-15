//-----------------------------------------------------------------------------------//
//                            //Kukoo2 21 years anniversary\\                        //
//...................................................................................//
// Kukoo2 Advance - old school GBA port Project of 1994's Kukoo2 BBS VGA intro       //
// (c) 2002-2015 - Laurent Lardinois (TypeOne / TFL-TDV)                             //
// Started at SOTA 2002 (Roubaix - France) using VisualHAM                           //
// Rewritten with devkitpro at Evoke 2014 (Köln - Germany)                           //
// Released at Evoke 2015 (Köln - Germany)                                           //
//-----------------------------------------------------------------------------------//


#include <math.h>
#include <stdlib.h>


#include "tonc_bios.h"
#include "tonc_core.h"
#include "tonc_memdef.h"
#include "tonc_memmap.h"
#include "tonc_types.h"


#include "krawall.h"
#include "modules.h"
#include "samples.h"


// Graphics Includes

// bubble-tile sprite and map
#include "gfx/bulle_64x64.map.c"
#include "gfx/bulle_64x64.pal.c"
#include "gfx/bulle_64x64_Tiles.c"


// text-tile pattern and map
#include "gfx/PATTERN.map.c"
#include "gfx/PATTERN.pal.c"
#include "gfx/PATTERN.raw.c"


// chess perspective map
#include "gfx/damrol1.map.c"

// dark flamoots logo
#include "gfx/tfltdv240x160_8.pal.c"
#include "gfx/tfltdv240x160_8Bitmap.c"

// happy end screen
#include "gfx/happyend_evoke2015.c"

// splash screen
#include "gfx/gba_240x160_juice.c"

// anniversary
#include "gfx/Kukoo2Anniversary_title.c"

// Witan scrolltext font
#include "gfx/Witan.pal.c"
#include "gfx/Witan_Bitmap.c"
unsigned char* Witan_Bitmap = NULL;
const size_t Witan_BitmapSz = 25296;


// Each refresh consists of a 160 scanline vertical draw (VDraw) period
// followed by a 68 scanline blank (VBlank) period

#define NB_HBL_SCANLINE 228
#define NB_BLANK_SCAN 68

#define HBL_OFFSET 160
#define DAMIER_OFFSET (32 * 12)

#define HORIZ_RES 240
#define VERTI_RES 160

#define SPRITE_FONT_PALETTE_OFFSET (255 - (32 + 4 + 1) + 1)
#define FNT_CHAR_SIZE (16 * 17)


// debugging
// http://www.pouet.net/topic.php?which=8556
// you can output stuff to the VBA console using swi 0xff (don't leave that in ROMs intended to run on a real GBA)
// https://www.mtholyoke.edu/courses/dstrahma/cs221/lectures/presentation11a_files/frame.htm

/////////////////////////////////////////////////////////////////////////////
// G L O B A L   V A R I A B L E S

volatile u16 g_bNewFrame = 0;
volatile u16 g_bNewHbl = 0;

volatile u8 g_bVBlankEnabled = 1;

volatile u8 g_EnableTextRotate = 0;
volatile u8 g_EnableTextZoom = 0;
volatile u8 g_EnableTextSineWave = 0;
volatile u8 g_EnableTextVerticalShift = 0;
volatile u8 g_EnableTextQuickSpeed = 0;
volatile u8 g_EnableTextMaxSpeed = 0;
volatile u8 g_EnableChessColorCycling = 0;
volatile u8 g_EnableChessBoardSine = 0;
volatile u8 g_EnableChessBoardMove = 0;
volatile u8 g_EnableVerticalCopperBars = 0;
volatile u8 g_EnableHorizontalCopperBars = 0;
volatile u8 g_EnableSecondCopperBar = 0;
volatile u8 g_EnableTwistCopperBars = 0;
volatile u8 g_EnableTwistCopperBars2 = 0;
volatile u8 g_EnableBubbleSprites = 0;
volatile u8 g_StartFadeOut = 0;

volatile u16 g_ScrollFntCircularBufferIndex[16] __attribute__((aligned(2)))
__attribute__((section(".sbss"))); // used for scrolltext and fnt sprite allocation

// chess palette
// 1st color index = 256-37-16
// color cycling = modulo 16*3
const u8 g_Chess_Pal[] = { 32, 32, 32, 40, 40, 40, 48, 48, 48, 56, 56, 56, 48, 48, 48, 40, 40, 40, 32, 32, 32, 24, 24, 24, 0, 0, 32, 0, 0,
    40, 0, 0, 48, 0, 0, 56, 0, 0, 48, 0, 0, 40, 0, 0, 32, 0, 0, 24, 32, 32, 32, 40, 40, 40, 48, 48, 48, 56, 56, 56, 48, 48, 48, 40, 40, 40,
    32, 32, 32, 24, 24, 24, 0, 0, 32, 0, 0, 40, 0, 0, 48, 0, 0, 56, 0, 0, 48, 0, 0, 40, 0, 0, 32, 0, 0, 24 };

const u16 g_Chess_BaseColor = 203; /** (256-37-16) **/
const u16 g_Chess_Period = 16;
volatile u16 g_Chess_ColorOffset = 0;

// rotational-tile map (0 is reserved for an empty 8x8 tile)
const u8 COPPER_Map[32]
    = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32 };


// blue background
u16* g_Raster1 = NULL; //[66] __attribute__((aligned (2)));
const size_t g_Raster1Sz = 66;

// red and green background
u16* g_Raster2 = NULL; //[13*8] __attribute__((aligned (2)));
const size_t g_Raster2Sz = 13 * 8;

// generation equivalent of INCLUDE multi2.xy
u8* g_Multi = NULL; //[512 * 2] __attribute__((section(".ewram"))); // x and y coordinates
const size_t g_MultiSz = 512 * 2;

const u8 g_BarsEven[] __attribute__((aligned(2)))
= { 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81,
      82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
      112, 113, 114, 115, 116, 117, 118, 119, 104, 105, 106, 107, 108, 109, 110, 111, 96, 97, 98, 99, 100, 101, 102, 103, 88, 89, 90, 91,
      92, 93, 94, 95, 80, 81, 82, 83, 84, 85, 86, 87, 72, 73, 74, 75, 76, 77, 78, 79, 64, 65, 66, 67, 68, 69, 70, 71, 56, 57, 58, 59, 60,
      61, 62, 63, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
      144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170,
      171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 176, 177, 178, 179, 180, 181,
      182, 183, 168, 169, 170, 171, 172, 173, 174, 175, 160, 161, 162, 163, 164, 165, 166, 167, 152, 153, 154, 155, 156, 157, 158, 159, 144,
      145, 146, 147, 148, 149, 150, 151, 136, 137, 138, 139, 140, 141, 142, 143, 128, 129, 130, 131, 132, 133, 134, 135 };

const u8 g_BarsOdd[] __attribute__((aligned(2)))
= { 0, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80,
      81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110,
      111, 112, 113, 114, 115, 116, 117, 118, 119, 104, 105, 106, 107, 108, 109, 110, 111, 96, 97, 98, 99, 100, 101, 102, 103, 88, 89, 90,
      91, 92, 93, 94, 95, 80, 81, 82, 83, 84, 85, 86, 87, 72, 73, 74, 75, 76, 77, 78, 79, 64, 65, 66, 67, 68, 69, 70, 71, 56, 57, 58, 59,
      60, 61, 62, 63, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142,
      143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169,
      170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 176, 177, 178, 179, 180,
      181, 182, 183, 168, 169, 170, 171, 172, 173, 174, 175, 160, 161, 162, 163, 164, 165, 166, 167, 152, 153, 154, 155, 156, 157, 158, 159,
      144, 145, 146, 147, 148, 149, 150, 151, 136, 137, 138, 139, 140, 141, 142, 143, 128, 129, 130, 131, 132, 133, 134, 135 };

// INCLUDE copper2.xx
// generated output same as INCLUDE copper2.xx
short int* g_Copper = NULL;         //[512 * 2] __attribute__((aligned (2))) __attribute__((section(".ewram")));
const size_t g_CopperSz = 1024 * 2; // 512 * 2;

// cylinders on blue background
// generated ouput same as INCLUDE rolax.y
u16* g_Rolax = NULL; //[256*6] __attribute__((aligned (2))) __attribute__((section(".ewram")));
const size_t g_RolaxSz = 256 * 6;

// blue waves in half-screen
// generated output same as INCLUDE vague.y
u16* g_Vague = NULL; //[512 + 8*4] __attribute__((aligned (2))) __attribute__((section(".ewram")));
const size_t g_VagueSz = 512 + 8 * 4;

// INCLUDE ampli.tbl
// generated output same as INCLUDE amply.tbl
short int* g_Ampli = NULL;         //[512*2] __attribute__((aligned (2))) __attribute__((section(".ewram")));
const size_t g_AmpliSz = 1024 * 2; // 512*2;


const char Scroll1[] = {
    "BBS intro by *TYPE{ONE*. great chiptune by BZL(cro). runs on real hw or emu. "
    "greetings:CRO,WITAN,PANDACUBE,RGBA,ASPIRINE,ANTARES,SMASH{DESIGNS,HOODLUM,ORANGE,MAJIC{12,VIBRANTS,HAUJOBB,FAIRLIGHT,RAZOR{1911,TRSI,"
    "S!P,TCB,ULM,THE{LOST{BOYS,NEXT,MELON{DESIGN,FARBRAUSCH,THE{BLACK{LOTUS,BOMB,TPOLM,ECLIPSE,IMPHOBIA,NOON,COCOON,OXYGENE,BOOZE{DESIGN,"
    "KEFRENS,IGUANA,TRITON,KLOON,PULPE,SCOOPEX,ARCHIMA,THE{REPLICANTS,CHANNEL{38,SPACEBALLS,MAHONEY&KAKTUS,FUTURE{CREW,RENAISSANCE,TRB. a "
    "big kiss to my wife and my little boy. be crazy.enjoy and keep the scene alive!"

    //         "BBS intro by TYPE{ONE. runs on real hw or emu.
    //         greetings:PANDACUBE,RGBA,ASPIRINE,ANTARES,SMASH{DESIGNS,HOODLUM,ORANGE,MAJIC{12,VIBRANTS,HAUJOBB,FAIRLIGHT,RAZOR{1911,TRSI,S!P,TCB,ULM,THE{LOST{BOYS,NEXT,MELON{DESIGN,FARBRAUSCH,THE{BLACK{LOTUS,BOMB,TPOLM,ECLIPSE,IMPHOBIA,NOON,COCOON,OXYGENE,BOOZE{DESIGN,KEFRENS,IGUANA,TRITON,KLOON,PULPE,SCOOPEX,ARCHIMA,WITAN,THE{REPLICANTS,CHANNEL{38,SPACEBALLS,MAHONEY&KAKTUS,FUTURE{CREW,RENAISSANCE,TRB.
    //         a big kiss to my wife and my little boy. saludos:
    //         unix,wizard,scorpik,roudoudou,nomad,wally,bladerunner,cola,z,drealmer,pl,rez,made,darkness,moebius,matt,vatin,access,katana,sam,magic
    //         fred,mesh .....and all others met at
    //         WIRED{94,SIH{95,MEKKA-SYMPOSIUM96,WIRED{97,WIRED{98,SATURNE{98,THE{PARTY{98,SOTA{2002,SOTA{2004,BREAKPOINT{2010,EVOKE{2014
    //         and EVOKE{2015! be crazy.enjoy and keep the scene alive!"

};

/** lookup table for copper xcoord and tile offset **/
u32* g_LookupXcoord = NULL; //[244 * 2] __attribute__((aligned (4))); // gonfler a *2 pour memleak ?
const size_t g_LookupXcoordSz = 244 * 2;

u8* g_Couleurs = NULL; //[62*3*4 + 63*4 + 4*4 + 12*3*4 + 35*2*4 + 5*3*4 + 1*4] /*__attribute__((section(".ewram")))*/;
const size_t g_CouleursSz = 62 * 3 * 4 + 63 * 4 + 4 * 4 + 12 * 3 * 4 + 35 * 2 * 4 + 5 * 3 * 4 + 1 * 4;
// s16 g_SinCosTable[1024 + 512] __attribute__((aligned (2))) /*__attribute__((section(".ewram")))*/; // store both sinus and cosines (cos a
// = sin (a+Pi/2))
s16* g_SinCosTable = NULL;
const size_t g_SinCosTableSz = 1024 + 512;


u16 g_ScrollTextAngleZ = 0;
u16 g_ScrollTextAngleY = 0;
u16 g_ScrollTextRotaAngle = 0;
s16 g_ScrollTextSineAmpli = 0;

u16 g_PatternAngleX = 0;
u16 g_PatternAngleX2 = 0;

u16 g_BulleTimeAngle = 0;
u16 g_BulleTimeAngle2 = 0;
u16 g_BulleTimeAngle3 = 0;
u16 g_BulleModAngleX = 0;
u16 g_BulleModAngleY = 0;


// raster buffers
u16* g_RasterPage1 = NULL; //[NB_HBL_SCANLINE] __attribute__((aligned (4)));
u16* g_RasterPage2 = NULL; //[NB_HBL_SCANLINE] __attribute__((aligned (4)));

const u16* g_VaguePTR = NULL;
const u16* g_RolaxPTR = NULL;

volatile u16* g_BgPage1 = NULL;
volatile u16* g_BgPage2 = NULL;

volatile u16* g_RasterPage = NULL;
volatile u16* g_OffRasterPage = NULL;
volatile u16* g_HBLRasterPtr = NULL;

volatile u16 g_ScanCounter = 0;

volatile unsigned int g_MultiIndex = 0;

u16 g_prevAmpIndex[2] __attribute__((aligned(4))) = { 0, 256 };
u16 g_AmpIndex[2] __attribute__((aligned(4))) = { 0, 256 }; /** copper amplitude **/
volatile int g_prevBarIndex = 0;
volatile int g_BarIndex = 0; /** copper bar ptr **/
volatile int g_BarColorIndex = 0;

u8* g_CheckerBoardTmpBuffer = NULL;

volatile unsigned long g_HeapEwram = MEM_EWRAM + EWRAM_SIZE;

/////////////////////////////////////////////////////////////////////////////
// Memory buffer pre-alloc

void InitMemBufferAlloc(void)
{
    // #define	EWRAM		0x02000000
    // #define	EWRAM_END	0x02040000

    // #define IWRAM_CODE	__attribute__((section(".iwram"), long_call))
    // #define EWRAM_CODE	__attribute__((section(".ewram"), long_call))

    // #define IWRAM_DATA	__attribute__((section(".iwram")))
    // #define EWRAM_DATA	__attribute__((section(".ewram")))
    // #define EWRAM_BSS	__attribute__((section(".sbss")))


    // Witan_Bitmap = (unsigned char*) memalign(4, Witan_BitmapSz * sizeof(unsigned char));
    g_HeapEwram -= (Witan_BitmapSz * sizeof(unsigned char) + 4) & ~3;
    Witan_Bitmap = (unsigned char*)(g_HeapEwram);

    g_HeapEwram -= (g_CouleursSz * sizeof(u8) + 4) & ~3;
    g_Couleurs = (u8*)g_HeapEwram; //(u8*) malloc(g_CouleursSz * sizeof(u8));

    g_HeapEwram -= (g_LookupXcoordSz * sizeof(u32) + 4) & ~3;
    g_LookupXcoord = (u32*)g_HeapEwram; //(u32*) memalign(4, g_LookupXcoordSz * sizeof(u32));

    g_HeapEwram -= (g_SinCosTableSz * sizeof(s16) + 4) & ~3;
    g_SinCosTable = (s16*)g_HeapEwram; //(s16*) memalign(2, g_SinCosTableSz * sizeof(s16));

    g_HeapEwram -= (NB_HBL_SCANLINE * sizeof(u16) + 4) & ~3;
    g_RasterPage1 = (u16*)g_HeapEwram; //(u16*) memalign(4, NB_HBL_SCANLINE * sizeof(u16));
    g_HeapEwram -= (NB_HBL_SCANLINE * sizeof(u16) + 4) & ~3;
    g_RasterPage2 = (u16*)g_HeapEwram; //(u16*) memalign(4, NB_HBL_SCANLINE * sizeof(u16));

    g_HeapEwram -= (g_AmpliSz * sizeof(short int) + 4) & ~3;
    g_Ampli = (short int*)g_HeapEwram; //(short int*) memalign(2, g_AmpliSz * sizeof(short int));

    g_HeapEwram -= (g_VagueSz * sizeof(u16) + 4) & ~3;
    g_Vague = (u16*)g_HeapEwram; //(u16*) memalign(2, g_VagueSz * sizeof(u16));

    g_HeapEwram -= (g_RolaxSz * sizeof(u16) + 4) & ~3;
    g_Rolax = (u16*)g_HeapEwram; //(u16*) memalign(2, g_RolaxSz * sizeof(u16));

    g_HeapEwram -= (g_CopperSz * sizeof(short int) + 4) & ~3;
    g_Copper = (short int*)g_HeapEwram; //(short int*) memalign(2, g_CopperSz * sizeof(short int));

    g_HeapEwram -= (g_MultiSz * sizeof(u8) + 4) & ~3;
    g_Multi = (u8*)g_HeapEwram; //(u8*) malloc(g_MultiSz * sizeof(u8));

    g_HeapEwram -= (g_Raster1Sz * sizeof(u16) + 4) & ~3;
    g_Raster1 = (u16*)g_HeapEwram; //(u16*) memalign(2, g_Raster1Sz * sizeof(u16));
    g_HeapEwram -= (g_Raster2Sz * sizeof(u16) + 4) & ~3;
    g_Raster2 = (u16*)g_HeapEwram; //(u16*) memalign(2, g_Raster2Sz * sizeof(u16));

    g_HeapEwram -= (210 * 64 + 4) & ~3; //(240*56 + 4) & ~3;
    g_CheckerBoardTmpBuffer = (u8*)g_HeapEwram;
}


/////////////////////////////////////////////////////////////////////////////
// Algorithms


#if 0
u16 FakeRandomGen(void)
{
     // derived from coded Posted on UseNet in 1994 by Jare/IGUANA

     static u16 NextNumber = 0xF972;
     register u16 tmp;

     tmp = NextNumber + 0x9248;
     tmp = (tmp << 3) | (tmp >> 13);
     NextNumber = tmp;

     return tmp & 0x7FFF;
}
#endif


// plot in BG x following existing map and 8x8 tiles
void TilePixel(int x0, int y0, u8 color, int cbb, int sbb, int bgOffset)
{
    u16 tileIndex = damrol1_Map[((x0 >> 3) + (y0 >> 3) * 30) % 210];

    u8* tileBase = g_CheckerBoardTmpBuffer;

    tileBase[(tileIndex << 6) + ((y0 & 7) << 3) + (x0 & 7)] = color;
}

u8 GetTilePixel(int x0, int y0, int cbb, int sbb, int bgOffset)
{

    u16 tileIndex = damrol1_Map[((x0 >> 3) + (y0 >> 3) * 30) % 210];

    u8* tileBase = g_CheckerBoardTmpBuffer;

    return tileBase[(tileIndex << 6) + ((y0 & 7) << 3) + (x0 & 7)];
}

void BresenhamLine(int x0, int y0, int x1, int y1, int clipX, int clipY, u8 color, int cbb, int sbb, int bgOffset)
{


    // http://rosettacode.org/wiki/Bitmap/Bresenham%27s_line_algorithm

    int dx = abs(x1 - x0), sx = (x0 < x1) ? 1 : -1;
    int dy = abs(y1 - y0), sy = (y0 < y1) ? 1 : -1;
    int err = ((dx > dy) ? dx : -dy) >> 1;
    int e2;

    for (;;)
    {
        if ((x0 >= 0) && (x0 < clipX) && (y0 >= 0) && (y0 < clipY))
        {
            TilePixel(x0, y0, color, cbb, sbb, bgOffset);
        }


        if ((x0 == x1) && (y0 == y1))
            break;
        e2 = err;
        if (e2 > -dx)
        {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dy)
        {
            err += dx;
            y0 += sy;
        }
    }
}

// damrol is in BG1 (stored in CBB2) with BG1 map in SBB4
// zone of 240x56 pixels built from 8x8 tiles
// base color = 256 - 37 - 16
void CheckerBoardPattern(void)
{
    int a = 0;
    int oy = 0;
    int p;
    int y, x;
    int fx;
    u8 color;

    y = 56;

    for (x = 0; x < 240; ++x)
    {
        color = ((x + a) & 15) + g_Chess_BaseColor;

        BresenhamLine(x, oy, ((x - 120) << 1) + 120, y, 240, 56, color, 2 /*cbb*/, 4 /*sbb*/, DAMIER_OFFSET + 32);
    }

    u8 prevColor = 0;
    for (y = 0; y < 56; ++y)
    {
        color = GetTilePixel(0, y, 2, 4, DAMIER_OFFSET + 32);
        if (0 == color)
        {
            color = GetTilePixel(1, y, 2, 4, DAMIER_OFFSET + 32);
        }

        for (x = 0; x < 240; ++x)
        {
            prevColor = color;
            color = GetTilePixel(x, y, 2, 4, DAMIER_OFFSET + 32);
            if (0 == color)
            {
                TilePixel(x, y, prevColor, 2, 4, DAMIER_OFFSET + 32);
            }
        }
    }

    oy = 0;
    for (p = 2; p < 11; ++p)
    {
        y = (p * p);

        // shift color on horizontal line
        for (x = oy; x < ((y < 56) ? y : 56); ++x)
        {
            for (fx = 0; fx < 240; ++fx)
            {
                color = GetTilePixel(fx, x, 2, 4, DAMIER_OFFSET + 32);
                TilePixel(fx, x, (((color - g_Chess_BaseColor) + a) & 15) + g_Chess_BaseColor, 2, 4, DAMIER_OFFSET + 32);
            }
        }

        a = a ^ 8;
        oy = y;
    }
}

void InsertionSort(u16* keyArray, u16* eltArray, u16 arrayLength)
{
    // http://mycodinglab.com/insertion-sort-algorithm/

    u16 i, j, tmp;

    for (i = 1; i < arrayLength; ++i)
    {
        j = i;

        while ((j > 0) && (keyArray[j - 1] > keyArray[j]))
        {
            tmp = keyArray[j];
            keyArray[j] = keyArray[j - 1];
            keyArray[j - 1] = tmp;

            tmp = eltArray[j];
            eltArray[j] = eltArray[j - 1];
            eltArray[j - 1] = tmp;

            --j;
        } // end of while loop
    }     // end for
}

///////////////////////////////////////////////////
// Mandelbrot/Julia pixel
void MandelJuliaPixel(int p_nbPixels)
{
    // http://nuclear.mutantstargoat.com/articles/sdr_fract/
    // http://forum.6502.org/viewtopic.php?f=2&t=2243
    // http://en.wikipedia.org/wiki/Julia_set
    // http://www.karlsims.com/julia.html


    // http://www.youtube.com/watch?v=Afosk6o6NmE
    // To generate, you iterate on the formula z = z*z + c. If the absolute value ever
    // exceeds 2, it will eventually trend towards infinity. The black area is where you
    //  never reach 2 and the colors are based on how many iterations it takes to reach 2.
    // The difficult part is that z and c are complex numbers with x being the real part
    // and y being the imaginary part (z = x + iy). To get z*z, the new x is x*x-y*y and
    // the new z is 2*x*y. c is the original (x + iy), so add the original x to x and y to y.

    // http://fractilefractals.files.wordpress.com/2010/12/colorchooser.jpg


    static u16* videoBuffer = (u16*)(MEM_VRAM);
    static u16 videoOffset = 0; // 240*6;
    static s16 px = 0;
    static s16 py = 0;

#define RGBPAL(r, g, b) ((b << 10) | (g << 5) | r)

    static const u16 fractalPal[] = { RGBPAL(0, 0, 1), RGBPAL(0, 0, 2), RGBPAL(0, 0, 3), RGBPAL(0, 0, 4), RGBPAL(0, 0, 5), RGBPAL(0, 0, 6),
        RGBPAL(0, 0, 7), RGBPAL(0, 0, 8), RGBPAL(0, 0, 9), RGBPAL(0, 0, 10), RGBPAL(0, 0, 11), RGBPAL(0, 0, 12), RGBPAL(0, 0, 13),
        RGBPAL(0, 0, 14), RGBPAL(0, 0, 15), RGBPAL(0, 0, 16), RGBPAL(0, 0, 17), RGBPAL(0, 0, 18), RGBPAL(0, 0, 19), RGBPAL(0, 0, 20),
        RGBPAL(0, 0, 21), RGBPAL(0, 0, 22), RGBPAL(0, 0, 23), RGBPAL(0, 0, 24), RGBPAL(0, 0, 25), RGBPAL(0, 0, 26), RGBPAL(0, 0, 27),
        RGBPAL(0, 0, 28), RGBPAL(0, 0, 29), RGBPAL(0, 0, 30), RGBPAL(0, 0, 31), RGBPAL(1, 0, 30), RGBPAL(2, 0, 29), RGBPAL(3, 0, 28),
        RGBPAL(4, 0, 27), RGBPAL(5, 0, 26), RGBPAL(6, 0, 25), RGBPAL(7, 0, 24), RGBPAL(8, 0, 23), RGBPAL(9, 0, 22), RGBPAL(10, 0, 21),
        RGBPAL(11, 0, 20), RGBPAL(12, 0, 19), RGBPAL(13, 0, 18), RGBPAL(14, 0, 17), RGBPAL(15, 0, 16), RGBPAL(16, 0, 15), RGBPAL(17, 0, 14),
        RGBPAL(18, 0, 13), RGBPAL(19, 0, 12), RGBPAL(20, 0, 11), RGBPAL(21, 0, 10), RGBPAL(22, 0, 9), RGBPAL(23, 0, 8), RGBPAL(24, 0, 7),
        RGBPAL(25, 0, 6), RGBPAL(26, 0, 5), RGBPAL(27, 0, 4), RGBPAL(28, 0, 3), RGBPAL(29, 0, 2), RGBPAL(30, 0, 1), RGBPAL(31, 0, 0),
        RGBPAL(30, 1, 0), RGBPAL(29, 2, 0), RGBPAL(28, 3, 0), RGBPAL(27, 4, 0), RGBPAL(26, 5, 0), RGBPAL(25, 6, 0), RGBPAL(24, 7, 0),
        RGBPAL(23, 8, 0), RGBPAL(22, 9, 0), RGBPAL(21, 10, 0), RGBPAL(20, 11, 0), RGBPAL(19, 12, 0), RGBPAL(18, 13, 0), RGBPAL(17, 14, 0),
        RGBPAL(16, 15, 0), RGBPAL(15, 16, 0), RGBPAL(14, 17, 0), RGBPAL(13, 18, 0), RGBPAL(12, 19, 0), RGBPAL(11, 20, 0), RGBPAL(10, 21, 0),
        RGBPAL(9, 22, 0), RGBPAL(8, 23, 0), RGBPAL(7, 24, 0), RGBPAL(6, 25, 0), RGBPAL(5, 26, 0), RGBPAL(4, 27, 0), RGBPAL(3, 28, 0),
        RGBPAL(2, 29, 0), RGBPAL(1, 30, 0), RGBPAL(0, 31, 0), RGBPAL(0, 31, 1), RGBPAL(0, 31, 2), RGBPAL(0, 31, 3), RGBPAL(0, 31, 4),
        RGBPAL(0, 31, 5), RGBPAL(0, 31, 6), RGBPAL(0, 31, 7), RGBPAL(0, 31, 8), RGBPAL(0, 31, 9), RGBPAL(0, 31, 10), RGBPAL(0, 31, 11),
        RGBPAL(0, 31, 12), RGBPAL(0, 31, 13), RGBPAL(0, 31, 14), RGBPAL(0, 31, 15), RGBPAL(0, 31, 16), RGBPAL(0, 31, 17), RGBPAL(0, 31, 18),
        RGBPAL(0, 31, 19), RGBPAL(0, 31, 20), RGBPAL(0, 31, 21), RGBPAL(0, 31, 22), RGBPAL(0, 31, 23), RGBPAL(0, 31, 24), RGBPAL(0, 31, 25),
        RGBPAL(0, 31, 26), RGBPAL(0, 31, 27), RGBPAL(0, 31, 28), RGBPAL(0, 31, 29), RGBPAL(0, 31, 30), RGBPAL(0, 31, 31)


    };

    u16 color, colorCombo;

    s64 zx, zy, cx, cy, x, y;
    register unsigned int i, j, k;
    const s64 ceilDist = (((const s64)4) << 48);

    // Z = x + y.i   and i^2 = -1
    // Z = Z^2 + C

    cx = -0.039f * (1 << 24);
    cy = 0.695f * (1 << 24);

    for (j = 0; (j < p_nbPixels) & (py <= 159); ++j)
    {

        color = videoBuffer[videoOffset];
        colorCombo = color;

        if (0 == color)
        {
            zx = 3 * (((px - 120) << 24) / 240); // 3.0 * [-0.5, 0.5]
            zy = 2 * (((py - 80) << 24) / 160);  // 2.0 * [-0.5, 0.5]

            for (i = 0; i < 26; ++i)
            {
                x = ((zx * zx - zy * zy) >> 24) + cx;
                y = ((zy * zx + zx * zy) >> 24) + cy;

                if (((x * x + y * y)) > ceilDist)
                    break;

                zx = x;
                zy = y;

                ++i;

                x = ((zx * zx - zy * zy) >> 24) + cx;
                y = ((zy * zx + zx * zy) >> 24) + cy;

                if (((x * x + y * y)) > ceilDist)
                    break;

                zx = x;
                zy = y;
            }

            videoBuffer[videoOffset] = fractalPal[8 + (i << 1) + i];
        }

        ++videoOffset;

        if (0 == videoBuffer[videoOffset])
        {
            videoBuffer[videoOffset] = videoBuffer[videoOffset - 1];
        }

        ++videoOffset;


        px += 2;

        if (px >= 240)
        {
            kramWorker();

            py += 2;
            px = 0;

            for (k = 0; k < 240; ++k)
            {
                if (0 == videoBuffer[videoOffset + k])
                {
                    videoBuffer[videoOffset + k] = (((videoBuffer[videoOffset - 241 + k] + videoBuffer[videoOffset - 240 + k]) >> 1)
                                                       + videoBuffer[videoOffset - 239 + k])
                        >> 1;
                }
            }

            videoOffset += 240;
        }

        kramWorker();

    } // end nb pixels
}

/////////////////////////////////////////////////////////////////////////////
// Pre-calculate tables

void Init_Tables_part1(void)
{
    int i, j;
    float angle;
    const float Pi = 3.141599f;
    const float twoPi = 2.0f * Pi;
    u16* videoBuffer = (u16*)(MEM_VRAM);
    u16 videoOffset = 0;

    // init sin/cos table

    for (i = 0; i < (1024 + 512); ++i)
    {
        angle = (i * twoPi) / 512.0f;

        float sinAngle = sin(angle);
        g_SinCosTable[i] = (s16)(sinAngle * 256.0f);
    }


    // init pattern multi2.xy table
    for (i = 0, j = 0; i < 512; ++i, j += 2)
    {
        angle = (i * twoPi) / 512.0f;

        g_Multi[j] = (u8)((s16)120 + (s16)(120 * cos(4.0f * angle)));    // 240 pixels wide
        g_Multi[j + 1] = (u8)((s16)190 + (s16)(40 * sin(5.0f * angle))); // 160 pixels height
    }

    // init vague table
    for (i = 0; i < (512 + 8 * 4); ++i)
    {
        angle = (i * twoPi) / 512.0f;

        // min value 67, max value 108
        g_Vague[i] = 87 + 21 * sin(5.0f * angle) * sin(2.0f * angle);
    }
}

void Init_Tables_part2(void)
{
    int i, j;
    float angle;
    const float Pi = 3.141599f;
    const float twoPi = 2.0f * Pi;
    u16* videoBuffer = (u16*)(MEM_VRAM);
    u16 videoOffset = 0;


    // init rolling roll table
    u16 sx[6], sy[6];
    u16 rollOffset = 0;
    for (i = 0; i < 256; ++i)
    {
        angle = (i * twoPi) / 256.0f;

        for (j = 0; j < 6; ++j)
        {
            // min value = 2, max value = 57
            sx[j] = (u16)(55 + cos(2 * angle + (twoPi * j * 13) / 360.0f) * 55); // rolling depth
            sy[j] = (u16)(29.5f + 0.5f * sin(2 * angle + (twoPi * j * 13) / 360.0f) * 55.0f);
        } // end for j

        InsertionSort(sx, sy, 6); // depth sort

        for (j = 0; j < 6; ++j)
        {
            g_Rolax[rollOffset++] = sy[j];
        }

    } // end for i


    // short int g_Ampli[1024];
    // min -220, max 237
    // 44*16 + 10 + 17 = 731
    // g_Copper[1024 * 2];
    // min -31187, max 32767
    for (i = 0; i < 1024; ++i)
    {
        angle = (i * twoPi) / 1024.0f;
        g_Ampli[i] = (short int)(8.5f + 228.5f * (sin(4.0f * angle) * cos(5.0f * angle)));
        g_Copper[i] = (short int)(cos(2.0f * angle) * 32767);
        g_Ampli[i + 1024] = g_Ampli[i];
        g_Copper[i + 1024] = g_Copper[i];
    }
}


/////////////////////////////////////////////////////////////////////////////
// Clear Screen

void Clear_Screen(void)
{
    u8 i, j;
    unsigned long long* Addr = (unsigned long long*)MEM_VRAM;

    for (j = 0; j < 2; ++j)
    {
        // Clear each scanline
        for (i = 0; i < 160; ++i)
        {
            *Addr++ = 0;
            *Addr++ = 0;
            *Addr++ = 0;
            *Addr++ = 0;
            *Addr++ = 0;
            *Addr++ = 0;
            *Addr++ = 0;
            *Addr++ = 0;
            *Addr++ = 0;
            *Addr++ = 0;
            *Addr++ = 0;
            *Addr++ = 0;
            *Addr++ = 0;
            *Addr++ = 0;
            *Addr++ = 0;
            *Addr++ = 0;
            *Addr++ = 0;
            *Addr++ = 0;
            *Addr++ = 0;
            *Addr++ = 0;
            *Addr++ = 0;
            *Addr++ = 0;
            *Addr++ = 0;
            *Addr++ = 0;
            *Addr++ = 0;
            *Addr++ = 0;
            *Addr++ = 0;
            *Addr++ = 0;
            *Addr++ = 0;
            *Addr++ = 0;
        }

        kramWorker();

        Addr += 0xA000;
    }
}


// initialize palette (get it from pattern table)
void Init_Palette(void)
{
    unsigned int i;

    // Initialize the background palette
    for (i = 0; i < sizeof(PATTERN_Palette); ++i)
    {
        *((u16*)(MEM_PAL_BG + i * 2)) = PATTERN_Palette[i];
    }

    // cycling chess palette

    for (i = 0; i < (sizeof(g_Chess_Pal) / 3); i++)
    {
        *((u16*)(MEM_PAL_BG + (g_Chess_BaseColor + i) * 2))
            = (((u16)g_Chess_Pal[i * 3 + 2] >> 1) << 10) | (((u16)g_Chess_Pal[i * 3 + 1] >> 1) << 5) | ((u16)g_Chess_Pal[i * 3] >> 1);
    }

    kramWorker();
}


// fill background with pattern (36x64)
void Put_Pattern(void)
{
    int i, j, k, l, m;

    // http://www.coranac.com/tonc/text/regbg.htm
    //
    // Both the tiles and tilemaps are stored in VRAM, which is divided into charblocks and screenblocks.
    // The tileset is stored in the charblocks and the tilemap goes into the screenblocks.
    //
    // Memory 	0600:0000 	0600:4000 	0600:8000 	0600:C000
    // charblock 	0 	1 	2 	3
    // screenblock 	0 	… 	7 	8 	… 	15 	16 	… 	23 	24 	… 	31

    // 240 x 160 pixels

    /* BG0 (pattern) 64x32 tiles = 512x256, no rotation */
    /* BG1 (copper) 32x32 tiles = 256x256, with rotation */
    /* BG2 (damier cycle) 32x32 tiles = 256x256, no rotation */

    // Load BG0 tiles into CBB 0
    memcpy16(&tile8_mem[0][1], PATTERN_Tiles, sizeof(PATTERN_Tiles) >> 1);

    kramWorker();


    // Load BG0 map into SBB 1

    // 64x32 bloc is made of two 32x32 blocs

    u16* tilePtr = &se_mem[1][0];

    j = 0;

    for (l = 0; l < 5; ++l)
    {
        for (k = 0; k < 5; ++k)
        {
            for (i = 0; i < 3; ++i)
            {
                tilePtr[0 + i * 32 + j + k * 6] = PATTERN_Map[0 + i * 3];
                tilePtr[1 + i * 32 + j + k * 6] = PATTERN_Map[1 + i * 3];
                tilePtr[2 + i * 32 + j + k * 6] = PATTERN_Map[2 + i * 3];

                tilePtr[0 + 2 + i * 32 + j + k * 6 + 32 * 32 - 1 + 3 * 32] = PATTERN_Map[0 + i * 3];
                tilePtr[1 + 2 + i * 32 + j + k * 6 + 32 * 32 - 1 + 3 * 32] = PATTERN_Map[1 + i * 3];
                tilePtr[2 + 2 + i * 32 + j + k * 6 + 32 * 32 - 1 + 3 * 32] = PATTERN_Map[2 + i * 3];
            }
        }

        // last column before second 32x32 tile bloc
        for (i = 0; i < 3; ++i)
        {
            tilePtr[0 + i * 32 + j + 5 * 6] = PATTERN_Map[0 + i * 3];
            tilePtr[1 + i * 32 + j + 5 * 6] = PATTERN_Map[1 + i * 3];
            tilePtr[32 * 32 + i * 32 + j] = PATTERN_Map[2 + i * 3];
        }


        j += 32 * 3;

        for (k = 0; k < 5; ++k)
        {
            for (i = 0; i < 3; ++i)
            {
                tilePtr[3 + 0 + i * 32 + j + k * 6] = PATTERN_Map[0 + i * 3];
                tilePtr[3 + 1 + i * 32 + j + k * 6] = PATTERN_Map[1 + i * 3];
                tilePtr[3 + 2 + i * 32 + j + k * 6] = PATTERN_Map[2 + i * 3];

                if (j < 32 * 3 * 9) // last line of 2nd 32x32 bloc - repeat at beginning of 2nd 32x32 bloc
                {
                    tilePtr[3 + 0 + 2 + i * 32 + j + k * 6 + 32 * 32 - 1 + 3 * 32] = PATTERN_Map[0 + i * 3];
                    tilePtr[3 + 1 + 2 + i * 32 + j + k * 6 + 32 * 32 - 1 + 3 * 32] = PATTERN_Map[1 + i * 3];
                    tilePtr[3 + 2 + 2 + i * 32 + j + k * 6 + 32 * 32 - 1 + 3 * 32] = PATTERN_Map[2 + i * 3];
                }
                else
                {
                    tilePtr[3 + 0 + 2 + i * 32 + k * 6 + 32 * 32 - 1] = PATTERN_Map[0 + i * 3];
                    tilePtr[3 + 1 + 2 + i * 32 + k * 6 + 32 * 32 - 1] = PATTERN_Map[1 + i * 3];
                    tilePtr[3 + 2 + 2 + i * 32 + k * 6 + 32 * 32 - 1] = PATTERN_Map[2 + i * 3];
                }
            }
        }

        j += 32 * 3;
    }


    // Load BG2 map into SBB 3
    // 240 pixels / 8 = 30 tiles
    memcpy16(&se_mem[3][0], COPPER_Map, sizeof(COPPER_Map) >> 1);

    // Load BG1 tiles into CBB 2
    // (30*8) x (7*8) + 8x8 = 240 x 56 = 240x56 pic built from 8x8 tiles
    kramWorker();
    memcpy16(&tile8_mem[2][0], g_CheckerBoardTmpBuffer, (13440) >> 1);


    // Load BG1 map into SBB 4

    for (i = 0; i < 32; ++i)
    {
        se_mem[4][DAMIER_OFFSET + i] = 0;
    }

    for (k = 0; k < 7; ++k)
    {
        for (i = 0; i < 30; ++i)
        {
            se_mem[4][DAMIER_OFFSET + 32 + i + k * 32] = damrol1_Map[i + k * 30];
        }
    }

    kramWorker();
}


// fill sprites (fonts 16x17)
void Put_Sprites(void)
{
    int i, j, k, l, m;

    // init sprite font palette

    // http://www.coranac.com/tonc/text/objbg.htm
    // sprites have their own palette which starts at 0500:0200h (right after the background
    // palette).

    for (i = 0; i < SPRITE_FONT_PALETTE_OFFSET; ++i)
    {
        *((u16*)(MEM_PAL_OBJ + i * 2)) = bulle_64x64_Palette[i];
    }

    for (i = 0; i < sizeof(Witan_Palette) / 3; ++i)
    {
        // witan fnt palette is in 8 bits/component > shift to 5 bits/component
        *((u16*)(MEM_PAL_OBJ + (i + SPRITE_FONT_PALETTE_OFFSET) * 2)) = (((u16)(Witan_Palette[i * 3 + 2] >> 3)) << 10)
            | (((u16)(Witan_Palette[i * 3 + 1] >> 3)) << 5) | ((u16)(Witan_Palette[i * 3] >> 3));
    }

    // init sprite font bitmap
    // Load Font bitmap into higher block CBB 4 (tile id 0 - 511)
    kramWorker();
    LZ77UnCompWram(packed_Witan_Bitmap, Witan_Bitmap);

    // 20 x 5 - 6 characters = 94 chars = 94 regular sprites
    int nbFontSprites = Witan_BitmapSz / (16 * 17); // start from space bar (code 32)
    u8* tileSprPtr = &tile8_mem[4][0];
    u16* tileSprPtrW = tileSprPtr;

    u16 pixel;

    // 240 / 16 = 15 + 1 sprites to handle font scroll text
    for (i = 0; i < 16; ++i)
    {
        // 16 x (16+1) pixels
        // = 4* 8x8 tiles + skip 1 vertical pixel

        // 16x16 sprite = 4 tiles de 8x8

        // fill 16 sprites with space character

        // top-left
        // 8 scanlines of 8 pixels
        for (j = 0; j < 8; ++j)
        {
            tileSprPtrW[i * 16 * 8 + j * 4 + 0]
                = (u16)(Witan_Bitmap[0 * FNT_CHAR_SIZE + j * 16 + 0]) | ((u16)(Witan_Bitmap[0 * FNT_CHAR_SIZE + j * 16 + 1])) << 8;
            tileSprPtrW[i * 16 * 8 + j * 4 + 1]
                = (u16)(Witan_Bitmap[0 * FNT_CHAR_SIZE + j * 16 + 2]) | ((u16)(Witan_Bitmap[0 * FNT_CHAR_SIZE + j * 16 + 3])) << 8;
            tileSprPtrW[i * 16 * 8 + j * 4 + 2]
                = (u16)(Witan_Bitmap[0 * FNT_CHAR_SIZE + j * 16 + 4]) | ((u16)(Witan_Bitmap[0 * FNT_CHAR_SIZE + j * 16 + 5])) << 8;
            tileSprPtrW[i * 16 * 8 + j * 4 + 3]
                = (u16)(Witan_Bitmap[0 * FNT_CHAR_SIZE + j * 16 + 6]) | ((u16)(Witan_Bitmap[0 * FNT_CHAR_SIZE + j * 16 + 7])) << 8;
        }

        // top-right
        // 8 scanlines of 8 pixels
        for (j = 0; j < 8; ++j)
        {
            tileSprPtrW[i * 16 * 8 + (j + 8) * 4 + 0]
                = (u16)Witan_Bitmap[0 * FNT_CHAR_SIZE + j * 16 + 8] | (u16)Witan_Bitmap[0 * FNT_CHAR_SIZE + j * 16 + 9] << 8;
            tileSprPtrW[i * 16 * 8 + (j + 8) * 4 + 1]
                = (u16)Witan_Bitmap[0 * FNT_CHAR_SIZE + j * 16 + 10] | (u16)Witan_Bitmap[0 * FNT_CHAR_SIZE + j * 16 + 11] << 8;
            tileSprPtrW[i * 16 * 8 + (j + 8) * 4 + 2]
                = (u16)Witan_Bitmap[0 * FNT_CHAR_SIZE + j * 16 + 12] | (u16)Witan_Bitmap[0 * FNT_CHAR_SIZE + j * 16 + 13] << 8;
            tileSprPtrW[i * 16 * 8 + (j + 8) * 4 + 3]
                = (u16)Witan_Bitmap[0 * FNT_CHAR_SIZE + j * 16 + 14] | (u16)Witan_Bitmap[0 * FNT_CHAR_SIZE + j * 16 + 15] << 8;
        }

        // bottom-left
        // 8 scanlines of 8 pixels
        for (j = 0; j < 8; ++j)
        {
            tileSprPtrW[i * 16 * 8 + (j + 16) * 4 + 0]
                = (u16)Witan_Bitmap[0 * FNT_CHAR_SIZE + (j + 8) * 16 + 0] | (u16)Witan_Bitmap[0 * FNT_CHAR_SIZE + (j + 8) * 16 + 1] << 8;
            tileSprPtrW[i * 16 * 8 + (j + 16) * 4 + 1]
                = (u16)Witan_Bitmap[0 * FNT_CHAR_SIZE + (j + 8) * 16 + 2] | (u16)Witan_Bitmap[0 * FNT_CHAR_SIZE + (j + 8) * 16 + 3] << 8;
            tileSprPtrW[i * 16 * 8 + (j + 16) * 4 + 2]
                = (u16)Witan_Bitmap[0 * FNT_CHAR_SIZE + (j + 8) * 16 + 4] | (u16)Witan_Bitmap[0 * FNT_CHAR_SIZE + (j + 8) * 16 + 5] << 8;
            tileSprPtrW[i * 16 * 8 + (j + 16) * 4 + 3]
                = (u16)Witan_Bitmap[0 * FNT_CHAR_SIZE + (j + 8) * 16 + 6] | (u16)Witan_Bitmap[0 * FNT_CHAR_SIZE + (j + 8) * 16 + 7] << 8;
        }

        // bottom-right
        // 8 scanlines of 8 pixels
        for (j = 0; j < 8; ++j)
        {
            tileSprPtrW[i * 16 * 8 + (j + 24) * 4 + 0]
                = (u16)Witan_Bitmap[0 * FNT_CHAR_SIZE + (j + 8) * 16 + 8] | (u16)Witan_Bitmap[0 * FNT_CHAR_SIZE + (j + 8) * 16 + 9] << 8;
            tileSprPtrW[i * 16 * 8 + (j + 24) * 4 + 1]
                = (u16)Witan_Bitmap[0 * FNT_CHAR_SIZE + (j + 8) * 16 + 10] | (u16)Witan_Bitmap[0 * FNT_CHAR_SIZE + (j + 8) * 16 + 11] << 8;
            tileSprPtrW[i * 16 * 8 + (j + 24) * 4 + 2]
                = (u16)Witan_Bitmap[0 * FNT_CHAR_SIZE + (j + 8) * 16 + 12] | (u16)Witan_Bitmap[0 * FNT_CHAR_SIZE + (j + 8) * 16 + 13] << 8;
            tileSprPtrW[i * 16 * 8 + (j + 24) * 4 + 3]
                = (u16)Witan_Bitmap[0 * FNT_CHAR_SIZE + (j + 8) * 16 + 14] | (u16)Witan_Bitmap[0 * FNT_CHAR_SIZE + (j + 8) * 16 + 15] << 8;
        }

        kramWorker();
    }

    // store bubble 64x64 sprite
    LZ77UnCompVram(packed_bulle_64x64_Tiles, &tileSprPtrW[16 * 16 * 8]);
    kramWorker();


    // http://www.coranac.com/tonc/text/regobj.htm

    // declare OAM sprite objects

    // OBJ_ATTR or OBJ_AFFINE
    // 4 OBJ_ATTR structures and lay them over one OBJ_AFFINE structure
    // room for 128 OBJ_ATTR structures and 32 OBJ_AFFINEs

    // affine sprites
    // http://www.coranac.com/tonc/text/affobj.htm

    // 128 OBJ ATTR
    OBJ_ATTR* obj_buffer = (OBJ_ATTR*)MEM_OAM;
    OBJ_AFFINE* obj_aff_buffer = (OBJ_AFFINE*)obj_buffer;

    // http://www.coranac.com/tonc/text/regobj.htm

    for (i = 0; i < 16; ++i)
    {
        // http://www.coranac.com/tonc/text/regobj.htm#tbl-obj-size
        obj_buffer[i].attr0 = ATTR0_Y(120) | ATTR0_AFF | ATTR0_8BPP | ATTR0_SQUARE | ATTR0_BLEND; // normal rendering, 256 col,
        obj_buffer[i].attr1 = ATTR1_X(i * 16) | ATTR1_SIZE(1) | ATTR1_AFF_ID(i);                  // 16x16 square sprite
        // 1 sprite = 4 tiles de 8x8 x 2 ?
        obj_buffer[i].attr2 = ATTR2_ID(i * 8) | ATTR2_PRIO(1); //  	Base tile-index of sprite, the higher priority are drawn first.
        obj_buffer[i].fill = 0;

        g_ScrollFntCircularBufferIndex[i] = i * 8; // index on corresponding tile block
    }

    for (i = 16; i < 26; ++i)
    {
        // http://www.coranac.com/tonc/text/regobj.htm#tbl-obj-size
        obj_buffer[i].attr0 = ATTR0_Y(60) | ATTR0_AFF | ATTR0_8BPP | ATTR0_SQUARE | ATTR0_BLEND; // normal rendering, 256 col,
        obj_buffer[i].attr1 = ATTR1_X((i - 16) * 16) | ATTR1_SIZE(3) | ATTR1_AFF_ID(i);          // 64x64 square sprite
        // 1 sprite = 4 tiles de 8x8 x 2 ?
        obj_buffer[i].attr2 = ATTR2_ID(16 * 8) | ATTR2_PRIO(0); //  	Base tile-index of sprite, the higher priority are drawn first.
        obj_buffer[i].fill = 0;
    }

    kramWorker();

    for (i = 26; i < 128; ++i)
    {
        obj_buffer[i].attr0 = ATTR0_HIDE; // don't display
    }

    // http://www.coranac.com/tonc/text/affobj.htm
    // set affine transformations -  origin of the transformation is center of the sprite
    // pa pb
    // pc pd
    // 8.8 fixed point numbers that form the actual matrix

    // If all you are after is a simple scale-then-rotate matrix, try this: for a zoom by sx and sy
    // followed by a counter-clockwise rotation by α, the correct matrix is this:
    // cos(a)/sx  -sin(a)/sx
    // sin(a)/sy  cos(a)/sy

    for (i = 0; i < 32; ++i)
    {
        // identity matrices
        obj_aff_buffer[i].pa = (s16)1 << 8;
        obj_aff_buffer[i].pb = 0;
        obj_aff_buffer[i].pc = 0;
        obj_aff_buffer[i].pd = (s16)1 << 8;
    }
}


/////////////////////////////////////////////////////////////////////////////
// Init raster colors

void Init_Rasters(void)
{
    u16 i, j;
    unsigned int k, l;

    u16* p1;
    u8* pb;

    // blue raster background
    p1 = g_Raster1;

    *p1++ = 0;
    *p1++ = 0;
    for (i = 0; i <= 63; i++)
    {
        *p1++ = ((i >> 1) << 10) | ((i >> 2) << 5);
    }
    // red and green raster background
    p1 = g_Raster2;

    for (k = 0, j = 0; k < 13; k++, j += 5)
    {
        for (l = 0, i = 0; l < 8; l++, i += 4)
        {
            *p1++ = i | (j << 5);
        }
    }

    //
    pb = g_Couleurs;

    for (i = 0; i < 62; i++)
    {
        *pb++ = 2;
        *pb++ = 0;
        *pb++ = i >> 1;
        *pb++ = i;
        *pb++ = 2;
        *pb++ = 0;
        *pb++ = i >> 1;
        *pb++ = i + 1;
        *pb++ = 2;
        *pb++ = 0;
        *pb++ = i >> 1;
        *pb++ = i;
    }

    for (i = 0; i < 63; i++)
    {
        *pb++ = 0;
        *pb++ = i;
        *pb++ = 0;
        *pb++ = 0;
    }

    for (i = 0; i < 4; i++)
    {
        *pb++ = 0;
        *pb++ = 0;
        *pb++ = 0;
        *pb++ = i;
    }

    for (i = 0; i < 24; i += 2)
    {
        *pb++ = 0;
        *pb++ = 0;
        *pb++ = 0;
        *pb++ = i;
        *pb++ = 0;
        *pb++ = 0;
        *pb++ = 0;
        *pb++ = i;
        *pb++ = 0;
        *pb++ = 0;
        *pb++ = 0;
        *pb++ = i + 1;
    }

    for (i = 0; i < 35; i++)
    {
        *pb++ = 0;
        *pb++ = 0;
        *pb++ = 0;
        *pb++ = i;
        *pb++ = 0;
        *pb++ = 0;
        *pb++ = 0;
        *pb++ = i;
    }

    i--;
    for (j = 0; j < 5; j++)
    {
        *pb++ = 0;
        *pb++ = 0;
        *pb++ = 0;
        *pb++ = i;
        *pb++ = 0;
        *pb++ = 0;
        *pb++ = 0;
        *pb++ = i;
        *pb++ = 0;
        *pb++ = 0;
        *pb++ = 0;
        *pb++ = i;
    }
    *pb++ = 0;
    *pb++ = 0;
    *pb++ = 0;
    *pb++ = 0;


    //
    g_BgPage1 = &g_Raster1[0];
    g_BgPage2 = &g_Raster1[2];


    // init pointer

    g_VaguePTR = g_Vague;
    g_RolaxPTR = g_Rolax;

    g_RasterPage = g_RasterPage1;
    g_OffRasterPage = g_RasterPage2;

    g_HBLRasterPtr = g_RasterPage;


    // build vertical copper bars lookup table for xcoord

    /** lookup table for copper xcoord and tile offset **/
    for (i = 0; i < (244 * 2); i++) // gonfler a *2 pour memleak ?
    {
        g_LookupXcoord[i] = ((i >> 2) << 5) + (i & 0x3);
    }
}


//------------------------------------
// Vectorbulle routine
//------------------------------------

void VectorBulleUpdate()
{
    register int i;

    // update position and scale for each sprite

    // sympa
    // http://www.math.uri.edu/~bkaskosz/flashmo/parcur/
    // x = cos(2*t)*sin(t+Pi)
    // y = sin(3*t + Pi)
    // z = cos(t)*sin(3*t)

    // 128 OBJ ATTR
    OBJ_ATTR* obj_buffer = (OBJ_ATTR*)MEM_OAM;
    OBJ_AFFINE* obj_aff_buffer = (OBJ_AFFINE*)obj_buffer;

    s16 x, y, z, p, modX, modY;
    const u16 cosOffset = 128; // Pi/2 (512 mapped on TwoPi)
    const u16 PiOffset = 256;
    //   s16 xCoord, yCoord, scale;
    u16 step;

    u16 sId[10], sZ[10];
    s16 xCoord[10], yCoord[10], scale[10];


    // position update + bubble matrix update
    for (i = 16, step = 0; i < 26; ++i, step += 16)
    {
        sId[i - 16] = i;

        x = (g_SinCosTable[g_BulleTimeAngle2 + cosOffset + step] * g_SinCosTable[g_BulleTimeAngle + PiOffset + step]) >> 8;
        y = g_SinCosTable[g_BulleTimeAngle3 + PiOffset + step];
        z = (g_SinCosTable[g_BulleTimeAngle + cosOffset + step] * g_SinCosTable[g_BulleTimeAngle3 + step]) >> 8;
        p = (1 << 8) + z;

        sZ[i - 16] = p;

        modX = g_SinCosTable[g_BulleModAngleX + (i << 2)];
        modY = g_SinCosTable[g_BulleModAngleY + (i << 2)];
        x = x + (x * modX) >> 8;
        y = y + (y * modY) >> 8;

        xCoord[i - 16] = (100 + ((120 * x) / p)) & 0x1ff;
        yCoord[i - 16] = (30 + ((50 * y) / p)) & 0xff;

        scale[i - 16] = 600 + z;
    }

    // depth sort
    InsertionSort(sZ, sId, 10);

    // update pos by draw priority

    u16 id;

    for (i = 0; i < 10; ++i)
    {
        id = sId[i];

        // http://www.coranac.com/tonc/text/regobj.htm#tbl-obj-size
        obj_buffer[i + 16].attr0
            = ATTR0_Y(yCoord[id - 16]) | ATTR0_AFF | ATTR0_8BPP | ATTR0_SQUARE | ATTR0_BLEND;   // normal rendering, 256 col,
        obj_buffer[i + 16].attr1 = ATTR1_X(xCoord[id - 16]) | ATTR1_SIZE(3) | ATTR1_AFF_ID(id); // 64x64 square sprite
        // 1 sprite = 4 tiles de 8x8 x 2 ?
        // Base tile-index of sprite, the higher priority are drawn

        obj_aff_buffer[i + 16].pa = scale[id - 16];
        obj_aff_buffer[i + 16].pd = scale[id - 16];
    }
}


// some triggers

void ScrollTextTriggers(int scrollTextOffset)
{
    switch (scrollTextOffset)
    {
        case 10:
            g_EnableChessColorCycling = 1;
            g_EnableTextQuickSpeed = 0;
            g_EnableTextMaxSpeed = 0;
            break;

        case 20:
            g_EnableHorizontalCopperBars = 1;
            break;

        case 35:
            g_EnableBubbleSprites = 1;
            break;

        case 50:
            g_EnableChessBoardMove = 1;
            break;

        case 70:
            g_EnableVerticalCopperBars = 1;
            break;

        case 95:
            g_EnableTextSineWave = 1;
            break;

        case 120:
            g_EnableSecondCopperBar = 1;
            g_EnableChessBoardSine = 1;
            break;

        case 135:
            g_EnableTextVerticalShift = 1;
            break;

        case 150:
            g_EnableTextQuickSpeed = 1;
            g_EnableTextZoom = 1;
            break;

        case 200:
            g_EnableTwistCopperBars = 1;
            break;

        case 250:
            g_EnableTextMaxSpeed = 1;
            g_EnableTextQuickSpeed = 0;
            g_EnableTextRotate = 1;
            break;

        case 280:
            g_EnableTextMaxSpeed = 0;
            g_EnableTextQuickSpeed = 1;
            g_EnableTwistCopperBars2 = 1;
            g_EnableTwistCopperBars = 0;
            break;

        case 360:
            g_StartFadeOut = 1;
            break;

            // 200 chars ~the end

        default:
            break;
    }
}

//------------------------------------
// ScrollText routine
//------------------------------------


void ScrollTextUpdate()
{
    static int scrollPosition = 0;
    static int scrollTextOffset = 0;
    register int i, j, k;

    if (g_EnableTextQuickSpeed)
    {
        scrollPosition -= 2;
    }
    else if (g_EnableTextMaxSpeed)
    {
        scrollPosition -= 4;
    }
    else
    {
        scrollPosition--;
    }

    if (scrollPosition < -15)
    {

        // Reset scrollPosition
        scrollPosition = 0;


        u8* tileSprPtr = &tile8_mem[4][0];
        u16* tileSprPtrW = tileSprPtr;

        // Shift sprites bitmap and add new character

        u16 firstElem = g_ScrollFntCircularBufferIndex[0];

        // shift elem in circular buffer (we shift tile indexes) > lightweight process
        for (i = 0; i < 15; ++i)
        {
            g_ScrollFntCircularBufferIndex[i] = g_ScrollFntCircularBufferIndex[i + 1];
        }
        g_ScrollFntCircularBufferIndex[15] = firstElem;


        // Load Font bitmap into higher block CBB 4 (tile id 0 - 511)

        // 20 x 5 - 6 characters = 94 chars = 94 regular sprites


        const int lastCharPos = (g_ScrollFntCircularBufferIndex[15] << 4); // last sprite offset used for scrolltext
        i = Scroll1[scrollTextOffset] - ' ';                               // font set start from space bart
        int charPos = i * FNT_CHAR_SIZE;

        scrollTextOffset = (scrollTextOffset + 1) % sizeof(Scroll1); // next char in scroll text

        ScrollTextTriggers(scrollTextOffset);

        // 16 x (16+1) pixels
        // = 4* 8x8 tiles + skip 1 vertical pixel

        // 16x16 sprite = 4 tiles de 8x8

        register u32* curTileSprPtrD = (u32*)&tileSprPtrW[lastCharPos];
        register u32* curFntPtrD = (u32*)&Witan_Bitmap[charPos];

        // top-left
        // 8 scanlines of 8 pixels
        for (j = 0, k = 0; j < 8; ++j, k += 4)
        {
            // lastChar*16*8 + j*4
            *curTileSprPtrD++ = curFntPtrD[k + 0];
            *curTileSprPtrD++ = curFntPtrD[k + 1];
        }

        // top-right
        // 8 scanlines of 8 pixels
        for (j = 0, k = 0; j < 8; ++j, k += 4)
        {
            // lastChar*16*8 + j*4
            *curTileSprPtrD++ = curFntPtrD[k + 2];
            *curTileSprPtrD++ = curFntPtrD[k + 3];
        }

        // bottom-left
        // 8 scanlines of 8 pixels
        for (j = 0, k = 32; j < 8; ++j, k += 4)
        {
            // lastChar*16*8 + j*4
            *curTileSprPtrD++ = curFntPtrD[k + 0];
            *curTileSprPtrD++ = curFntPtrD[k + 1];
        }

        // bottom-right
        // 8 scanlines of 8 pixels
        for (j = 0, k = 32; j < 8; ++j, k += 4)
        {
            *curTileSprPtrD++ = curFntPtrD[k + 2];
            *curTileSprPtrD++ = curFntPtrD[k + 3];
        }

    } // end if scrollPos < -16


    // update scrolling position for each sprite

    // 128 OBJ ATTR
    OBJ_ATTR* obj_buffer = (OBJ_ATTR*)MEM_OAM;
    OBJ_AFFINE* obj_aff_buffer = (OBJ_AFFINE*)obj_buffer;

    static s16 pingpong = 0;
    static s16 pingpongdir = 1;

    if (g_EnableTextVerticalShift)
    {
        pingpongdir = (pingpong >= 20) || (pingpong <= -100) ? -pingpongdir : pingpongdir;
        pingpong += pingpongdir;
    }

    for (i = 0; i < 16; ++i)
    {
        u16 xPos = (scrollPosition + (i << 4)) & 0x01FF; // i*16
        u16 yPos = 120 + pingpong;

        if (g_EnableTextSineWave)
        {
            s16 ampli = g_ScrollTextSineAmpli * g_SinCosTable[g_ScrollTextAngleY + (i << 4)];
            yPos = (yPos + (ampli >> 8)) & 0x0FF;
        }

        // http://www.coranac.com/tonc/text/regobj.htm#tbl-obj-size
        obj_buffer[i].attr0 = ATTR0_Y(yPos) | ATTR0_AFF | ATTR0_8BPP | ATTR0_SQUARE | ATTR0_BLEND; // normal rendering, 256 col,
        obj_buffer[i].attr1 = ATTR1_X(xPos) | ATTR1_SIZE(1) | ATTR1_AFF_ID(i);                     // 16x16 square sprite
        // 1 sprite = 4 tiles de 8x8 x 2 ?
        // Base tile-index of sprite, the higher priority are drawn first.
        // renew tile index of OAM object after potential previous shift
        obj_buffer[i].attr2 = ATTR2_ID(g_ScrollFntCircularBufferIndex[i])
            | ATTR2_PRIO(1); //  	Base tile-index of sprite, the higher priority are drawn first.
    }

    if (g_EnableTextSineWave && (g_ScrollTextSineAmpli < 20))
        g_ScrollTextSineAmpli++;


    // update scaling and rotation


    // http://www.coranac.com/tonc/text/affobj.htm
    // set affine transformations -  origin of the transformation is center of the sprite
    // pa pb
    // pc pd
    // 8.8 fixed point numbers that form the actual matrix

    // If all you are after is a simple scale-then-rotate matrix, try this: for a zoom by sx and sy
    // followed by a counter-clockwise rotation by α, the correct matrix is this:
    // cos(a)/sx  -sin(a)/sx
    // sin(a)/sy  cos(a)/sy

    s16 scale = (1 << 8);
    const u16 cosOffset = 128;
    s16 rotaSin = (0 << 8);
    s16 rotaCos = (1 << 8);

    if (g_EnableTextRotate)
    {
        rotaSin = g_SinCosTable[g_ScrollTextRotaAngle];
        rotaCos = g_SinCosTable[g_ScrollTextRotaAngle + cosOffset];
    }


    for (i = 0; i < 16; ++i)
    {
        if (g_EnableTextZoom)
        {
            scale = ((1 << 8) + (g_SinCosTable[g_ScrollTextAngleZ + (i << 4)] >> 1));
        }

        obj_aff_buffer[i].pa = (rotaCos * scale) >> 8;
        obj_aff_buffer[i].pb = (-rotaSin * scale) >> 8;
        obj_aff_buffer[i].pc = (rotaSin * scale) >> 8;
        obj_aff_buffer[i].pd = (rotaCos * scale) >> 8;
    }
}


//------------------------------------
// VBlank routine
//------------------------------------


void VBlank(void)
{

    if (!g_bVBlankEnabled)
        return;

    if (g_EnableBubbleSprites)
    {
        REG_WININ = WININ_BUILD(WIN_BG0 | WIN_OBJ, WIN_BG1 | WIN_OBJ);   // win1 win0
        REG_WINOUT = WINOUT_BUILD(WIN_BG2 | WIN_OBJ, WIN_BG2 | WIN_OBJ); // winobj winout
    }
    else
    {
        REG_WININ = WININ_BUILD(WIN_BG0, WIN_BG1 | WIN_OBJ);   // win1 win0
        REG_WINOUT = WINOUT_BUILD(WIN_BG2, WIN_BG2 | WIN_OBJ); // winobj winout
    }

    if (g_EnableVerticalCopperBars && g_EnableChessBoardMove)
    {
        REG_WIN0H = (0 << 8) | 240; // horiz 0 to 240 (for BG0/pattern and OBJ)
        REG_WIN0V = (0 << 8) | 64;  // vert 0 to 64  (for BG0/pattern and OBJ)

        //  clip copper background (BG2)
        REG_WIN1H = (0 << 8) | 240;   // horiz 0 to 240 (for BG1/damier and OBJ)
        REG_WIN1V = (120 << 8) | 160; // vert 120 to 160 (for BG1/damier and OBJ)
    }
    else if (!g_EnableVerticalCopperBars && g_EnableChessBoardMove)
    {
        REG_WIN0H = (0 << 8) | 240; // horiz 0 to 240 (for BG0/pattern and OBJ)
        REG_WIN0V = (0 << 8) | 64;  // vert 0 to 64  (for BG0/pattern and OBJ)

        //  clip copper background (BG2)
        REG_WIN1H = (0 << 8) | 240;   // horiz 0 to 240 (for BG1/damier and OBJ)
        REG_WIN1V = (120 << 8) | 160; // vert 120 to 160 (for BG1/damier and OBJ)
    }
    else if (!g_EnableChessBoardMove)
    {
        REG_WIN0H = (0 << 8) | 240; // horiz 0 to 240 (for BG0/pattern and OBJ)
        REG_WIN0V = (0 << 8) | 0;   // vert 0 to 64  (for BG0/pattern and OBJ)

        //  clip copper background (BG2)
        REG_WIN1H = (0 << 8) | 240;   // horiz 0 to 240 (for BG1/damier and OBJ)
        REG_WIN1V = (120 << 8) | 160; // vert 120 to 160 (for BG1/damier and OBJ)
    }

    // multi-scroll BG 0 /////

    u16 scrollx = 0, scrolly = 0;

    scrollx = g_Multi[g_MultiIndex];
    scrolly = g_Multi[g_MultiIndex + 1];

    // hardware scroll registers for BG0
    REG_BG0HOFS = (s16)scrollx;
    REG_BG0VOFS = ((s16)scrolly);

    // Set blending for bubbles

    REG_BLDCNT = BLD_BUILD(BLD_OBJ, BLD_BG0 | BLD_BG1 | BLD_BG2, 1); // top = sprite, bottom = background

    REG_BLDALPHA = BLDA_BUILD(27, 31 - 27); // blending 32 is full weight, 0 is null weight
    REG_BLDY = BLDY_BUILD(0);               // for fade to black only

    // http://www.cs.rit.edu/~tjh8300/CowBite/CowBiteSpec.htm

    // F E D C  B A 9 8  7 6 5 4  3 2 1 0
    // S I I I  I I I I  F F F F  F F F F

    // 0-7 (F) = Fraction
    // 8-E (I) = Integer
    // F   (S) = Sign bit

    // 2D matrix
    // PA PB
    // PC PD

    // BG2 stretch vertically ...
    // BG2 Read Source Pixel Y Increment
    REG_BG2PC = 0; // don't increment Y vertically src -> roll over 1 line of pixel
    // BG2 Write Destination Pixel Y Increment
    REG_BG2PD = 0; // don't increment Y vertically dst

    // -- experiments --

    // BG2 stretch horizontally ...
    REG_BG2PA = 1 << 8; // BG2 Write Destination Pixel X Increment)
    REG_BG2PB = 0 << 8; // BG2 Write Destination Pixel X Increment

    // 31 30 29 28  27 26 25 24  23 22 21 20  19 18 17 16  15 14 13 12  11 10 9 8  7 6 5 4  3 2 1 0
    // X  X  X  X   S  I  I  I   I  I  I  I   I  I  I  I   I  I  I  I   I  I  I I  F F F F  F F F F

    // 0-7  (F) - Fraction
    // 8-26 (I) - Integer
    // 27   (S) - Sign bit

    // These registers define the location of the pixel that appears at 0,0.
    // They are very similar to the background scrolling registers, REG_HOFS
    // and REG_VOFS, which become disabled when a rotate/ scale background is in use.

    REG_BG2X = 0 << 8; // X Coordinate for BG2 Rotational Background

    // update scroll text
    ScrollTextUpdate();

    // update balls curves
    VectorBulleUpdate();

    u16 *swapRasterPage = 0, *p1 = 0, *p2 = 0;
    unsigned int k = 0; //,i;

    u32* tileptrl = 0;

    // add more stuff here
    register u16* tilebaseaddr = 0;
    register u16* tileptr = 0;
    u32 xcoord = 0;
    int kk = 0;
    register u16* colptrw = 0;
    register u32* xcoord_ptr = 0;
    // register u32 scan_flag;
    register u16 bit_mask = 0;
    int angle = 0;
    register u32 AmpIndex = 0;
    register int BarColorIndex = 0;
    register int BarIndex = 0;


    g_ScanCounter = 0;
    g_HBLRasterPtr = g_RasterPage;

    // http://www.coranac.com/tonc/text/dma.htm

    // \param _src	Source address.
    // \param count	Number of transfers.
    // \param ch	DMA channel.
    // \param mode	DMA mode.

    // start HDMA to update background rasters (no need for explicit CPU writes at MEM_PAL_BG / scanline)
    DMA_TRANSFER(MEM_PAL_BG, &g_HBLRasterPtr[1], 1, 3, DMA_HDMA);

    // inc vertical copper counters

    if (g_EnableVerticalCopperBars)
    {
        g_prevBarIndex = (g_prevBarIndex + 1) & 1023;
        g_prevAmpIndex[0] = (g_prevAmpIndex[0] + 2) & 1023;
        g_prevAmpIndex[1] = (g_prevAmpIndex[1] + 2) & 1023;
    }

    g_BarIndex = g_prevBarIndex;
    g_AmpIndex[0] = g_prevAmpIndex[0];
    g_AmpIndex[1] = g_prevAmpIndex[1];

    g_BarColorIndex = 0;


    // clear copper tile data

    // CBB1 + 64;
    tileptrl = (u32*)(&tile8_mem[1][2]); // skip 1st black tile - char block 1

    for (k = 0; k < 32; k++, tileptrl += 14) // fill 8 1st pixels from every 64 bytes
    {
        // write to tiledata test...
        // clear first line of 8x8 tile data
        // tile data = MEM_VRAM + s*0x4000 (/2 qd rot ?)  - char block s

        *tileptrl++ = 0; // 4x8
        *tileptrl++ = 0; // 4x8
    }


    // --- chess color cycling ---

    // cycling chess palette

    for (k = 0; k < (sizeof(g_Chess_Pal) / 3); k++)
    {
        *((u16*)(MEM_PAL_BG + (g_Chess_BaseColor + k) * 2)) = (((u16)g_Chess_Pal[(k + g_Chess_ColorOffset) * 3 + 2] >> 1) << 10)
            | (((u16)g_Chess_Pal[(k + g_Chess_ColorOffset) * 3 + 1] >> 1) << 5) | ((u16)g_Chess_Pal[(k + g_Chess_ColorOffset) * 3] >> 1);
    }

    if (g_EnableChessColorCycling)
    {
        g_Chess_ColorOffset = (g_Chess_ColorOffset + 1) % g_Chess_Period;
    }

    // --- use available cycles to process music ---

    kramWorker();

    // --- wait HBL raster bar zone ---

    // http://www.coranac.com/tonc/text/dma.htm
    // >> HBL-DMA alternative ?

    while (REG_VCOUNT < (NB_BLANK_SCAN - 1 + HBL_OFFSET))
        ;

    for (k = 0; k < 63; ++k)
    {
        // -- experiment --

        s16 offsetX = 0;

        if (g_EnableChessBoardSine)
        {
            offsetX += 7 * g_SinCosTable[g_PatternAngleX + (k << 4)];
            offsetX += 3 * g_SinCosTable[g_PatternAngleX2 + (k << 2)];
            offsetX >>= 8;
        }

        while ((REG_DISPSTAT & DSTAT_IN_HBL) == 0)
            ;

        // -- experiment --

        REG_BG0HOFS = ((s16)(scrollx + offsetX) << 1) / 3;
        REG_BG0VOFS = ((s16)(scrolly) << 1) / 3 - 64 - 32;
    }


    // --- wait HBL copper zone ---

    // skip pattern part & stop before scroll-text


    BarIndex = g_BarIndex;
    AmpIndex = *((u32*)g_AmpIndex); // fetch g_AmpIndex[0] and g_AmpIndex[1]
    BarColorIndex = g_BarColorIndex;

    if (g_EnableVerticalCopperBars)
    {

        // BG2 tile base CBB 1
        tilebaseaddr = &tile8_mem[1][2]; // CBB1 + 64;

        for (k = 0; k < 29; ++k)
        {
            for (kk = 0; kk < 2; kk++)
            {
                s16 offsetX = g_SinCosTable[g_PatternAngleX + (k << 4)] << 3;
                offsetX += g_SinCosTable[g_PatternAngleX2 + (k << 2)] << 2;

                while ((REG_DISPSTAT & DSTAT_IN_HBL) == 0)
                    ;

                // done by HDMA
                // BG2 stretch horizontally ...

                if (g_EnableTwistCopperBars)
                    REG_BG2X = offsetX; // X Coordinate for BG2 Rotational Background
                if (g_EnableTwistCopperBars2)
                    REG_BG2PA = (1 << 8) + (offsetX >> 6);

                angle = (int)g_Copper[BarIndex++];

                xcoord = (HORIZ_RES >> 1) + ((angle * g_Ampli[AmpIndex >> 16]) >> 16);

                // put 8 pixels copper slice

                if (xcoord & 1)
                {
                    xcoord_ptr = &g_LookupXcoord[xcoord >> 1];
                    colptrw = (u16*)&g_BarsOdd[BarColorIndex + 2];

                    tileptr = &tilebaseaddr[*xcoord_ptr++];
                    bit_mask = (*tileptr & 0x00ff) | (((u16) * (((u8*)colptrw) - 1)) << 8);
                    *tileptr++ = bit_mask;

                    tilebaseaddr[*xcoord_ptr++] = *colptrw++;
                    tilebaseaddr[*xcoord_ptr++] = *colptrw++;
                    tilebaseaddr[*xcoord_ptr++] = *colptrw++;

                    tileptr = &tilebaseaddr[*xcoord_ptr];
                    bit_mask = (*tileptr & 0xff00) | ((u16) * ((u8*)colptrw));
                    *tileptr = bit_mask;
                }
                else
                {
                    xcoord_ptr = &g_LookupXcoord[xcoord >> 1];
                    colptrw = (u16*)&g_BarsEven[BarColorIndex];

                    tilebaseaddr[*xcoord_ptr++] = *colptrw++;
                    tilebaseaddr[*xcoord_ptr++] = *colptrw++;
                    tilebaseaddr[*xcoord_ptr++] = *colptrw++;
                    tilebaseaddr[*xcoord_ptr] = *colptrw;
                } /* end if (xcoord & 1) */


                if (g_EnableSecondCopperBar)
                {

                    xcoord = (HORIZ_RES >> 1) + ((angle * g_Ampli[AmpIndex & 1023]) >> 16);

                    if (xcoord & 1)
                    {
                        xcoord_ptr = &g_LookupXcoord[xcoord >> 1];
                        colptrw = (u16*)&g_BarsOdd[BarColorIndex + 130]; // 16*8 + 2

                        tileptr = &tilebaseaddr[*xcoord_ptr++];
                        bit_mask = (*tileptr & 0x00ff) | (((u16) * (((u8*)colptrw) - 1)) << 8);
                        *tileptr++ = bit_mask;

                        tilebaseaddr[*xcoord_ptr++] = *colptrw++;
                        tilebaseaddr[*xcoord_ptr++] = *colptrw++;
                        tilebaseaddr[*xcoord_ptr++] = *colptrw++;

                        tileptr = &tilebaseaddr[*xcoord_ptr];
                        bit_mask = (*tileptr & 0xff00) | ((u16) * ((u8*)colptrw));
                        *tileptr = bit_mask;
                    }
                    else
                    {
                        xcoord_ptr = &g_LookupXcoord[xcoord >> 1];
                        colptrw = (u16*)&g_BarsEven[BarColorIndex + 128]; // 16*8

                        tilebaseaddr[*xcoord_ptr++] = *colptrw++;
                        tilebaseaddr[*xcoord_ptr++] = *colptrw++;
                        tilebaseaddr[*xcoord_ptr++] = *colptrw++;
                        tilebaseaddr[*xcoord_ptr] = *colptrw;
                    } /* end if (xcoord & 1) */
                }

                BarColorIndex = (BarColorIndex + 8) & 127; // if >= 128 then 0
                AmpIndex += 65537;                         // 65536 + 1


            } /** end for kk **/
        }
    }

    // --- end copper zone ---

    // re-launch HDMA to update damier sine wave offsets

    // Set blending for scrolltext sprite

    REG_BLDCNT = BLD_BUILD(BLD_OBJ, BLD_BG0 | BLD_BG1 | BLD_BG2, 1); // top = sprite, bottom = background

    REG_BLDALPHA = BLDA_BUILD(29, 31 - 29); // blending 32 is full weight, 0 is null weight
    REG_BLDY = BLDY_BUILD(0);               // for fade to black only


    g_BarIndex = BarIndex;
    g_BarColorIndex = BarColorIndex;
    *((u32*)g_AmpIndex) = AmpIndex;


    ///// build BG raster /////
    p1 = g_BgPage1;
    p2 = &g_OffRasterPage[0];

    for (k = 0; k < 64; k++)
    {
        *p2++ = *p1++;
    }

    p1 = g_Raster2;
    for (k = 0; k < 56; k++)
    {
        *p2++ = *p1++;
    }

    if (g_EnableHorizontalCopperBars)
    {
        // add vague

        p2 = g_VaguePTR;
        p1 = &g_OffRasterPage[0];
        for (k = 0; k < 8; k++)
        {

            p1 = &g_OffRasterPage[*p2];
            p2 += 4;

            *p1 &= 0x3ff;
            *p1++ |= 8 << 10; // blue
            *p1 &= 0x3ff;
            *p1++ |= 16 << 10;
            *p1 &= 0x3ff;
            *p1++ |= 24 << 10;
            *p1 &= 0x3ff;
            *p1++ |= 31 << 10;
            *p1 &= 0x3ff;
            *p1++ |= 24 << 10;
            *p1 &= 0x3ff;
            *p1++ |= 16 << 10;
            *p1 &= 0x3ff;
            *p1++ |= 8 << 10;
        }

        // Affichage des rouleaux ...

        p2 = g_RolaxPTR;
        p1 = &g_OffRasterPage[0];
        for (k = 0; k < 6; k++)
        {
            p1 = &g_OffRasterPage[*p2++];

            *p1 &= 0x7c1f;
            *p1++ |= 8 << 5; // green
            *p1 &= 0x7c1f;
            *p1++ |= 16 << 5;
            *p1 &= 0x7c1f;
            *p1++ |= 24 << 5;
            *p1 &= 0x7c1f;
            *p1++ |= 31 << 5;
            *p1 &= 0x7c1f;
            *p1++ |= 24 << 5;
            *p1 &= 0x7c1f;
            *p1++ |= 16 << 5;
            *p1 &= 0x7c1f;
            *p1++ |= 8 << 5;
        }
    }

    // raster page flip
    swapRasterPage = g_RasterPage;
    g_RasterPage = g_OffRasterPage;
    g_OffRasterPage = swapRasterPage;
    g_HBLRasterPtr = g_RasterPage;

    swapRasterPage = g_BgPage1;
    g_BgPage1 = g_BgPage2;
    g_BgPage2 = swapRasterPage;


    // wrap around

    if (g_EnableHorizontalCopperBars)
    {
        g_VaguePTR++;
        if (g_VaguePTR >= &g_Vague[511])
        {
            g_VaguePTR = g_Vague;
        }

        g_RolaxPTR += 6;
        if (g_RolaxPTR >= &g_Rolax[256 * 6 - 1])
        {
            g_RolaxPTR = g_Rolax;
        }
    }

    if (g_EnableChessBoardMove)
    {
        g_MultiIndex += 2;
        if (g_MultiIndex >= g_MultiSz)
        {
            g_MultiIndex = 0;
        }
    }

    // update curves functions


    g_ScrollTextAngleZ = (g_ScrollTextAngleZ + 5) & 511;
    g_ScrollTextAngleY = (g_ScrollTextAngleY + 10) & 511;
    g_ScrollTextRotaAngle = (g_ScrollTextRotaAngle + 3) & 511;

    g_PatternAngleX = (g_PatternAngleX + 7) & 511;
    g_PatternAngleX2 = (g_PatternAngleX2 + 5) & 511;

    if (g_EnableBubbleSprites)
    {
        g_BulleTimeAngle = (g_BulleTimeAngle + 2 /*3*/) & 511;
        g_BulleTimeAngle2 = (g_BulleTimeAngle + 4 /*6*/) & 511;
        g_BulleTimeAngle3 = (g_BulleTimeAngle + 6 /*9*/) & 511;
        g_BulleModAngleX = (g_BulleModAngleX + 3) & 511;
        g_BulleModAngleY = (g_BulleModAngleY + 4) & 511;
    }

    g_bNewFrame = 1;
}


// IntrTable for crt0
void (*IntrTable[])() = {
    VBlank,        // v-blank 	(for background movement & raster/copper update)
    0,             // h-blank
    0,             // vcount
    0,             // timer0
    kradInterrupt, // timer 1
    0,             // timer2
    0,             // timer3
    0,             // serial
    0,             // dma0
    0,             // dma1		(for soundbuffer-swapping)
    0,             // dma2
    0,             // dma3
    0,             // key
    0,             // cart
};


//! Wait for next VBlank
void vid_vsync()
{
    // http://problemkaputt.de/gbatek.htm

    while (REG_VCOUNT >= 160)
        ; // wait till VDraw
    while (REG_VCOUNT < 160)
        ; // wait till VBlank
}

// http://detective-ds.googlecode.com/svn/trunk/source/CFxFade.cpp
// https://www.coursehero.com/file/p4evcg3/1321-GBA-Blending-Backgrounds-are-always-enabled-for-blending-To-enable-sprite/

void FadeIn(u16 factor)
{
    REG_BLDCNT = (3 << 6) | 63; // blend A with black (fade to black) using the weight from REG_BLDY
    REG_BLDY = factor;          // Top blend fade. Used for white and black fades.
}

void FadeOut(u16 factor)
{
    REG_BLDCNT = (3 << 6) | 63; // blend A with black (fade to black) using the weight from REG_BLDY
    REG_BLDY = factor;          // Top blend fade. Used for white and black fades.
}

void BlendOff(void) { REG_BLDCNT = (0 << 6) | 63; }

void ShowSplash(void)
{
    int i = 0;

    // 240x160 15 bits
    REG_DISPCNT = DCNT_MODE3 | DCNT_BG2;

    // 240x160 256 colors
    REG_BG2CNT = 0; // Clear all BG2 parameters.

    LZ77UnCompVram((u16*)gba_240x160_juice, (u16*)(MEM_VRAM));

    // generate checker board here
    CheckerBoardPattern();

    Init_Tables_part1();

    u32 fade = 0;

    // fade_out()
    while (fade != (16 << 1))
    {
        fade += 1;
        FadeOut(fade >> 2);
        vid_vsync();
    }

    LZ77UnCompVram((u16*)Kukoo2Anniversary_title, (u16*)(MEM_VRAM));

    BlendOff();

    Init_Tables_part2();

    fade = 0;

    // fade_out()
    while (fade != (16 << 1))
    {
        fade += 1;
        FadeOut(fade >> 2);
        vid_vsync();
    }
}

void ShowTFLTDV(void)
{
    int i, j;
    u16* palSrc;
    u16* palDest;
    u16* videoBuffer;
    u16* srcImage;
    u8* tmpImage;

    // http://www.coranac.com/tonc/text/bitmaps.htm
    // 240x160 15 bits
    REG_DISPCNT = DCNT_MODE3 | DCNT_BG2;

    // 240x160 256 colors
    REG_BG2CNT = 0; // Clear all BG2 parameters.

    BlendOff();

    // set palette

    palSrc = (u16*)tfltdv240x160_8_Palette;

    videoBuffer = (u16*)(MEM_VRAM);
    srcImage = (u16*)tfltdv240x160_8Bitmap;
    tmpImage = (u8*)(g_HeapEwram - 240 * 160);
    LZ77UnCompWram(srcImage, tmpImage);

    // convert to 15 bits in videoBuffer

    for (i = 0; i < (240 * 160); ++i)
    {
        *videoBuffer++ = palSrc[*tmpImage++];
    }
}


void ExitTFLTDV(void)
{
    u32 fade = 0;

    // fade_out()
    while (fade != (16 << 1))
    {
        fade += 4;
        FadeIn(fade >> 2);

        kramWorker();

        vid_vsync();
    }
    kramWorker();
}


void ShowHappyEnd()
{
    g_bVBlankEnabled = 0;

    vid_vsync();

    // http://www.coranac.com/tonc/text/bitmaps.htm
    // http://www.coranac.com/tonc/text/first.htm

    // 240x160 15 bits
    REG_DISPCNT = DCNT_MODE3 | DCNT_BG2;
    REG_BG2CNT = 0; // Clear all BG2 parameters.

    // http://libnds.devkitpro.org/group__main__display__registers.html
    // http://www.cs.rit.edu/~tjh8300/CowBite/CowBiteSpec.htm

    // BG2 increments - matrix 2x2
    // identity : diagonal of 1
    // 1 0
    // 0 1
    REG_BG2PC = 0 << 8; // REG_BG2PC (BG2 Read Source Pixel Y Increment)
    REG_BG2PD = 1 << 8; // REG_BG2PD (BG2 Write Destination Pixel Y Increment)
    REG_BG2PA = 1 << 8; // REG_BG2PA (BG2 Read Source Pixel X Increment)
    REG_BG2PB = 0 << 8; // REG_BG2PB (BG2 Write Destination Pixel X Increment)

    REG_BG2HOFS = 0;
    REG_BG2VOFS = 0;

    LZ77UnCompVram((u16*)happyend_evoke2015, (u16*)(MEM_VRAM));
}

void ExitIntro()
{
    krapStop();
    kradDeactivate();
    kragReset();

    // http://www.coranac.com/tonc/text/interrupts.htm

    REG_IME = 0;
    REG_IE = 0;
    REG_DISPSTAT = 0;

    // http://www.coranac.com/tonc/text/dma.htm

    ShowHappyEnd();

    u16 count = 60 * 3;
    while (count-- > 0)
    {
        vid_vsync();
    }

    u32 fade = 0;

    // fade_out()
    while (fade != (32 << 1))
    {
        fade += 1;
        FadeIn(fade >> 3);
        vid_vsync();
    }

    // reset ram
    asm("swi 0x01\n"
        "bx lr");
}


void MusicEventsCallback(int event, int param) { ExitIntro(); }


void AgbMain()
{
    int pixBatch = 0;

    InitMemBufferAlloc();

    // show splash screen and calculate tables
    ShowSplash();

    kragInit(KRAG_INIT_STEREO);
    kramQualityMode(KRAM_QM_NORMAL);

    krapCallback(MusicEventsCallback);

    REG_IME = 0;

    ShowTFLTDV();

    REG_IE |= IRQ_TM1;
    REG_IME = 1;

    // Start playing MOD
    krapPlay(&mod_music, KRAP_MODE_SONG, 0);

    // fill background with Julia set and play music

    for (pixBatch = 0; pixBatch < 9600; ++pixBatch)
    {
        kramWorker();
        MandelJuliaPixel(4);
    }

    kramWorker();

    ExitTFLTDV();

    kramWorker();

    // Initialise irqs;

    REG_DISPSTAT |= DSTAT_VBL_IRQ;

    kramWorker();

    // Set the Background to Mode 4
    // 240x160
    // 160x128
    // kukoo2 orig : 384x564
    // 225 HBL scanlines

    // Setup the background mode

    // Address: 0x4000000 - REG_DISPCNT (The display control register)
    // 0-2 (M) = The video mode. See video modes list above.
    // Mode 1 This mode is similar in most respects to Mode 0, the main difference
    //  being that only 3 backgrounds are accessible -- 0, 1, and 2. Bgs 0 and 1
    // are text backgrounds, while bg 2 is a rotation/scaling background.

    // http://www.coranac.com/tonc/text/video.htm#sec-blanks

    REG_DISPCNT = DCNT_MODE1 | DCNT_BG0 | DCNT_BG1 | DCNT_BG2 | DCNT_WIN0 | DCNT_WIN1 | DCNT_WINOBJ | DCNT_OBJ | DCNT_OBJ_1D;
    REG_BG0CNT = 0; // Clear all BG0 parameters.
    REG_BG1CNT = 0; // Clear all BG1 parameters.
    REG_BG2CNT = 0; // Clear all BG2 parameters.

    kramWorker();

    Clear_Screen();
    kramWorker();

    Init_Rasters();
    kramWorker();

    Init_Palette();
    kramWorker();

    Put_Pattern();
    kramWorker();

    Put_Sprites();
    kramWorker();

    // Initialize a tiled BG for use in HAM.

    // http://www.coranac.com/tonc/text/regbg.htm
    // bits	name	define	description
    // 0-1	Pr 	BG_PRIO# 	Priority. Determines drawing order of backgrounds.
    // 2-3	CBB 	BG_CBB# 	Character Base Block. Sets the charblock that serves as the base for character/tile indexing. Values: 0-3.
    // 6 	Mos 	BG_MOSAIC 	Mosaic flag. Enables mosaic effect.
    // 7 	CM 	BG_4BPP, BG_8BPP 	Color Mode. 16 colors (4bpp) if cleared; 256 colors (8bpp) if set.
    // 8-C	SBB 	BG_SBB# 	Screen Base Block. Sets the screenblock that serves as the base for screen-entry/map indexing. Values: 0-31.
    // D 	Wr 	BG_WRAP 	Affine Wrapping flag. If set, affine background wrap around at their edges. Has no effect on regular backgrounds
    // as they wrap around by default. E-F	Sz 	BG_SIZE#, see below 	Background Size. Regular and affine backgrounds have different sizes
    // available to them. The sizes, in tiles and in pixels, can be found in table 9.5.

    // Sz-flag 	define 	(tiles)	(pixels)
    // 00 	BG_REG_32x32 	32x32 	256x256
    // 01 	BG_REG_64x32 	64x32 	512x256
    // 10 	BG_REG_32x64 	32x64 	256x512
    // 11 	BG_REG_64x64 	64x64 	512x512

    // 00 	BG_AFF_16x16 	16x16 	128x128
    // 01 	BG_AFF_32x32 	32x32 	256x256
    // 10 	BG_AFF_64x64 	64x64 	512x512
    // 11 	BG_AFF_128x128 	128x128 	1024x1024


    // Parameters:
    // bgno  The BG number we want to initialize (0-3)
    // active  Turn the BG on or off (0,1)
    // prio  Priority of this BG to other BGs (0=highest prio,3=lowest)
    // mosaic  Enable / Disable Mosaic for this BG (0=off,1=on)

    // http://www.coranac.com/tonc/text/regbg.htm
    //
    // Both the tiles and tilemaps are stored in VRAM, which is divided into charblocks and screenblocks.
    // The tileset is stored in the charblocks and the tilemap goes into the screenblocks.
    //
    // Memory 	0600:0000 	0600:4000 	0600:8000 	0600:C000
    // charblock 	0 	1 	2 	3
    // screenblock 	0 	… 	7 	8 	… 	15 	16 	… 	23 	24 	… 	31


    // initialize background 0, show,  priority
    /* 64x32 tiles = 512x256, no rotation */
    REG_BG0CNT = BG_PRIO(3) | BG_CBB(0) | BG_8BPP | BG_SBB(1) | BG_REG_64x32; // multiscroll pattern

    // initialize background 1, show, priority
    REG_BG1CNT = BG_PRIO(3) | BG_CBB(2) | BG_8BPP | BG_SBB(4) | BG_REG_32x32; // color cycling pattern

    // initialize background 2, show,  priority, mosaic
    REG_BG2CNT = BG_PRIO(3) | BG_MOSAIC | BG_CBB(1) | BG_8BPP | BG_SBB(3) | BG_AFF_32x32; // vertical coppers

    //  Create a new GBA Window

    // http://www.coranac.com/tonc/text/gfx.htm

    REG_WININ = WININ_BUILD(WIN_BG0, WIN_BG1 | WIN_OBJ);   // win1 win0
    REG_WINOUT = WINOUT_BUILD(WIN_BG2, WIN_BG2 | WIN_OBJ); // winobj winout

    //  clip pattern background (BG0) : emulate kukoo2 VGA LineCompare
    //  clip chess color cycle background (BG1) :
    //  OBJ (sprites) are visible everywhere

    REG_WIN0H = (0 << 8) | 240; // horiz 0 to 240 (for BG0/pattern and OBJ)
    REG_WIN0V = (0 << 8) | 64;  // vert 0 to 64  (for BG0/pattern and OBJ)

    //  clip copper background (BG2)
    REG_WIN1H = (0 << 8) | 240;   // horiz 0 to 240 (for BG1/damier and OBJ)
    REG_WIN1V = (120 << 8) | 160; // vert 120 to 160 (for BG1/damier and OBJ)

    REG_BLDCNT = BLD_BUILD(BLD_OBJ, BLD_BG0 | BLD_BG1 | BLD_BG2, 1); // top = sprite, bottom = background

    REG_BLDALPHA = BLDA_BUILD(29, 31 - 29); // blending 32 is full weight, 0 is null weight
    REG_BLDY = BLDY_BUILD(0);               // for fade to black only

    kramWorker();
    REG_IE |= IRQ_TM1 | IRQ_VBLANK;

    // Loop
    while (1)
    {
        // Will be set to 1 in VBLhandler (VblFunc) again
        while (g_bNewFrame == 0)
            ;
        g_bNewFrame = 0;

    } // end infinite loop
}
