/*
	This code was written to support the book, "ARM Assembly for Embedded Applications",
	by Daniel W. Lewis. Permission is granted to freely share this software provided
	that this notice is not removed. This software is intended to be used with a run-time
    library adapted by the author from the STM Cube Library for the 32F429IDISCOVERY 
    board and available for download from http://www.engr.scu.edu/~dlewis/book3.
*/

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include "library.h"
#include "graphics.h"
#include "touch.h"

// Function to be implemented in assembly language:
extern void MatrixMultiply(int32_t a[3][3], int32_t b[3][3], int32_t c[3][3]) ;

// Public function defined in this file to be called from assembly
int32_t MultAndAdd(int32_t a, int32_t b, int32_t c)
	{
	float z = *((float *) &a) + *((float *) &b) * *((float *) &c) ;
	return *((int32_t *) &z) ;
	}

typedef int					BOOL ;
#define	FALSE				0
#define	TRUE				1

typedef uint8_t				CLR_INDEX ;	// CLR_INDEX is simply an index into
#define	CLR_INDEX_WHITE		0			// the ChromArt's color look-up table
#define	CLR_INDEX_BLUE		1
#define	CLR_INDEX_GREEN		2			// These index values must correspond
#define	CLR_INDEX_CYAN		3			// to the order of colors defined in
#define	CLR_INDEX_RED		4			// function ChromArtInitialize.
#define	CLR_INDEX_MAGENTA	5
#define	CLR_INDEX_YELLOW	6

typedef uint32_t			CLR_RGB32 ;

#define	SCREEN_DIMENSIONS	2	// x & y
typedef	int					SCREEN_COORDINATE[SCREEN_DIMENSIONS] ;

#define	FRAME_DIMENSIONS	3	// x, y, & z
typedef float				VECTOR[FRAME_DIMENSIONS] ;
typedef VECTOR				MATRIX[FRAME_DIMENSIONS] ;

#define	MATRIX_ROWS			(sizeof(MATRIX)/sizeof(VECTOR))
#define	MATRIX_COLS			(sizeof(VECTOR)/sizeof(float))

typedef struct
	{
	float 					x ;
	float					y ;
	float					z ;
	} VERTEX ;

#define	VERTICES			3

typedef struct
	{
	VERTEX *				vertices[VERTICES] ;
	CLR_INDEX				clr_index ;
	} TRIANGLE ;

typedef struct
	{
	uint32_t				CR ;		// Control register
	uint32_t				ISR ;		// Interrupt Status Register 
	uint32_t				IFCR ;		// Interrupt flag clear register
	uint32_t				FGMAR ;		// Foreground memory address register
	uint32_t				FGOR ;		// Foreground offset register 
	uint32_t				BGMAR ;		// Background memory address register
	uint32_t				BGOR ;		// Background offset register
	uint32_t				FGPFCCR ;	// Foreground PFC control register
	uint32_t				FGCOLR ;	// Foreground color register
	uint32_t				BGPFCCR ;	// Background PFC control register
	uint32_t				BGCOLR ;	// Background color register
	uint32_t				FGCMAR ;	// Foreground CLUT memory address register
	uint32_t				BGCMAR ;	// Background CLUT memory address register
	uint32_t				OPFCCR ;	// Output PFC control register
	uint32_t				OCOLR ;		// Output color register 
	uint32_t				OMAR ;		// Output memory address register
	uint32_t				OOR ;		// Output offset register
	uint32_t				NLR ;		// Number of line register
	uint32_t				LWR ;		// Line watermark register
	uint32_t				AMTCR ;		// AHB master timer configuration register
	} CHROM_ART ;

typedef struct
	{
	const char *			name ;
	uint32_t * const		pmsec ;
	const float				vmin ;
	const float				vmax ;
	const uint32_t			xmin ;
	const uint32_t			ymin ;
	const uint32_t			width ;
	const uint32_t			height ;
	uint32_t				xpos ;
	} SLIDER ;

#define	FONT_WIDTH			7
#define	FONT_HEIGHT			12

