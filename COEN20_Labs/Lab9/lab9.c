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
#include "library.h"
#include "graphics.h"

typedef int32_t Q16 ;
typedef int BOOL ;

#define	FALSE	0
#define	TRUE	1

#define	BRDR_WNDW	COLOR_RED
#define	FGND_WNDW	COLOR_BLACK
#define	BGND_WNDW	COLOR_YELLOW

#define	XLFT_WNDW	2
#define	XRGT_WNDW	234
#define	XSIZ_WNDW	(XRGT_WNDW - XLFT_WNDW + 1)
#define	XLBL_WNDW	(XLFT_WNDW + 5)

#define	YTOP_RSLT	70
#define	YTOP_PERF	170
#define	YPOS_TESTS	260
#define	YPOS_MSG	280

#define	FGND_ERR	COLOR_WHITE
#define	BGND_ERR	COLOR_RED

#define	FGND_GOOD	COLOR_WHITE
#define	BGND_GOOD	COLOR_DARKGREEN

#define	FGND_NRML	COLOR_BLACK
#define	BGND_NRML	COLOR_WHITE

// Public fonts defined in run-time library
typedef struct
	{
	const uint8_t *	table ;
	const uint16_t	Width ;
	const uint16_t	Height ;
	} sFONT ;

typedef struct { unsigned num, ttl, min, avg, max ; } CYCLES ;

static Q16		Correct(Q16 dividend, Q16 divisor) ;
static unsigned	DisplayAt(unsigned xpos, unsigned ypos, char *text, sFONT *font) ;
static Q16		GetOperand(void) ;
static void		Message(char *msg) ;
static BOOL		Overflow(Q16 dividend, Q16 divisor) ;
static void		Performance(unsigned cyc_div, unsigned cyc_ref) ;
static BOOL		Results(Q16 dividend, Q16 divisor, Q16 quotient, Q16 correct) ;
static void		SetFontSize(sFONT *pFont) ;
static void		TestCount(unsigned count) ;
static void		UpdateCycles(CYCLES *cyc, unsigned cycles) ;
static unsigned	Window(unsigned ypos, unsigned rows, char *label, sFONT *font) ;

extern Q16		Q16Divide(Q16 dividend, Q16 divisor) ;
extern sFONT	Font8, Font12, Font16, Font20, Font24 ;

#define	FLOAT(q16)	((q16) / 65536.0)

int main(void)
	{
	unsigned counter, cyc_div, cyc_ref ;
	Q16 dividend, divisor, quotient, correct ;
	uint32_t params[4] ;
	uint32_t result[2] ;
	BOOL error ;

	InitializeHardware(HEADER, "Lab 8f: Q16 Division") ;

	Message("Blue Pushbutton to Pause") ;
	for (counter = 0;; counter++)
		{
		dividend = GetOperand() ;
		divisor  = GetOperand() ;
		if (Overflow(dividend, divisor)) continue ;

		TestCount(counter) ;

		params[0] = dividend ;
		params[1] = divisor ;
		cyc_div = CountCycles(Q16Divide, params, params, result) ;
		quotient = result[0] ;
		cyc_ref = CountCycles(Correct, params, params, result) ;
		correct = result[0] ;
		Performance(cyc_div, cyc_ref) ;
		error = Results(dividend, divisor, quotient, correct) ;
		if (error)
			{
			Message("Blue Pushbutton to Continue") ;
			WaitForPushButton() ;
			Message("Blue Pushbutton to Pause") ;
			}
		else if (PushButtonPressed())
			{
			Message("Release Pushbutton to Continue") ;
			while (PushButtonPressed()) ;
			Message("Blue Pushbutton to Pause") ;
			}
		}

	return 0 ;
	}

static void TestCount(unsigned count)
	{
	static sFONT *font = &Font12 ;
	static BOOL initialize = TRUE ;
	static unsigned xpos ;
	unsigned width ;
	char text[10] ;

	SetFontSize(font) ;
	SetForeground(FGND_NRML) ;
	SetBackground(BGND_NRML) ;
	if (initialize)
		{
		static char label[] = "Test Count: " ;
		width = (strlen(label) + 8) * font->Width ;
		xpos = (XPIXELS - width) / 2 ;
		DisplayStringAt(xpos, YPOS_TESTS, label) ;
		xpos += strlen(label) * font->Width ;
		initialize = FALSE ;
		}

	sprintf(text, "%08u", count) ;	
	DisplayStringAt(xpos, YPOS_TESTS, text) ;
	}

static void Message(char *msg)
	{
	static sFONT *font = &Font12 ;
	unsigned xpos, width ;

	SetFontSize(font) ;
	SetForeground(BGND_NRML) ;
	FillRect(0, YPOS_MSG, 240, font->Height) ;
	SetForeground(FGND_NRML) ;
	SetBackground(BGND_NRML) ;
	width = strlen(msg) * font->Width ;
	xpos = (XPIXELS - width) / 2 ;
	DisplayStringAt(xpos, YPOS_MSG, msg) ;
	}

static Q16 GetOperand(void)
	{
	int32_t random ;
	unsigned bits ;
	Q16 q16 ;

	random = GetRandomNumber() ;
	bits = 1 + (random & 0x1F) ;
	q16 = GetRandomNumber() & ((1 << bits) - 1) ;
	return random >= 0 ? q16 : -q16 ;
	}
	
static Q16 Correct(Q16 dividend, Q16 divisor)
	{
	return (Q16) ((((int64_t) dividend) << 16) / divisor) ;
	}

