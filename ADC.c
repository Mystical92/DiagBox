/*
 * ADC.c
 *
 * Created: 2013-07-28 16:37:48
 *  Author: tmf
 */

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stddef.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdio.h>

#include "ADC.h"
#include "TxDMABuffer.h"
#include "DataProtocol.h"

#define ADC_PACKET_LENGHT (ADC_BUF_SIZE + 6)
//ADC_Result_t ADC_Res[2][ADC_BUF_SIZE];  //Bufor na dane

// Ramka gotowa do wys³ania [Nag³ówek, Rozkaz, D³ugoœæ, Dane (2*500), CRC]
uint8_t AdcPacket[2][ADC_PACKET_LENGHT];	

// strona zawieraj¹ca wyniki
volatile uint8_t ADC_Res_Page;	

void SendData() {
	AdcPacket[0][0] = PKT_HEADER; // Nag³ówek
	AdcPacket[1][0] = PKT_HEADER;

	AdcPacket[0][1] = 0x04; // Rozkaz
	AdcPacket[1][1] = 0x04; // Rozkaz

	AdcPacket[0][2] = 0x00; // D³ugoœæ danych STARSZY BAJT
	AdcPacket[0][3] = 0x64; // D³ugoœæ danych M£ODSZT BAJT

	AdcPacket[1][2] = 0x00; // D³ugoœæ danych STARSZY BAJT
	AdcPacket[1][3] = 0x64; // D³ugoœæ danych M£ODSZT BAJT
	
	uint16_t crc16 = doCRC16(*(AdcPacket + ADC_Res_Page), (ADC_PACKET_LENGHT - 2) );
	AdcPacket[ADC_Res_Page][ADC_PACKET_LENGHT - 2] = crc16 >> 8;
	AdcPacket[ADC_Res_Page][ADC_PACKET_LENGHT - 1] = crc16 & 0xFF;
	DMA_InitTransfer(*(AdcPacket + ADC_Res_Page), ADC_PACKET_LENGHT);
}

ISR(DMA_CH0_vect) {
	ADC_Res_Page=0;
	// skasuj flagê przerwania
	DMA_CH0_CTRLB=DMA_CH_TRNINTLVL_LO_gc | DMA_CH_TRNIF_bm;  
	SendData();
	PORTD_OUTCLR = PIN2_bm;
}

ISR(DMA_CH1_vect) {
	ADC_Res_Page=1;
	// skasuj flagê przerwania
	DMA_CH1_CTRLB=DMA_CH_TRNINTLVL_LO_gc | DMA_CH_TRNIF_bm;  
	SendData();
}

uint8_t ReadCalibrationByte(uint8_t index) {
	uint8_t result;
	// odczytaj sygnaturê produkcyjn¹
	NVM_CMD=NVM_CMD_READ_CALIB_ROW_gc; 
	result=pgm_read_byte(index);
	// przywróæ normalne dzia³anie NVM
	NVM_CMD=NVM_CMD_NO_OPERATION_gc;   
	return result;
}

void ADC_CH_Init(ADC_CH_t *adcch, register8_t muxpos) {
	// tryb pojedynczego wejœcia ze znakiem
	adcch->CTRL=ADC_CH_INPUTMODE_SINGLEENDED_gc;  
	// pin wejœcia dodatniego
	adcch->MUXCTRL=muxpos;                        
}

// generator zdarzenia wykonania pomiaru
void Timer_Init() {
	TCC0.CTRLB=TC_WGMODE_NORMAL_gc;			//Zwyk³y tryb pracy timera
	//TCC0.PER=3125;						//Dla 100ms
	TCC0.PER=1250;							//Dla 10ms
	TCC0.CCA=0;								//Zdarzenie z kana³u A co 1 sekundê
	EVSYS_CH0MUX=EVSYS_CHMUX_TCC0_CCA_gc;	//Routowane do kana³u zdarzeñ nr 0
	TCC0.CTRLA=TC_CLKSEL_OFF_gc;			//Taktowanie = 32MHz : 256 = 125000
}


void DMA_CH_Init(DMA_CH_t *DMA_CH, uint16_t DstAddr) {
	DMA_CH->ADDRCTRL =	DMA_CH_SRCDIR_INC_gc | DMA_CH_DESTDIR_INC_gc | 
						DMA_CH_SRCRELOAD_BURST_gc | DMA_CH_DESTRELOAD_BLOCK_gc;
	// transfer wyzwala zakoñczenie konwersji w CH0
	DMA_CH->TRIGSRC = DMA_CH_TRIGSRC_ADCA_CH0_gc; 
	DMA_CH->TRFCNT = ADC_BUF_SIZE;
	// powtarzamy w nieskoñczonoœæ
	DMA_CH->REPCNT = 0;  
	DMA_CH->SRCADDR0 = ((uint16_t)&ADCA_CH0RES) & 0xff;
	DMA_CH->SRCADDR1 = ((uint16_t)&ADCA_CH0RES) >> 8;
	DMA_CH->SRCADDR2 = 0;
	DMA_CH->DESTADDR0 = DstAddr & 0xff;
	DMA_CH->DESTADDR1 = DstAddr >> 8;
	DMA_CH->DESTADDR2 = 0;
	DMA_CH->CTRLB = DMA_CH_TRNINTLVL_HI_gc;
	// tryb single, transferujemy na raz 2 bajtów
	DMA_CH->CTRLA = DMA_CH_ENABLE_bm | DMA_CH_SINGLE_bm | DMA_CH_BURSTLEN_2BYTE_gc 
					| DMA_CH_REPEAT_bm; 
}

void DMA_Init() {
	DMA_CH_Init(&DMA.CH0, (uint16_t)&AdcPacket[0][4]);
	DMA_CH_Init(&DMA.CH1, (uint16_t)&AdcPacket[1][4]);
	// Odblokuj DMAC, podwójne buforowanie prze kana³y 0 i 1, round-robin
	DMA.CTRL=DMA_ENABLE_bm | DMA_DBUFMODE_CH01_gc;    
}

void ADC_Init() {
	// Wszystkie kana³y wyzwalaj¹ transfer DMA
	ADCA.CTRLA=ADC_ENABLE_bm;  
	// Rozdzielczoœæ 12 bitów, tryb ze znakiem
	ADCA.CTRLB=ADC_CONMODE_bm;
	// Referencja 1V                      
	ADCA.REFCTRL=ADC_REFSEL_INT1V_gc | ADC_BANDGAP_bm;
	//Wyzwalanie kana³ów 0 przez EVCH0
	ADCA.EVCTRL=ADC_EVSEL_0123_gc | ADC_SWEEP_0_gc | ADC_EVACT_SWEEP_gc; 
	ADCA.PRESCALER=ADC_PRESCALER_DIV32_gc;     //CLKADC=1 MHz
	// Kalibracja kana³ów ADC
	ADCA.CALL=ReadCalibrationByte(offsetof(NVM_PROD_SIGNATURES_t, ADCACAL0));
	ADCA.CALH=ReadCalibrationByte(offsetof(NVM_PROD_SIGNATURES_t, ADCACAL1));
	// Inicjacja poszczegolnych kanalow ADC
	ADC_CH_Init(&ADCA.CH0, ADC_CH_MUXPOS_PIN0_gc);   
	
	DMA_Init();
	Timer_Init();	
}