#define	SLIDER_VMIN			1
#define	SLIDER_VMAX			120
#define	SLIDER_BGCLR		COLOR_LIGHTGRAY
#define	SLIDER_FGCLR		COLOR_BLACK
#define	SLIDER_RPADDING		5
#define	SLIDER_LPADDING		5
#define	SLIDER_HSIZE		(XPIXELS - SLIDER_LPADDING - SLIDER_RPADDING)
#define	SLIDER_VSIZE		12
#define	SLIDER_SSIZE		SLIDER_VSIZE
#define	SLIDER_XMIN			SLIDER_LPADDING
#define	SLIDER_YMIN			285

// Public fonts defined in run-time library
typedef struct
	{
	const uint8_t *			table ;
	const uint16_t			Width ;
	const uint16_t			Height ;
	} sFONT ;

extern sFONT				Font8, Font12, Font16, Font20, Font24 ;

#define	PI					3.14159
#define	ENTRIES(a)			(sizeof(a)/sizeof(a[0]))

#define	FRAME_ROWS			240
#define	FRAME_COLS			240
typedef CLR_INDEX			FRAME[FRAME_ROWS][FRAME_COLS] ;

#define	X_CENTER			(FRAME_COLS/2)
#define	Y_CENTER			(FRAME_ROWS/2)

#define	DISPLAY_XOFF		0
#define	DISPLAY_YOFF		48

#define	X_LEFT				-1.0
#define	X_RIGHT				+1.0

#define	Y_TOP				+1.0
#define	Y_BOTTOM			-1.0

#define	Z_FRONT				-1.0
#define	Z_REAR				+1.0

#define	SIZE				60.0

#define	ERR_FONT			Font12
#define	ERR_BRDR_COLOR		COLOR_BLACK
#define	ERR_BGND_COLOR		COLOR_RED
#define	ERR_FGND_COLOR		COLOR_WHITE

#define	MIN(a,b)			((a) < (b) ? (a) : (b))
#define	MAX(a,b)			((a) > (b) ? (a) : (b))

#define	CPU_CLOCK_SPEED_MHZ	168

static void					Adjust(SLIDER *slider) ;
static int32_t				Between(uint32_t min, uint32_t val, uint32_t max) ;
static void					BtmFlatTriangle(int x1, int x2, int xMin, int yMin, int yMax) ;
static void					CheckSlider(void) ;
static void					ChromArtInitialize(void) ;
static void					ChromArtWaitForDMA(void) ;
static void					ChromArtXferFrameBuffer(CLR_RGB32 *screen_pixels, FRAME frame_pixels) ;
static uint32_t				GetTimeout(uint32_t msec) ;
static void					DisplaySpeed(SLIDER *slider) ;
static void					Error(char *function, char *format, ...) ;
static void					GetScreenCoordinates(SCREEN_COORDINATE screen_coordinates[VERTICES], VERTEX *vertices[VERTICES]) ;
static void					HorizLine(int x, int y, int width) ;
static void					IdentityMatrix(MATRIX matrix) ;
static void					InitializeTouchScreen(void) ;
static void					InitSlider(SLIDER *slider) ;
static void					LEDs(int grn_on, int red_on) ;
static void					MxM(MATRIX a, MATRIX b, MATRIX c) ;
static void					MxV(VECTOR dstVector, MATRIX matrix, VECTOR srcVector) ;
static void					PaintTriangle(TRIANGLE *pTriangle) ;
static void					PutStringAt(int x, int y, char *fmt, ...) ;
static void					RotateAboutXAxis(float radians, MATRIX matrix) ;
static void					RotateAboutYAxis(float radians, MATRIX matrix) ;
static void					RotateAboutZAxis(float radians, MATRIX matrix) ;
static void					SanityCheck(void) ;
static void					SetColorIndex(CLR_INDEX index) ;
static void					SetFontSize(sFONT *pFont) ;
static void					TopFlatTriangle(int x1, int x2, int xMax, int yMin, int yMax) ;
static void					UpdateSlider(SLIDER *slider, uint32_t x) ;
static void					UpdateValue(SLIDER *slider, uint32_t x) ;
static BOOL					Visible(TRIANGLE *pTriangle) ;
static float				VxV(VECTOR vec1, VECTOR vec2) ;
static void					WaitForTimeout(uint32_t timeout, void (*func)(void)) ;

