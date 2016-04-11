/**
  ***********************************************************************************************************************
  * @file    VS1053.c
  * @author  Piotr Sperka
  * @date    07.08.2015
  * @brief   This file provides VS1053 usage and control functions. Based on VS1003 library by Przemyslaw Stasiak.
  ***********************************************************************************************************************
*/

/** @addtogroup VS1053
  * @{
  */

//#include "vs1053b-patches-flac.h"
#include "vs1053.h"
#include "stdio.h"
#include "spi.h"
#include "osapi.h"

extern volatile uint32_t PIN_OUT;
extern volatile uint32_t PIN_OUT_SET;
extern volatile uint32_t PIN_OUT_CLEAR;
 
extern volatile uint32_t PIN_DIR;
extern volatile uint32_t PIN_DIR_OUTPUT;
extern volatile uint32_t PIN_DIR_INPUT;
 
extern volatile uint32_t PIN_IN;
 
extern volatile uint32_t PIN_0;
extern volatile uint32_t PIN_2;

ICACHE_FLASH_ATTR void VS1053_HW_init(){
 	spi_init(HSPI);
	spi_clock(HSPI, 4, 10); //2MHz
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, 3);
	PIN_DIR_OUTPUT |= (1<<RST_PIN)|(1<<CS_PIN)|(1<<XDCS_PIN);
	PIN_DIR_INPUT |= (1<<DREQ_PIN);
	PIN_OUT_SET |= (1<<RST_PIN)|(1<<CS_PIN)|(1<<XDCS_PIN);
}

ICACHE_FLASH_ATTR void VS1053_SPI_SpeedUp()
{
	spi_clock(HSPI, 4, 3); //10MHz
}

ICACHE_FLASH_ATTR void VS1053_SPI_SpeedDown() {
//	spi_clock(HSPI, 4, 10); //2MHz
	spi_clock(HSPI, 4, 10); //2MHz
}

ICACHE_FLASH_ATTR void SPIPutChar(uint8_t data){
	spi_tx8(HSPI, data);
	while(spi_busy(HSPI));
}

ICACHE_FLASH_ATTR uint8_t SPIGetChar(){
	while(spi_busy(HSPI));
	return spi_rx8(HSPI);
}

ICACHE_FLASH_ATTR void Delay(uint32_t nTime)
{
	unsigned int i;
	unsigned long j;
	for(i = nTime;i > 0;i--)
		for(j = 1000;j > 0;j--);
}

ICACHE_FLASH_ATTR void ControlReset(uint8_t State){
	if(State) PIN_OUT_CLEAR |= (1<<RST_PIN);
	else PIN_OUT_SET |= (1<<RST_PIN);
}

ICACHE_FLASH_ATTR void SCI_ChipSelect(uint8_t State){
	if(State) PIN_OUT_CLEAR |= (1<<CS_PIN);
	else PIN_OUT_SET |= (1<<CS_PIN);
}

ICACHE_FLASH_ATTR void SDI_ChipSelect(uint8_t State){
	if(State) PIN_OUT_CLEAR |= (1<<XDCS_PIN);
	else PIN_OUT_SET |= (1<<XDCS_PIN);
}

ICACHE_FLASH_ATTR uint8_t VS1053_checkDREQ() {
	if(PIN_IN & (1<<DREQ_PIN)) return 1;
	else return 0;
}

ICACHE_FLASH_ATTR void VS1053_SineTest(){
	ControlReset(SET);
	VS1053_ResetChip();
	Delay(1000);
	SPIPutChar(0xff);

	SCI_ChipSelect(RESET);
	SDI_ChipSelect(RESET);
	ControlReset(RESET);

	Delay(500);

	VS1053_WriteRegister(SPI_MODE,0x08,0x20);
	Delay(500);

	while(VS1053_checkDREQ() == 0);

	SDI_ChipSelect(SET);
	SPIPutChar(0x53);
	SPIPutChar(0xef);
	SPIPutChar(0x6e);
	SPIPutChar(0x03); //0x24
	SPIPutChar(0x00);
	SPIPutChar(0x00);
	SPIPutChar(0x00);
	SPIPutChar(0x00);
	Delay(1000);
	SDI_ChipSelect(RESET);

}

ICACHE_FLASH_ATTR void VS1053_WriteRegister(uint8_t addressbyte, uint8_t highbyte, uint8_t lowbyte)
{
	spi_take_semaphore();
	VS1053_SPI_SpeedDown();
	SDI_ChipSelect(RESET);
	while(VS1053_checkDREQ() == 0);
	SCI_ChipSelect(SET);
	SPIPutChar(VS_WRITE_COMMAND);
	SPIPutChar(addressbyte);
	SPIPutChar(highbyte);
	SPIPutChar(lowbyte);
	while(VS1053_checkDREQ() == 0);
	SCI_ChipSelect(RESET);
	VS1053_SPI_SpeedUp();
	spi_give_semaphore();
}

