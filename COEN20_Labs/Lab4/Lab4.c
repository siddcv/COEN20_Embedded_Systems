/*
	This code was written to support the book, "ARM Assembly for Embedded Applications",
	by Daniel W. Lewis. Permission is granted to freely share this software provided
	that this notice is not removed. This software is intended to be used with a run-time
    library adapted by the author from the STM Cube Library for the 32F429IDISCOVERY 
    board and available for download from http://www.engr.scu.edu/~dlewis/book3.
*/

#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>
#include <memory.h>
#include "library.h"
#include "graphics.h"

#define	PLOT_YMIN		204
#define PLOT_YMAX		297
#define	PLOT_HEIGHT		(PLOT_YMAX - PLOT_YMIN + 1)

#define PLOT_XMIN		20
#define PLOT_XMAX		219
#define	PLOT_WIDTH		(PLOT_XMAX - PLOT_XMIN + 1)

#define	FILTER_SAMPLES	5

// Function to implement in assembly: Returns (mtop*x + mbtm/2)/mbtm + b
extern int32_t MxPlusB(int32_t x, int32_t mtop, int32_t mbtm, int32_t b) ;

typedef int BOOL ;
#define	FALSE			0
#define	TRUE			1

typedef struct
	{
	int16_t				VREFIN_CAL ;	// Raw data acquired at temperature of 30 °C VDDA = 3.3 V
	int16_t				TEMP_3V3_030C ;	// ADC reading for 30 degrees C w/ref voltage = 3.3
	int16_t				TEMP_3V3_110C ;	// ADC reading for 30 degrees C w/ref voltage = 3.3
	} ADC_CAL ;

typedef struct
	{
	uint32_t			SR ;		// Status register
	uint32_t			CR[2] ;		// Control registers 1 & 2
	uint32_t			SMPR[2] ;	// Sample time registers 1,2
	uint32_t			JOFR[4] ;	// Injected channel data offset registers 1-4
	uint32_t			HTR ;		// Watchdog higher threshold register 
	uint32_t			LTR ;		// Watchdog lower threshold register 
	uint32_t			SQR[3] ;	// Regular sequence register 1-3
	uint32_t			JSQR ;		// Injected sequence register
	uint32_t			JDR[4] ;	// Injected data registers 1-4
	uint32_t			DR ;		// Regular data register
	} ADC_CHANNEL ;

typedef struct
	{
	uint32_t			CSR ;		// Common status register
	uint32_t			CCR ;		// Common control register
	uint32_t			CDR ;		// Common regular data register for dual and triple modes
	} ADC_COMMON ;

typedef struct
	{
	uint32_t			CR ;			// Clock control register
	uint32_t			PLLCFGR ;		// PLL configuration register 
	uint32_t			CFGR ;			// Clock configuration register 
	uint32_t			CIR ;			// Clock interrupt register
	uint32_t			AHBRSTR[4] ;	// AHB peripheral reset registers 1-3 (4 unused)
	uint32_t			APBRSTR[4] ;	// APB peripheral reset registers 1-2 (3,4 unused)
	uint32_t			AHBENR[4] ;		// AHB peripheral clock enable registers 1-3 (4 unused)
	uint32_t			APBENR[4] ;		// APB peripheral clock enable registers 1-2 (3,4 unused)
	uint32_t			AHBLPENR[4] ;	// AHB peripheral clock enable in low power mode registers 1-3 (4 unused)
	uint32_t			APBLPENR[4] ;	// APB peripheral clock enable in low power mode registers 1-2 (3,4 unused)
	uint32_t			BDCR ;			// Backup domain control register
	uint32_t			CSR ;			// Clock control & status register
	uint32_t			reserved[2] ;
	uint32_t			SSCGR ;			// Spread spectrum clock generation register 
	uint32_t			PLLI2SCFGR ;	// PLLI2S configuration register
	uint32_t			PLLSAICFGR ;	// PLL configuration register
	uint32_t			DCKCFGR ;		// Dedicated Clock Configuration Register
	} RSTCLKCTL ;