static uint32_t *			AHB1ENR	= (uint32_t *)	0x40023800 ;
static CHROM_ART *			DMA2D	= (CHROM_ART *)	0x4002B000 ;
static CLR_RGB32 *			FG_CLUT = (CLR_RGB32 *)	0x4002B400 ; 
static CLR_INDEX			clr_index = CLR_INDEX_WHITE ;
static CLR_RGB32 *			screen_pixels = (CLR_RGB32 *) 0xD0000000 ;
static FRAME				frame_pixels ;

// Define the vertices of the cube ...
static VERTEX 				ftl = {X_LEFT,	Y_TOP,		Z_FRONT} ;	// front top left
static VERTEX 				ftr = {X_RIGHT,	Y_TOP,		Z_FRONT} ;	// front top right
static VERTEX 				fbl = {X_LEFT,	Y_BOTTOM,	Z_FRONT} ;	// front bottom left
static VERTEX 				fbr = {X_RIGHT,	Y_BOTTOM,	Z_FRONT} ;	// front bottom right
static VERTEX 				rtl = {X_LEFT,	Y_TOP,		Z_REAR} ;	// rear top left
static VERTEX 				rtr = {X_RIGHT,	Y_TOP,		Z_REAR} ;	// rear top right
static VERTEX 				rbl = {X_LEFT,	Y_BOTTOM,	Z_REAR} ;	// rear bottom left
static VERTEX 				rbr = {X_RIGHT,	Y_BOTTOM,	Z_REAR} ;	// rear bottom right

// Create an array of pointers to the vertices ...
static VERTEX *				vertices[] = {&ftl, &ftr, &fbl, &fbr, &rtl, &rtr, &rbl, &rbr} ;

// Define the cube as an array of triangles - two per face.
// First vertex of each triangle must be at the 90 degree
// corner & in clockwise order as seen from outside of cube.
static TRIANGLE				triangles[] =
	{
	{{&rtl, &rtr, &ftl},	CLR_INDEX_YELLOW	},	// top
	{{&ftr, &ftl, &rtr},	CLR_INDEX_YELLOW	},

	{{&ftl, &ftr, &fbl},	CLR_INDEX_GREEN		},	// front face
	{{&fbr, &fbl, &ftr},	CLR_INDEX_GREEN		},

	{{&rtl, &ftl, &rbl},	CLR_INDEX_RED		},	// left side
	{{&fbl, &rbl, &ftl},	CLR_INDEX_RED		},

	{{&rtl, &rbl, &rtr},	CLR_INDEX_CYAN		},	// rear face
	{{&rbr, &rtr, &rbl},	CLR_INDEX_CYAN		},

	{{&rtr, &rbr, &ftr},	CLR_INDEX_BLUE		},	// right side
	{{&fbr, &ftr, &rbr},	CLR_INDEX_BLUE		},

	{{&fbl, &fbr, &rbl},	CLR_INDEX_MAGENTA	},	// bottom
	{{&rbr, &rbl, &fbr},	CLR_INDEX_MAGENTA	}
	} ;

static uint32_t msec = 60 ; // 20 RPM
static SLIDER slider = {"Speed", &msec, SLIDER_VMIN, SLIDER_VMAX, SLIDER_XMIN, SLIDER_YMIN, SLIDER_HSIZE, SLIDER_VSIZE} ;

