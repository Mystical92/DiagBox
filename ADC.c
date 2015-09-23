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

uint8_t AdcPacket[2][ADC_PACKET_LENGHT];	//Ramka gotowa do wys³ania [Nag³ówek, Rozkaz, D³ugoœæ, Dane (2*500), CRC]

// Na razie na sztywno
// Nag³ówek


volatile uint8_t ADC_Res_Page;								//Strona zawieraj¹ca wyniki

void SendData()
{
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

ISR(DMA_CH0_vect)
{
	ADC_Res_Page=0;
	DMA_CH0_CTRLB=DMA_CH_TRNINTLVL_LO_gc | DMA_CH_TRNIF_bm;  //Skasuj flagê przerwania
	SendData();
	PORTD_OUTCLR = PIN2_bm;

}

ISR(DMA_CH1_vect)
{
	ADC_Res_Page=1;
	DMA_CH1_CTRLB=DMA_CH_TRNINTLVL_LO_gc | DMA_CH_TRNIF_bm;  //Skasuj flagê przerwania
	SendData();

}

uint8_t ReadCalibrationByte(uint8_t index)
{
	uint8_t result;

	NVM_CMD=NVM_CMD_READ_CALIB_ROW_gc; //Odczytaj sygnaturê produkcyjn¹
	result=pgm_read_byte(index);

	NVM_CMD=NVM_CMD_NO_OPERATION_gc;   //Przywróæ normalne dzia³anie NVM
	return result;
}

void ADC_CH_Init(ADC_CH_t *adcch, register8_t muxpos)
{
	adcch->CTRL=ADC_CH_INPUTMODE_SINGLEENDED_gc;  //Tryb pojedynczego wejœcia ze znakiem
	adcch->MUXCTRL=muxpos;                        //Pin wejœcia dodatniego
}


void Timer_Init() // Generator zdarzenia wykonania pomiaru
{
	TCC0.CTRLB=TC_WGMODE_NORMAL_gc;			//Zwyk³y tryb pracy timera
	//TCC0.PER=3125;						//Dla 100ms
	TCC0.PER=1250;							//Dla 10ms
	TCC0.CCA=0;								//Zdarzenie z kana³u A co 1 sekundê
	EVSYS_CH0MUX=EVSYS_CHMUX_TCC0_CCA_gc;	//Routowane do kana³u zdarzeñ nr 0
	TCC0.CTRLA=TC_CLKSEL_OFF_gc;			//Taktowanie = 32MHz : 256 = 125000
}


void DMA_CH_Init(DMA_CH_t *DMA_CH, uint16_t DstAddr)
{
	DMA_CH->ADDRCTRL = DMA_CH_SRCDIR_INC_gc | DMA_CH_DESTDIR_INC_gc | DMA_CH_SRCRELOAD_BURST_gc | DMA_CH_DESTRELOAD_BLOCK_gc;
	DMA_CH->TRIGSRC = DMA_CH_TRIGSRC_ADCA_CH0_gc; //Transfer wyzwala zakoñczenie konwersji w CH0
	DMA_CH->TRFCNT = ADC_BUF_SIZE;
	DMA_CH->REPCNT = 0;  //Powtarzamy w nieskoñczonoœæ
	DMA_CH->SRCADDR0 = ((uint16_t)&ADCA_CH0RES) & 0xff;
	DMA_CH->SRCADDR1 = ((uint16_t)&ADCA_CH0RES) >> 8;
	DMA_CH->SRCADDR2 = 0;
	DMA_CH->DESTADDR0 = DstAddr & 0xff;
	DMA_CH->DESTADDR1 = DstAddr >> 8;
	DMA_CH->DESTADDR2 = 0;
	DMA_CH->CTRLB = DMA_CH_TRNINTLVL_HI_gc;
	DMA_CH->CTRLA = DMA_CH_ENABLE_bm | DMA_CH_SINGLE_bm | DMA_CH_BURSTLEN_2BYTE_gc | DMA_CH_REPEAT_bm; //Tryb single, transferujemy na raz 2 bajtów
}

void DMA_Init()
{
	DMA_CH_Init(&DMA.CH0, (uint16_t)&AdcPacket[0][4]);
	DMA_CH_Init(&DMA.CH1, (uint16_t)&AdcPacket[1][4]);
	DMA.CTRL=DMA_ENABLE_bm | DMA_DBUFMODE_CH01_gc;    //Odblokuj DMAC, podwójne buforowanie prze kana³y 0 i 1, round-robin
}

void ADC_Init()
{
	ADCA.CTRLA=ADC_ENABLE_bm; //| ADC_DMASEL_CH0123_gc;   //Wszystkie kana³y wyzwalaj¹ transfer DMA
	ADCA.CTRLB=ADC_CONMODE_bm;                         //Rozdzielczoœæ 12 bitów, tryb ze znakiem
	ADCA.REFCTRL=ADC_REFSEL_INT1V_gc | ADC_BANDGAP_bm; //Referencja 1V
	//ADCA.EVCTRL=ADC_SWEEP_0_gc | ADC_EVSEL0_bm | ADC_EVACT_SWEEP_gc; //Wyzwalanie kana³ów 0, 1, 2 i 3 przez EVCH0
	ADCA.EVCTRL=ADC_EVSEL_0123_gc | ADC_SWEEP_0_gc | ADC_EVACT_SWEEP_gc; //Wyzwalanie kana³ów 0 przez EVCH0
	ADCA.PRESCALER=ADC_PRESCALER_DIV32_gc;     //CLKADC=1 MHz
	ADCA.CALL=ReadCalibrationByte(offsetof(NVM_PROD_SIGNATURES_t, ADCACAL0));
	ADCA.CALH=ReadCalibrationByte(offsetof(NVM_PROD_SIGNATURES_t, ADCACAL1));

	ADC_CH_Init(&ADCA.CH0, ADC_CH_MUXPOS_PIN0_gc);   //Zainicjuj poszczególne kana³y ADC
	//ADC_CH_Init(&ADCA.CH1, ADC_CH_MUXPOS_PIN1_gc);
	//ADC_CH_Init(&ADCA.CH2, ADC_CH_MUXPOS_PIN2_gc);
	//ADC_CH_Init(&ADCA.CH3, ADC_CH_MUXPOS_PIN3_gc);
	DMA_Init();
	Timer_Init();	
}