typedef struct
	{
	int32_t				samples ;
	int32_t				rawX100[PLOT_WIDTH] ;
	int32_t				fltX100[PLOT_WIDTH] ;
	int32_t				minX100 ;
	int32_t				maxX100 ;
	float				yScale ;
	int32_t				minC ;
	int32_t				maxC ;
	} PLOT_DATA ;

// Public fonts defined in run-time library
typedef struct
	{
	const uint8_t *		table ;
	const uint16_t		Width ;
	const uint16_t		Height ;
	} sFONT ;

extern sFONT Font8, Font12, Font16, Font20, Font24 ;
#define	FONT			Font12

static ADC_CAL *		CALIBRATION =	(ADC_CAL *)		0x1FFF7A2A ;
static ADC_CHANNEL *	ADC1		=	(ADC_CHANNEL *)	0x40012000 ;
static ADC_COMMON *		ADC  		= 	(ADC_COMMON  *)	0x40012300 ;
static RSTCLKCTL *		RCC			=	(RSTCLKCTL *) 	0x40023800 ;

#define	ADC1_EN			(1 << 8)
#define	ADC1_EOC		(1 << 1)
#define	ADC1_LEN		(1 << 20)
#define	ADC1_IN18		18	// Input channel for temperature sensor
#define	ADC1_IN17		17	// Input channel for internal voltage reference
#define	ADON			(1 << 0)
#define	EOCS			(1 << 10)
#define	SWSTART			(1 << 30)
#define	TSVREFE			(1 << 23)

static void				ADC_Init(void) ;
static int32_t			ADC_Reading(int32_t channel) ;
static void				DelayMS(uint32_t msec) ;
static uint32_t			GetTimeout(uint32_t msec) ;
static void				LEDs(int grn_on, int red_on) ;
static int32_t			LowPassFilter(PLOT_DATA *plot, int depth) ;
static int32_t			PutStringAt(int32_t x, int32_t y, char *fmt, ...) ;
static void				PlotDegreesC(PLOT_DATA *plot, int sample) ;
static uint32_t *		PixelAddress(uint32_t x, uint32_t y) ;
static void				Rescale(PLOT_DATA *plot) ;
static BOOL				SanityChecksOK(void) ;
static void				SetFontSize(sFONT *Font) ;
static void				ShiftPlotLeft(void) ;
static BOOL				UpdateData(PLOT_DATA *plot, int32_t degrX100) ;

#define	ENTRIES(a)		(sizeof(a)/sizeof(a[0]))