int main()
	{
	uint32_t timeout ;
	MATRIX matrix ;

	InitializeHardware(NULL, "Lab 5a: Spinning Cube") ;
	InitializeTouchScreen() ;
	SanityCheck() ;
	ChromArtInitialize() ;
	InitSlider(&slider) ;

	// Create the transformation matrix
	IdentityMatrix(matrix) ;
	RotateAboutXAxis(PI/25, matrix) ;
	RotateAboutYAxis(PI/25, matrix) ;
	RotateAboutZAxis(PI/25, matrix) ;

	timeout = GetTimeout(msec) ;
	for (;;)
		{
		TRIANGLE *pTriangle ;
		VERTEX **ppVertex ;
		int k ;

		// Let DMA finish copying the frame buffer to the
		// display buffer before modifying the frame buffer
		ChromArtWaitForDMA() ;

		// Pause if user presses push button
		while (PushButtonPressed()) ;

		// Erase the frame buffer (remove triangles)
		memset(frame_pixels, CLR_INDEX_WHITE, sizeof(frame_pixels)) ;

		// Transform all the vertices
		ppVertex = &vertices[0] ;
		for (k = 0; k < ENTRIES(vertices); k++, ppVertex++)
			{
			MxV((float *) *ppVertex, matrix, (float *) *ppVertex) ;
			}

		// Paint visible triangles to the frame buffer
		pTriangle = &triangles[0] ;
		for (k = 0; k < ENTRIES(triangles); k++, pTriangle++)
			{
			if (Visible(pTriangle)) PaintTriangle(pTriangle) ;
			}

		// Copy frame buffer to display buffer; Chrom-Art Controller
		// automatically converts L8 (256 color table) to ARGB8888 format
		ChromArtXferFrameBuffer(screen_pixels, frame_pixels) ;

		// Limit the cube's rotation rate
		WaitForTimeout(timeout, CheckSlider) ;
		timeout = GetTimeout(msec) ;
		}

	return 0 ;
	}

static void CheckSlider(void)
	{
	if (TS_Touched()) Adjust(&slider) ;
	}

static void SetColorIndex(CLR_INDEX index)
	{
	clr_index = index ;
	}

static void HorizLine(int x, int y, int width)
	{
	int k, xmin, xmax ;
	CLR_INDEX *pPixel ;

	// Clip line to frame boundaries ...
	if (y < 0 || y >= FRAME_ROWS) return ;
	xmin = MAX(x, 0) ;
	xmax = MIN(x + width, FRAME_COLS) ;
	width = xmax - xmin ;
	if (width <= 0) return ;

	// Paint line to frame buffer
	pPixel = &frame_pixels[y][xmin] ;
	for (k = 0; k < width; k++)
		{
		*pPixel++ = clr_index ;
		}
	}

static void BtmFlatTriangle(int x1, int x2, int xMin, int yMin, int yMax)
	{
	int dx1, dx2, sx1, sx2, err1, err2, dy, y ;

	dy = yMin - yMax ;
	if (dy == 0) return ;

	dx1 = abs(x1 - xMin) ;
	dx2 = abs(x2 - xMin) ;
	if (dx1 == 0 && dx2 == 0) return ;

	err1 = dx1 + dy ;
	err2 = dx2 + dy ;

	sx1 = (x1 < xMin) ? -1 : +1 ;
	sx2 = (x2 < xMin) ? -1 : +1 ;

	x1 = x2 = xMin ;
	for (y = yMin;; y++)
		{
		int xLeft = (x1 < x2) ? x1 : x2 ;
		HorizLine(xLeft, y, 1 + abs(x2 - x1)) ;
		if (y == yMax) break ;

		for (;;)
			{
			int e2 = 2*err1 ;
			if (e2 >= dy ) { err1 += dy ; x1 += sx1 ; }
			if (e2 <= dx1) { err1 += dx1 ; break ; }
			}

		for (;;)
			{
			int e2 = 2*err2 ;
			if (e2 >= dy ) { err2 += dy ;  x2 += sx2 ; }
			if (e2 <= dx2) { err2 += dx2 ; break ; }
			}
		}
	}

static void TopFlatTriangle(int x1, int x2, int xMax, int yMin, int yMax)
	{
	int dx1, dx2, sx1, sx2, err1, err2, dy, y ;

	dy = yMin - yMax ;
	if (dy == 0) return ;

	dx1 = abs(x1 - xMax) ;
	dx2 = abs(x2 - xMax) ;
	if (dx1 == 0 && dx2 == 0) return ;

	err1 = dx1 + dy ;
	err2 = dx2 + dy ;

	sx1 = (x1 < xMax) ? -1 : +1 ;
	sx2 = (x2 < xMax) ? -1 : +1 ;

	x1 = x2 = xMax ;
	for (y = yMax;; y--)
		{
		int xLeft = (x1 < x2) ? x1 : x2 ;
		HorizLine(xLeft, y, 1 + abs(x2 - x1)) ;
		if (y == yMin) break ;

		for (;;)
			{
			int e2 = 2*err1 ;
			if (e2 >= dy ) { err1 += dy ; x1 += sx1 ; }
			if (e2 <= dx1) { err1 += dx1 ; break ; }
			}

		for (;;)
			{
			int e2 = 2*err2 ;
			if (e2 >= dy ) { err2 += dy ;  x2 += sx2 ; }
			if (e2 <= dx2) { err2 += dx2 ; break ; }
			}
		}
	}