static unsigned DisplayAt(unsigned xpos, unsigned ypos, char *text, sFONT *font)
	{
	DisplayStringAt(xpos, ypos, text) ;
	return ypos + (3*font->Height)/2 ;
	}

static BOOL Results(Q16 dividend, Q16 divisor, Q16 quotient, Q16 correct)
	{
	static BOOL initialize = TRUE ;
	static sFONT *font = &Font12 ;
	static unsigned xpos, ytop ;
	unsigned ypos ;
	char text[100] ;
	BOOL error ;

	SetFontSize(font) ;

	if (initialize)
		{
		xpos = XLFT_WNDW + font->Width ;
		ytop = Window(YTOP_RSLT, 4, "Q16Divide", font) ;
		ypos = DisplayAt(xpos, ytop, " Dividend: ", font) ;
		ypos = DisplayAt(xpos, ypos, "  Divisor: ", font) ;
		ypos = DisplayAt(xpos, ypos, " Quotient: ", font) ;
		ypos = DisplayAt(xpos, ypos, "Reference: ", font) ;
		xpos += 11 * font->Width ;
		initialize = FALSE ;
		}

	SetForeground(FGND_WNDW) ;
	SetBackground(BGND_WNDW) ;

	sprintf(text, "%+10.3E (%08X)", FLOAT(dividend), (unsigned) dividend) ;
	ypos = DisplayAt(xpos, ytop, text, font) ;

	sprintf(text, "%+10.3E (%08X)", FLOAT(divisor),  (unsigned) divisor) ;
	ypos = DisplayAt(xpos, ypos, text, font) ;

	error = quotient != correct ;

	SetForeground(error ? FGND_ERR : FGND_WNDW) ;
	SetBackground(error ? BGND_ERR : BGND_WNDW) ;
	sprintf(text, "%+10.3E (%08X)", FLOAT(quotient), (unsigned) quotient) ;
	ypos = DisplayAt(xpos, ypos, text, font) ;

	SetForeground(error ? FGND_GOOD : FGND_WNDW) ;
	SetBackground(error ? BGND_GOOD : BGND_WNDW) ;
	sprintf(text, "%+10.3E (%08X)", FLOAT(correct), (unsigned) correct) ;
	DisplayAt(xpos, ypos, text, font) ;

	return error ;
	}

static void UpdateCycles(CYCLES *cyc, unsigned cycles)
	{
	if (cyc->num == 10000) return ;

	if (cycles < cyc->min)		cyc->min = cycles ;
	else if (cycles > cyc->max)	cyc->max = cycles ;

	cyc->ttl += cycles ;
	cyc->avg  = cyc->ttl / ++cyc->num ;
	}

static unsigned Window(unsigned ypos, unsigned rows, char *label, sFONT *font)
	{
	unsigned height ;
	char text[100] ;

	SetForeground(BGND_WNDW) ;
	height = ((3*rows + 2) * font->Height) / 2 ;
	FillRect(XLFT_WNDW, ypos, XSIZ_WNDW, height) ;
	SetForeground(BRDR_WNDW) ;
	DrawRect(XLFT_WNDW, ypos, XSIZ_WNDW, height) ;
	SetForeground(FGND_WNDW) ;
	SetBackground(BGND_WNDW) ;
	sprintf(text, "[%s]", label) ;
	return DisplayAt(XLBL_WNDW, ypos - font->Height/2, text, font) ;
	}

static void Performance(unsigned cyc_div, unsigned cyc_ref)
	{
	static CYCLES div = {0, 0, UINT32_MAX, 0, 0.0} ;
	static CYCLES ref = {0, 0, UINT32_MAX, 0, 0.0} ;
	static BOOL initialize = TRUE ;
	static sFONT *font = &Font12 ;
	static unsigned xpos, ytop ;
	char text[100] ;
	unsigned ypos ;

	SetFontSize(font) ;

	if (initialize)
		{
		xpos = XLFT_WNDW + font->Width ;
		ypos = Window(YTOP_PERF, 3, "Clock Cycles", font) ;
		ytop = DisplayAt(xpos, ypos, "            Cur   Min  Avg  Max", font) ;
		ypos = DisplayAt(xpos, ytop, "Q16Divide: ", font) ;
		ypos = DisplayAt(xpos, ypos, "Reference: ", font) ;
		xpos += 11 * font->Width ;
		initialize = FALSE ;
		}

	UpdateCycles(&div, cyc_div) ;
	UpdateCycles(&ref, cyc_ref) ;

	SetForeground(FGND_WNDW) ;
	SetBackground(BGND_WNDW) ;

	sprintf(text, "%4u  %4u %4u %4u", cyc_div, div.min, div.avg, div.max) ;
	ypos = DisplayAt(xpos, ytop, text, font) ;

	sprintf(text, "%4u  %4u %4u %4u", cyc_ref, ref.min, ref.avg, ref.max) ;
	DisplayAt(xpos, ypos, text, font) ;
	}

static BOOL Overflow(Q16 dividend, Q16 divisor)
	{
	if (divisor == 0) return TRUE ;
	int64_t quotient = (((int64_t) dividend) << 16) / divisor ;
	return quotient < INT32_MIN || INT32_MAX < quotient ;
	}

static void SetFontSize(sFONT *Font)
	{
	extern void BSP_LCD_SetFont(sFONT *) ;
	BSP_LCD_SetFont(Font) ;
	}