ICACHE_FLASH_ATTR uint16_t VS1053_ReadRegister(uint8_t addressbyte){
//    while(!spi_take_semaphore());
	spi_take_semaphore();
	VS1053_SPI_SpeedDown();
	uint16_t result;
	SDI_ChipSelect(RESET);
	while(VS1053_checkDREQ() == 0);
	SCI_ChipSelect(SET);
	SPIPutChar(VS_READ_COMMAND);
	SPIPutChar(addressbyte);
	result = SPIGetChar() << 8;
	result |= SPIGetChar();
	while(VS1053_checkDREQ() == 0);
	SCI_ChipSelect(RESET);
	VS1053_SPI_SpeedUp();
	spi_give_semaphore();
	return result;
}

ICACHE_FLASH_ATTR void VS1053_ResetChip(){
	ControlReset(SET);
	Delay(500);
	SPIPutChar(0xff);
	SCI_ChipSelect(RESET);
	SDI_ChipSelect(RESET);
	ControlReset(RESET);
	Delay(100);
	while(VS1053_checkDREQ() == 0);
	Delay(100);
}

ICACHE_FLASH_ATTR uint16_t MaskAndShiftRight(uint16_t Source, uint16_t Mask, uint16_t Shift){
	return ( (Source & Mask) >> Shift );
}

ICACHE_FLASH_ATTR void VS1053_regtest()
{
	int MP3Status = VS1053_ReadRegister(SPI_STATUS);
	int MP3Mode = VS1053_ReadRegister(SPI_MODE);
	int MP3Clock = VS1053_ReadRegister(SPI_CLOCKF);
	int vsVersion ;
	printf("SCI_Mode (0x4800) = 0x%X\r\n",MP3Mode);
	printf("SCI_Status (0x48) = 0x%X\r\n",MP3Status);

	vsVersion = (MP3Status >> 4) & 0x000F; //Mask out only the four version bits
	printf("VS Version (VS1053 is 4) = %d\r\n",vsVersion);
	//The 1053B should respond with 4. VS1001 = 0, VS1011 = 1, VS1002 = 2, VS1053 = 3

	printf("SCI_ClockF = 0x%X\r\n",MP3Clock);
	printf("SCI_ClockF = 0x%X\r\n",MP3Clock);
}
/*
void VS1053_PluginLoad()
{
	int i;
  for (i=0;i<CODE_SIZE;i++) {
    VS1053_WriteRegister(atab[i], (dtab[i]>>8), (dtab[i]&0xff));
  }
}
*/
ICACHE_FLASH_ATTR void VS1053_Start(){
	VS1053_ResetChip();
	Delay(100);
// these 4 lines makes board to run on mp3 mode, no soldering required anymore
	VS1053_WriteRegister(SPI_WRAMADDR, 0xc0,0x17); //address of GPIO_DDR is 0xC017
	VS1053_WriteRegister(SPI_WRAM, 0x00,0x03); //GPIO_DDR=3
	VS1053_WriteRegister(SPI_WRAMADDR, 0xc0,0x19); //address of GPIO_ODATA is 0xC019
	VS1053_WriteRegister(SPI_WRAM, 0x00,0x00); //GPIO_ODATA=0
	Delay(100);
	while(VS1053_checkDREQ() == 0);
	VS1053_WriteRegister(SPI_CLOCKF,0x60,0x00);
//	VS1053_WriteRegister(SPI_MODE, (SM_LINE1 | SM_SDINEW)>>8 , SM_RESET); // soft reset
	VS1053_SoftwareReset();

	VS1053_WriteRegister(SPI_MODE, SM_SDINEW>>8, SM_LAYER12); //mode 
	while(VS1053_checkDREQ() == 0);
	VS1053_regtest();
}

ICACHE_FLASH_ATTR int VS1053_SendMusicBytes(uint8_t* music, uint16_t quantity){
	if(quantity < 1) return 0;
//	while(!spi_take_semaphore());
	spi_take_semaphore();
	while(VS1053_checkDREQ() == 0);
	SDI_ChipSelect(SET);
	int o = 0;
	while(quantity)
	{
		if(VS1053_checkDREQ()) {
			int t = quantity;
			int k;
			if(t > 32) t = 32;
			for (k=o; k < o+t; k++)
			{
				SPIPutChar(music[k]);
			}
			o += t;
			quantity -= t;
		}
	}
	SDI_ChipSelect(RESET);
	spi_give_semaphore();
	return o;
}

ICACHE_FLASH_ATTR void VS1053_SoftwareReset(){
	VS1053_WriteRegister(SPI_MODE, SM_SDINEW>>8,SM_RESET);
	VS1053_WriteRegister(SPI_MODE, SM_SDINEW>>8, SM_LAYER12); //mode 
}

ICACHE_FLASH_ATTR uint8_t VS1053_GetVolume(){
	return ( VS1053_ReadRegister(SPI_VOL) & 0x00FF );
}
/**
 * Function sets the same volume level to both channels.
 * @param xMinusHalfdB describes damping level as a multiple
 * 		of 0.5dB. Maximum volume is 0 and silence is 0xFEFE.
 */