static int Compare(const void *p1, const void *p2)
	{
	// Used with qsort to sort vertices of a triangle
	int *pY1 = (int *) p1 ;
	int *pY2 = (int *) p2 ;
	return pY1[1] - pY2[1] ;
	}

static void PaintTriangle(TRIANGLE *pTriangle)
	{
	SCREEN_COORDINATE screen_coordinates[VERTICES] ;
#	define	X(k)	(screen_coordinates[k][0])
#	define	Y(k)	(screen_coordinates[k][1])
	int x3, dvnd, dvsr ;

	GetScreenCoordinates(screen_coordinates, pTriangle->vertices) ;

	// Required: y[0] <= y[1] <= y[2]
	qsort(screen_coordinates, VERTICES, sizeof(SCREEN_COORDINATE), Compare) ;

	// Nothing to do if vertical extent is zero
	dvsr = Y(2) - Y(0) ;
	if (dvsr == 0) return ;

	SetColorIndex(pTriangle->clr_index) ;

	// Divide into two right triangles
	dvnd = (Y(1) - Y(0)) * (X(2) - X(0)) ;
	x3 = X(0) + dvnd/dvsr ;

	if (Y(0) != Y(1)) BtmFlatTriangle(X(1), x3, X(0), Y(0), Y(1)) ;
	if (Y(1) != Y(2)) TopFlatTriangle(X(1), x3, X(2), Y(1), Y(2)) ;
	}

static BOOL Visible(TRIANGLE *pTriangle)
	{
	float dx1, dy1, dx2, dy2 ;

	// Surface normal is cross-product of two sides
	dx1 = pTriangle->vertices[0]->x - pTriangle->vertices[1]->x ;
	dy1 = pTriangle->vertices[0]->y - pTriangle->vertices[1]->y ;

	dx2 = pTriangle->vertices[1]->x - pTriangle->vertices[2]->x ;
	dy2 = pTriangle->vertices[1]->y - pTriangle->vertices[2]->y ;

	// Return TRUE if surface normal points towards us
	return (dx1 * dy2) < (dy1 * dx2) ;
	}

static void GetScreenCoordinates(SCREEN_COORDINATE screen_coordinates[VERTICES], VERTEX *vertices[VERTICES])
	{
	VERTEX **ppVertex ;
	int k, x, y ;

	// Convert floating-point vertex coordinates
	// to screen row and column coordinates
	ppVertex = &vertices[0] ;
	for (k = 0; k < VERTICES; k++, ppVertex++)
		{
		int *pPixel = screen_coordinates[k] ;
		pPixel[0] = X_CENTER + (int) SIZE*(*ppVertex)->x ;
		pPixel[1] = Y_CENTER + (int) SIZE*(*ppVertex)->y ;

		// vertex 0 is at the 90 degree corner;
		// extend the opposite edge to avoid gap
		// between the two triangles of cube face
		if (k == 0) { x = pPixel[0]; y = pPixel[1]; continue; }

		pPixel[0] += (pPixel[0] > x) ? +1 : -1 ;
		pPixel[1] += (pPixel[1] > y) ? +1 : -1 ;
		}
	}

static float VxV(VECTOR v1, VECTOR v2)
	{
	// Dot Product: Scalar (returned) <-- Vector (v1) * Vector (v2)
	float sum ;
	int k ;

	sum = 0.0 ;
	for (k = 0; k < MATRIX_COLS; k++)
		{
		sum += v1[k] * v2[k] ;
		}

	return sum;
	}

static void MxV(VECTOR dstVector, MATRIX matrix, VECTOR srcVector)
	{
	// Vector (vdst) <-- Matrix (matrix) * Vector (vsrc)
	float *pFloat, *pMatrixRow ;
	VECTOR tmpVector ;
	int row ;

	pFloat = tmpVector ; pMatrixRow = (float *) matrix ;
	for (row = 0; row < MATRIX_ROWS; row++)
		{
		*pFloat++ = VxV(pMatrixRow, srcVector) ;
		pMatrixRow += MATRIX_COLS ;
		}
	memcpy(dstVector, tmpVector, sizeof(tmpVector)) ;
	}

