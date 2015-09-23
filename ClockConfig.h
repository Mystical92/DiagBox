/*
 * Clock.h
 *
 * Created: 2015-05-15 13:02:54
 *  Author: mysticaL-
 */ 


#ifndef CLOCK_H_
#define CLOCK_H_

#include <avr/io.h>
#include <util/delay.h>
#include <stdbool.h>

bool OSC_wait_for_rdy(uint8_t clk);
bool RC2M_en();
bool RC32M_en();


#endif /* CLOCK_H_ */