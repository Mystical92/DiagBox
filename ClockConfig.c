/*
 * ClockConfig.c
 *
 * Created: 2015-05-15 13:02:11
 *  Author: mysticaL-
 */ 

#include "ClockConfig.h"

bool OSC_wait_for_rdy(uint8_t clk)
{
	uint8_t czas=255;
	while ((!(OSC.STATUS & clk)) && (--czas)) // Czekaj na ustabilizowanie siê generatora
	_delay_ms(1);
	return czas;   //false jeœli generator nie wystartowa³, true jeœli jest ok
}

bool RC2M_en()
{
	OSC.CTRL |= OSC_RC2MEN_bm; //W³¹cz generator RC 32 MHz
	return OSC_wait_for_rdy(OSC_RC2MEN_bm); //Zaczekaj na jego poprawny start
}

bool RC32M_en()
{
	OSC.CTRL |= OSC_RC32MEN_bm; //W³¹cz generator RC 32 MHz
	return OSC_wait_for_rdy(OSC_RC32MEN_bm); //Zaczekaj na jego poprawny start
}