static void MxM(MATRIX a, MATRIX b, MATRIX c)
	{
	// Matrix (a) <-- Matrix (b) * Matrix (c)
	MATRIX tmpMatrix ;

	MatrixMultiply((void *) tmpMatrix, (void *) b, (void *) c) ;
	memcpy(a, tmpMatrix, sizeof(tmpMatrix)) ;
	}

static void IdentityMatrix(MATRIX matrix)
	{
	// matrix <-- Identity matrix
	static MATRIX ident =
		{
		{1.0, 0.0, 0.0},
		{0.0, 1.0, 0.0},
		{0.0, 0.0, 1.0}
		} ;

	memcpy(matrix, ident, sizeof(MATRIX)) ;
	}

static void RotateAboutZAxis(float radians, MATRIX matrix)
	{
	MATRIX tmpMatrix ;

	memset(tmpMatrix, 0, sizeof(MATRIX)) ;
	tmpMatrix[0][0] = +cos(radians) ;
	tmpMatrix[0][1] = -sin(radians) ;
	tmpMatrix[1][0] = +sin(radians) ;
	tmpMatrix[1][1] = +cos(radians) ;
	tmpMatrix[2][2] = +1.0 ;
	MxM(matrix, matrix, tmpMatrix) ;
	}

static void RotateAboutYAxis(float radians, MATRIX matrix)
	{
	MATRIX tmpMatrix ;

	memset(tmpMatrix, 0, sizeof(MATRIX)) ;
	tmpMatrix[0][0] = +cos(radians) ;
	tmpMatrix[0][2] = +sin(radians) ;
	tmpMatrix[1][1] = +1.0 ;
	tmpMatrix[2][0] = -sin(radians) ;
	tmpMatrix[2][2] = +cos(radians) ;
	MxM(matrix, matrix, tmpMatrix) ;
	}

static void RotateAboutXAxis(float radians, MATRIX matrix)
	{
	MATRIX tmpMatrix ;

	memset(tmpMatrix, 0, sizeof(MATRIX)) ;
	tmpMatrix[0][0] = +1.0 ;
	tmpMatrix[1][1] = +cos(radians) ;
	tmpMatrix[1][2] = -sin(radians) ;
	tmpMatrix[2][1] = +sin(radians) ;
	tmpMatrix[2][2] = +cos(radians) ;
	MxM(matrix, matrix, tmpMatrix) ;
	}

static uint32_t GetTimeout(uint32_t msec)
	{
	return GetClockCycleCount() + 1000 * msec * CPU_CLOCK_SPEED_MHZ ;
	}

static void	WaitForTimeout(uint32_t timeout, void (*func)(void))
	{
	do if (func != NULL) (*func)() ;
	while ((int) (timeout - GetClockCycleCount()) > 0) ;
	}

static void ChromArtWaitForDMA(void)
	{
	// wait until DMA transfer is finished
	while ((DMA2D->CR & 1) != 0)
		{
		// Poll no faster than once every microsecond
		uint32_t timeout = GetClockCycleCount() + CPU_CLOCK_SPEED_MHZ ;
		while ((int) (timeout - GetClockCycleCount()) > 0) ;	
		}
	}

static void ChromArtInitialize(void)
	{
	static uint32_t color_table[] =
		{
		COLOR_WHITE,	// These must be listed in
		COLOR_BLUE,		// the same order as the
		COLOR_GREEN,	// color table indices
		COLOR_CYAN,		// (CLR_INDEX_WHITE, CLR_INDEX_BLUE, etc.)
		COLOR_RED,
		COLOR_MAGENTA,
		COLOR_YELLOW
		} ;
	int k ;

	*AHB1ENR |= (1 << 23) ;	// Turn on DMA2D clock
	WaitForTimeout(GetTimeout(1), NULL) ;

	// Load color look-up table (CLUT)
	for (k = 0; k < ENTRIES(color_table); k++)
		{
		FG_CLUT[k] = color_table[k] ;
		}
	}