int main(void)
	{
	int32_t curVref, calVref, cal030, cal110, scaled110 ;
	int32_t rawTemp, scaled030, y, savey, degreesC ;
	static PLOT_DATA plot = {0} ;

	InitializeHardware(NULL, "Lab 4c: Linear Interpolation") ;
	ADC_Init() ;
	if (!SanityChecksOK()) return 0 ;

	curVref = ADC_Reading(ADC1_IN17) ;			// Get current reference voltage reading
	calVref = CALIBRATION->VREFIN_CAL ;			// Get calibrated reference voltage reading

	cal030 = CALIBRATION->TEMP_3V3_030C ;		// Get calibrated temp reading for  30 degrees C
	cal110 = CALIBRATION->TEMP_3V3_110C ;		// Get calibrated temp reading for 110 degrees C

	// Scale calibrated temp readings to current reference voltage
	scaled030 = MxPlusB(cal030, curVref, calVref, 0) ;
	scaled110 = MxPlusB(cal110, curVref, calVref, 0) ;

	SetFontSize(&FONT) ;
	y = 50 ;

	y = PutStringAt(20, y, "REF VOLTAGE A/D MEASUREMENTS:") + 2 ;
	DrawRect(19, y - 3, 201, 39) ;
	SetForeground(COLOR_LIGHTGREEN) ;
	FillRect(20, y - 2, 200, 38) ;
	SetForeground(COLOR_BLACK) ;
	SetBackground(COLOR_LIGHTGREEN) ;
	y = PutStringAt(20, y, "       Current Vref: %5d", (int) curVref) ;
	y = PutStringAt(20, y, "    Calibrated Vref: %5d", (int) calVref) ;
	y = PutStringAt(20, y, "       Scale Factor: %5.2f", (float) curVref /(float) calVref) ;
	SetBackground(COLOR_WHITE) ;
	y += 4 ;

	y = PutStringAt(20, y, "TEMPERATURE A/D MEASUREMENTS:") + 2 ;
	DrawRect(19, y - 3, 201, 83) ;
	SetForeground(COLOR_LIGHTGREEN) ;
	FillRect(20, y - 2, 200, 82) ;
	SetForeground(COLOR_BLACK) ;
	SetBackground(COLOR_LIGHTGREEN) ;
	y = PutStringAt(20, y, "  Calibrated ( 30C): %5d", (int) cal030) ;
	y = PutStringAt(20, y, "  Calibrated (110C): %5d", (int) cal110) ;
	y += 4 ;
	y = PutStringAt(20, y, "      Scaled ( 30C): %5d", (int) scaled030) ;
	y = PutStringAt(20, y, "      Scaled (110C): %5d", (int) scaled110) ;
	y += 4 ;

	savey = y ;
	while (1)
		{
		y = savey ;

		// Get raw temp reading from A/D converter
		rawTemp = ADC_Reading(ADC1_IN18) ;

		SetForeground(COLOR_BLACK) ;
		SetBackground(COLOR_LIGHTGREEN) ;
		y = PutStringAt(20, y, "    Raw A/D Reading: %5d", (int) rawTemp) ;

		// Convert to temp in degrees C (times 100)
		degreesC = MxPlusB(rawTemp - scaled030, 8000, scaled110 - scaled030, 3000) ;

		if (UpdateData(&plot, degreesC)) Rescale(&plot) ;
		ShiftPlotLeft() ;
		PlotDegreesC(&plot, plot.samples - 1) ;

		SetForeground(COLOR_BLACK) ;
		SetBackground(COLOR_LIGHTGREEN) ;
		PutStringAt(20, y, "   Temp (degrees C): %5.1f", plot.fltX100[plot.samples-1]/100.0) ;

		DelayMS(50) ;
		}

	return 0 ;
	}

static BOOL UpdateData(PLOT_DATA *plot, int32_t degreesC)
	{
	static int32_t oldMin = 0 ;
	static int32_t oldMax = 0 ;
	int32_t *pflt ;
	int k ;

	if (plot->samples > 0)
		{
		oldMin = plot->minX100 ;
		oldMax = plot->maxX100 ;
		}

	if (plot->samples == ENTRIES(plot->rawX100))
		{
		memmove(&plot->rawX100[0], &plot->rawX100[1], sizeof(plot->rawX100) - sizeof(plot->rawX100[0])) ;
		memmove(&plot->fltX100[0], &plot->fltX100[1], sizeof(plot->fltX100) - sizeof(plot->fltX100[0])) ;
		plot->samples-- ;
		}

	plot->rawX100[plot->samples] = degreesC ;
	plot->fltX100[plot->samples] = LowPassFilter(plot, FILTER_SAMPLES) ;
	plot->minX100 = plot->maxX100 = plot->fltX100[plot->samples++] ;

	pflt = plot->fltX100 ;
	for (k = 0; k < plot->samples; k++, pflt++)
		{
		if (*pflt < plot->minX100) plot->minX100 = *pflt ;
		if (*pflt > plot->maxX100) plot->maxX100 = *pflt ;
		}

	return plot->minX100 != oldMin || plot->maxX100 != oldMax ;
	}