ICACHE_FLASH_ATTR void VS1053_SetVolume(uint8_t xMinusHalfdB){
	VS1053_WriteRegister(SPI_VOL,xMinusHalfdB,xMinusHalfdB);
}

/**
 * Functions returns level of treble enhancement.
 * @return Returned value describes enhancement in multiplies
 * 		of 1.5dB. 0 value means no enhancement, 8 max (12dB).
 */
ICACHE_FLASH_ATTR uint8_t	VS1053_GetTreble(){
	return ( (VS1053_ReadRegister(SPI_BASS) & 0xF000) >> 12);
}

/**
 * Sets treble level.
 * @note If xOneAndHalfdB is greater than max value, sets treble
 * 		to maximum.
 * @param xOneAndHalfdB describes level of enhancement. It is a multiplier
 * 		of 1.5dB. 0 - no enhancement, 8 - maximum, 12dB.
 * @return void
 */
ICACHE_FLASH_ATTR void VS1053_SetTreble(uint8_t xOneAndHalfdB){
	uint16_t bassReg = VS1053_ReadRegister(SPI_BASS);
	if ( xOneAndHalfdB <= 8)
		VS1053_WriteRegister( SPI_BASS, MaskAndShiftRight(bassReg,0x0F00,8) | (xOneAndHalfdB << 4), bassReg & 0x00FF );
	else
		VS1053_WriteRegister( SPI_BASS, MaskAndShiftRight(bassReg,0x0F00,8) | 0x80, bassReg & 0x00FF );
}

/**
 * Sets low limit frequency of treble enhancer.
 * @note new frequency is set only if argument is valid.
 * @param xkHz The lowest frequency enhanced by treble enhancer.
 * 		Values from 0 to 15 (in kHz)
 * @return void
 */
ICACHE_FLASH_ATTR void VS1053_SetTrebleFreq(uint8_t xkHz){
	uint16_t bassReg = VS1053_ReadRegister(SPI_BASS);
	if ( xkHz <= 15 )
		VS1053_WriteRegister( SPI_BASS, MaskAndShiftRight(bassReg,0xF000,8) | xkHz, bassReg & 0x00FF );
}

/**
 * Returns level of bass boost in dB.
 * @return Value of bass enhancement from 0 (off) to 15(dB).
 */
ICACHE_FLASH_ATTR uint8_t	VS1053_GetBass(){
	return ( (VS1053_ReadRegister(SPI_BASS) & 0x00F0) >> 4);
}

/**
 * Sets bass enhancement level (in dB).
 * @note If xdB is greater than max value, bass enhancement is set to its max (15dB).
 * @param xdB Value of bass enhancement from 0 (off) to 15(dB).
 * @return void
 */
ICACHE_FLASH_ATTR void VS1053_SetBass(uint8_t xdB){
	uint16_t bassReg = VS1053_ReadRegister(SPI_BASS);
	if (xdB <= 15)
		VS1053_WriteRegister(SPI_BASS, (bassReg & 0xFF00) >> 8, (bassReg & 0x000F) | (xdB << 4) );
	else
		VS1053_WriteRegister(SPI_BASS, (bassReg & 0xFF00) >> 8, (bassReg & 0x000F) | 0xF0 );
}

/**
 * Sets low limit frequency of bass enhancer.
 * @note new frequency is set only if argument is valid.
 * @param xTenHz The lowest frequency enhanced by bass enhancer.
 * 		Values from 2 to 15 ( equal to 20 - 150 Hz).
 * @return void
 */
ICACHE_FLASH_ATTR void VS1053_SetBassFreq(uint8_t xTenHz){
	uint16_t bassReg = VS1053_ReadRegister(SPI_BASS);
	if (xTenHz >=2 && xTenHz <= 15)
		VS1053_WriteRegister(SPI_BASS, MaskAndShiftRight(bassReg,0xFF00,8), (bassReg & 0x00F0) | xTenHz );
}

ICACHE_FLASH_ATTR uint16_t VS1053_GetDecodeTime(){
	return VS1053_ReadRegister(SPI_DECODE_TIME);
}

ICACHE_FLASH_ATTR uint16_t VS1053_GetBitrate(){
	uint16_t bitrate = (VS1053_ReadRegister(SPI_HDAT0) & 0xf000) >> 12;
	uint8_t ID = (VS1053_ReadRegister(SPI_HDAT1) & 0x18) >> 3;
	uint16_t res;
	if (ID == 3)
	{	res = 32;
		while(bitrate>13)
		{
			res+=64;
			bitrate--;
		}
		while (bitrate>9)
		{
			res+=32;
			bitrate--;
		}
		while (bitrate>5)
		{
			res+=16;
			bitrate--;
		}
		while (bitrate>1)
		{
			res+=8;
			bitrate--;
		}
	}
	else
	{	res = 8;

		while (bitrate>8)
		{
			res+=16;
			bitrate--;
		}
		while (bitrate>1)
		{
			res+=8;
			bitrate--;
		}
	}
	return res;
}

ICACHE_FLASH_ATTR uint16_t VS1053_GetSampleRate(){
	return (VS1053_ReadRegister(SPI_AUDATA) & 0xFFFE);
}