static void ChromArtXferFrameBuffer(CLR_RGB32 *screen_pixels, FRAME frame_pixels)
	{
	DMA2D->NLR		= (FRAME_COLS << 16) | (FRAME_ROWS - 20) ; 

	DMA2D->OMAR		= (uint32_t) (screen_pixels + XPIXELS*DISPLAY_YOFF + DISPLAY_XOFF) ;
	DMA2D->OOR		= 0 ;
	DMA2D->OPFCCR	= 0 ;	// Output pixel format ARGB8888.

	DMA2D->FGMAR	= (uint32_t) frame_pixels ;	// foreground (source buffer) address.
	DMA2D->FGOR		= 0 ;	// pixels rows are adjacent in source.
	DMA2D->FGPFCCR	= 5 ;	// Source pixel format L8.

	// start transfer; Enable PFC (Pixel Format Conversion)
	DMA2D->CR		= 0x10001 ;
	}

static void SanityCheck(void)
	{
	MATRIX random, ident, product ;
	int row, col ;

	LEDs(TRUE, FALSE) ;
	IdentityMatrix(ident) ;
	for (row = 0; row < MATRIX_ROWS; row++)
		{
		for (col = 0; col < MATRIX_COLS; col++)
			{
			random[row][col] = (float) GetRandomNumber() / UINT32_MAX ;
			}
		}
	MatrixMultiply((void *) product, (void *) ident, (void *) random) ;
	for (row = 0; row < MATRIX_ROWS; row++)
		{
		for (col = 0; col < MATRIX_COLS; col++)
			{
			if (product[row][col] != random[row][col])
				{
				Error("MatrixMultiply", "Bad Result @ r,c=%d,%d", row, col) ;
				}
			}
		}
	}

static void SetFontSize(sFONT *pFont)
	{
	extern void BSP_LCD_SetFont(sFONT *) ;
	BSP_LCD_SetFont(pFont) ;
	}

static void Error(char *function, char *format, ...)
	{
#	define	GFXROW1		54
#	define	GFXROWN		215
#	define	GFXROWS		(GFXROWN - GFXROW1 + 1)
	uint32_t width, row, col, chars ;
	va_list args ;
	char text[100] ;

	sprintf(text, "Error: %s", function) ;
	chars = strlen(text) ;
	width = ERR_FONT.Width * (chars + 2) ;
	col = (XPIXELS - width) / 2 ;
	row = GFXROW1 + GFXROWS / 2 ;

	SetFontSize(&ERR_FONT) ;
	SetColor(ERR_BGND_COLOR) ;
	FillRect(col + 1, row, width - 1, 3*ERR_FONT.Height) ;
	SetColor(ERR_BRDR_COLOR) ;
	DrawRect(col, row, width, 3*ERR_FONT.Height) ;
	row += ERR_FONT.Height/2 ;

	SetForeground(ERR_FGND_COLOR) ;
	SetBackground(ERR_BGND_COLOR) ;
	DisplayStringAt(col + ERR_FONT.Width, row, text) ;
	row += ERR_FONT.Height ;

	va_start(args, format) ;
	vsprintf(text, format, args) ;
	va_end(args) ;
	DisplayStringAt(col + ERR_FONT.Width, row, text) ;

	LEDs(0, 1) ;
	for (;;) ;
	}

static void LEDs(int grn_on, int red_on)
	{
	static uint32_t * const pGPIOG_MODER	= (uint32_t *) 0x40021800 ;
	static uint32_t * const pGPIOG_ODR		= (uint32_t *) 0x40021814 ;
	
	*pGPIOG_MODER |= (1 << 28) | (1 << 26) ;	// output mode
	*pGPIOG_ODR &= ~(3 << 13) ;			// both off
	*pGPIOG_ODR |= (grn_on ? 1 : 0) << 13 ;
	*pGPIOG_ODR |= (red_on ? 1 : 0) << 14 ;
	}

static void InitializeTouchScreen(void)
	{
	static char *message[] =
		{
		"If this message remains on",
		"the screen, there is an",
		"initialization problem with",
		"the touch screen. This does",
		"NOT mean that there is a",
		"problem with your code.",
		" ",
		"To correct the problem,",
		"disconnect and reconnect",
		"the power.",
		NULL
		} ;
	char **pp ;
	unsigned x, y ;

	x = 25 ;
	y = 100 ;
	for (pp = message; *pp != NULL; pp++)
		{
		DisplayStringAt(x, y, *pp) ;
		y += 12 ;
		}
	TS_Init() ;
	ClearDisplay() ;
	}