static int32_t LowPassFilter(PLOT_DATA *plot, int depth)
	{
	int32_t total, *sample ;
	int k ;

	if (depth > plot->samples + 1) depth = plot->samples + 1 ;

	sample = &plot->rawX100[plot->samples] ;
	total = 0 ; ;
	for (k = 0; k < depth; k++)
		{
		total += *sample-- ;
		}

	return total / k ;
	}

static void Rescale(PLOT_DATA *plot)
	{
	int degreesC, step, sample, line, range ;
	char text[100] ;

	// Erase all previous vertical labels
	SetForeground(COLOR_WHITE) ;
	FillRect(4, PLOT_YMIN - FONT.Height/2, 2*FONT.Width, PLOT_HEIGHT + FONT.Height/2) ;

	// Paint background of graph
	SetForeground(COLOR_LIGHTYELLOW) ;
	FillRect(PLOT_XMIN, PLOT_YMIN, PLOT_WIDTH, PLOT_HEIGHT) ;

	// Paint outline of graph
	SetForeground(COLOR_BLACK) ;
	DrawRect(PLOT_XMIN - 1, PLOT_YMIN - 1, PLOT_WIDTH + 1, PLOT_HEIGHT) ;

	// Label vertical scale
	plot->minC = plot->minX100/100 ;
	plot->maxC = (plot->maxX100 + 100)/100 ;
	if (plot->maxC == plot->minC) plot->maxC = plot->minC + 1 ;
	plot->yScale = (float) PLOT_HEIGHT / (plot->maxC - plot->minC) ;

	step = 0.5 + (float) (plot->maxC - plot->minC + 1) / (PLOT_HEIGHT / FONT.Height) ;
	if (step == 0) step = 1 ;

	SetForeground(COLOR_BLACK) ;
	SetBackground(COLOR_WHITE) ;
	for (degreesC = plot->minC; degreesC <= plot->maxC; degreesC += step)
		{
		int row = 0.5 + plot->yScale * (degreesC - plot->minC) ;
		sprintf(text, "%d", (int) degreesC) ;
		DisplayStringAt(4, PLOT_YMAX - row - FONT.Height/2, text) ;
		}

	// Draw horizontal grid lines
	range = plot->maxC - plot->minC ;
	if (range == 1) step = 1 ;
	else if (range <= 3) step = 2 ;
	else step = 4 ;
	SetForeground(COLOR_BLACK) ;
	for (line = 0; line <= 4*range; line += step)
		{
		int row = 0.5 + plot->yScale * line/4.0 ;
		DrawHLine(PLOT_XMIN, PLOT_YMAX - row, PLOT_WIDTH) ;
		}

	// Replot old data with new scale
	for (sample = 0; sample < plot->samples; sample++)
		{
		PlotDegreesC(plot, sample) ;
		}
	}

static void ShiftPlotLeft(void)
	{
	uint32_t *px ;
	int row ;

	px = PixelAddress(PLOT_XMIN + 1, PLOT_YMIN) ;
	for (row = PLOT_YMIN; row <= PLOT_YMAX; row++, px += 240)
		{
		memmove(px, px + 1, (PLOT_XMAX - PLOT_XMIN - 1)*sizeof(uint32_t)) ;
		}
	}

static void PlotDegreesC(PLOT_DATA *plot, int sample)
	{
	int yold, xnew, ynew ;

	if (sample == 0 || plot->samples < 2) return ;

	yold = plot->yScale * (plot->fltX100[sample-1]/100.0 - plot->minC) ;
	xnew = PLOT_XMAX - (plot->samples - sample) ;
	ynew = plot->yScale * (plot->fltX100[sample]/100.0 - plot->minC) ;

	SetForeground(COLOR_RED) ;
	DrawLine(xnew - 1, PLOT_YMAX - yold, xnew, PLOT_YMAX - ynew) ;
	}