static void Adjust(SLIDER *slider)
	{
#	define	XFUDGE	0
#	define	YFUDGE	-10
	uint32_t x, y, xmin, xmax ;

	x = TS_GetX() + XFUDGE ;
	y = TS_GetY() + YFUDGE ;

	xmax = slider->xmin + slider->width - SLIDER_SSIZE/2 - 1 ;
	xmin = slider->xmin + SLIDER_SSIZE/2 ;
	if (!Between(slider->ymin, y, slider->ymin + slider->height - 1))	return ;
	if (!Between(slider->xmin, x, slider->xmin + slider->width - 1))	return ;
	if (x < xmin) x = xmin ;
	if (x > xmax) x = xmax ;
	UpdateValue(slider, x) ;
	UpdateSlider(slider, x) ;
	}

static int32_t Between(uint32_t min, uint32_t val, uint32_t max)
	{
	return (min <= val && val <= max) ;
	}

static void UpdateValue(SLIDER *slider, uint32_t x)
	{
	float percent = 1.0 - (float) (x - slider->xmin - SLIDER_SSIZE/2) / (slider->width - SLIDER_SSIZE - 1) ;
	*slider->pmsec = slider->vmin + percent * (slider->vmax - slider->vmin) ;
	DisplaySpeed(slider) ;
	}

static void DisplaySpeed(SLIDER *slider)
	{
	static char text[100] ;
	static int xpos = 0 ;

	if (xpos != 0)
		{
		SetColor(COLOR_WHITE) ;
		PutStringAt(xpos, slider->ymin - FONT_HEIGHT, text) ;
		}

	sprintf(text, "%s = %d RPM", slider->name, (int) (1200 / *slider->pmsec)) ;
	xpos = slider->xmin + (slider->width - FONT_WIDTH * strlen(text)) / 2 ;
	SetColor(COLOR_BLACK) ;
	PutStringAt(xpos, slider->ymin - FONT_HEIGHT, text) ;
	}

static void InitSlider(SLIDER *slider)
	{
	float percent ;

	DisplaySpeed(slider) ;
	SetColor(SLIDER_BGCLR) ;
	FillRect(slider->xmin + 1, slider->ymin, slider->width - 1, slider->height) ;
	SetColor(COLOR_RED) ;
	DrawRect(slider->xmin, slider->ymin, slider->width, slider->height) ;
	percent = (float) (*slider->pmsec - slider->vmin) / (slider->vmax - slider->vmin) ;
	UpdateSlider(slider, slider->xpos = slider->xmin + percent * slider->width) ;
	}

static void UpdateSlider(SLIDER *slider, uint32_t x)
	{
	static CLR_RGB32 bg[SLIDER_SSIZE], fg[SLIDER_SSIZE] ;
	int k, xold, xnew, ypos, yoff ;
	static BOOL init = TRUE ;

	if (init)
		{
		for (k = 0; k < SLIDER_SSIZE; k++)
			{
			bg[k] = SLIDER_BGCLR ;
			fg[k] = SLIDER_FGCLR ;
			}
		init = FALSE ;
		}

	ypos = slider->ymin + 1 ;
	xold = slider->xpos - SLIDER_SSIZE/2 + 1 ;
	xnew = x - SLIDER_SSIZE/2 + 1 ;
	for (yoff = 0; yoff < slider->height - 1; yoff++)
		{
		memcpy(screen_pixels + XPIXELS*ypos + xold, bg, sizeof(bg)) ;
		memcpy(screen_pixels + XPIXELS*ypos + xnew, fg, sizeof(fg)) ;
		ypos++ ;
		}
	slider->xpos = x ;
	}

static void PutStringAt(int x, int y, char *fmt, ...)
	{
	va_list args ;
	char text[100] ;

	va_start(args, fmt) ;
	vsprintf(text, fmt, args) ;
	va_end(args) ;

	DisplayStringAt(x, y, text) ;
	}