static void ADC_Init(void)
	{
	RCC->APBENR[1]	|= ADC1_EN ;		// Turn on ADC1 clock
	ADC->CCR		|= TSVREFE ;		// Select temperature sensor and internal voltage reference as input 
	ADC1->CR[1]		= (ADON | EOCS) ;	// Turn on ADC and configure EndOfConversion signal
	ADC1->SQR[0]	= ADC1_LEN ;		// Set # channels in seequence to 1
	}

static int32_t ADC_Reading(int32_t channel)
	{
	ADC1->SQR[2]	= channel ;			// Select channel IN18 as only input
	DelayMS(1) ;

	ADC1->CR[1]		|= SWSTART ;		// Start the ADC EndOfConversion=1
	DelayMS(1) ;							// Give it a chance to convert

	while ((ADC1->SR & ADC1_EOC) == 0) ;// Wait for conversion
	return ADC1->DR ;					// Read data and clear EndOfConversion
	}

static uint32_t GetTimeout(uint32_t msec)
	{
#	define	CPU_CLOCK_SPEED_MHZ			168
	uint32_t cycles = 1000 * msec * CPU_CLOCK_SPEED_MHZ ;
	return GetClockCycleCount() + cycles ;
	}

static void DelayMS(uint32_t msec)
	{
	uint32_t timeout = GetTimeout(msec) ;
	while ((int) (timeout - GetClockCycleCount()) > 0) ;
	}

static int32_t PutStringAt(int32_t x, int32_t y, char *fmt, ...)
	{
	char text[100] ;
	va_list aptr ;

	va_start(aptr, fmt);
	vsprintf(text, fmt, aptr) ;
	va_end(aptr) ;
	DisplayStringAt(x, y, text) ;
	return y + FONT.Height ;
	}

static uint32_t *PixelAddress(uint32_t x, uint32_t y)
	{
	return ((uint32_t *) 0xD0000000) + x + 240*y ;
	}

static BOOL SanityChecksOK(void)
	{
#	define	COUNT(a)	(sizeof(a)/sizeof(a[0]))
	typedef struct {int	x, mtop, mbtm, b, correct, result; } CHECK ;
	static CHECK check[] =
		{
		{0,	 0,	 1,	 1,	 1},	{2,	 3,	 1,	 0,	 6},	{2,	-3,	 1,	 0,	-6},	{2,	 3,	-1,	0, -6},
		{1,	 5,	 3,	 0,	 2},	{1,	-5,	 3,	 0,	-2},	{1,	 5,	-3,	 0,	-2},	{1,	-5,	-3,	0,  2},
		{1,	 4,	 3,	 0,	 1},	{1,	-4,	 3,	 0,	-1},	{1,	 4,	-3,	 0,	-1},	{1,	-4,	-3,	0,  1}
		} ;
	int which, errors ;
	CHECK *p ;

	errors = 0 ;
	p = check ;
	for (which = 0; which < COUNT(check); which++, p++)
		{
		p->result = MxPlusB(p->x, p->mtop, p->mbtm, p->b) ;
		if (p->result != p->correct) errors++ ;
		}
	LEDs(!errors, errors) ;
	if (errors == 0) return TRUE ;

	printf("\n       SANITY CHECK ERRORS:\n\n") ;
	printf("       x mtop mbtm b  result\n") ;
	printf("       - ---- ---- -  ------\n") ;

	p = check ;
	for (which = 0; which < COUNT(check); which++, p++)
		{
		if (p->result != p->correct) printf("       %2d  %+d   %+d  %d  %+4d\n", p->x, p->mtop, p->mbtm, p->b, p->result) ;
		}
	return FALSE ;
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

static void SetFontSize(sFONT *Font)
	{
	extern void BSP_LCD_SetFont(sFONT *) ;
	BSP_LCD_SetFont(Font) ;
	}
