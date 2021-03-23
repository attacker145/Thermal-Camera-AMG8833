/*
 * HexToLong.c
 *
 *  Created on: Jan 13, 2017
 *      Author: rchak
 */

#include <string.h>
#include "stdint.h"

extern uint8_t Rx_buf[10];

/*
 * Convert uint32_t hex value to an uint8_t array. Use this function to convert year to character decimal string.
 */
void hexdec_long( uint32_t count )
{
    //uint8_t ones;
    //uint8_t tens;
    //uint8_t hundreds;
    //uint8_t thousands;
    //uint8_t thousand10s;
    //uint8_t thousand100s;
    //uint8_t mill;
    //uint8_t mill10s;
    //uint8_t mill100s;
    //uint8_t bills;

    char ones;
    char tens;
    char hundreds;
    char thousands;
    char thousand10s;
    char thousand100s;
    char mill;
    char mill10s;
    char mill100s;
    char bills;

	bills			= 0;
	mill100s		= 0;
	mill10s			= 0;
	mill			= 0;
	thousand100s 	= 0;
	thousand10s 	= 0;
	thousands 		= 0;
	hundreds 		= 0;
	tens  			= 0;
	ones 			= 0;

	while ( count >= 1000000000 )
	{
		count -= 1000000000;		// subtract 1000000000, one billion
		bills++;					// increment billions
	}
	while ( count >= 100000000 )
	{
		count -= 100000000;			// subtract 100000000, 100 million
		mill100s++;					// increment 100th millions
	}
	while ( count >= 10000000 )
	{
		count -= 10000000;			// subtract 10000000
		mill10s++;					// increment 10th millions
	}
	while ( count >= 1000000 )
	{
		count -= 1000000;			// subtract 1000000
		mill++;						// increment 1 millions
	}
	while ( count >= 100000 )
	{
		count -= 100000;			// subtract 100000
		thousand100s++;				// increment 100th thousands
	}
	while ( count >= 10000 )
	{
		count -= 10000;             // subtract 10000
		thousand10s++;				// increment 10th thousands
	}
	while ( count >= 1000 )
	{
		count -= 1000;				// subtract 1000
		thousands++;				// increment thousands
	}
	while ( count >= 100 )
	{
		count -= 100;               // subtract 100
		hundreds++;                 // increment hundreds
	}
	while ( count >= 10 )
	{
		count -= 10;				// subtract 10
		tens++;						// increment tens
	}
		ones = count;				// remaining count equals ones

	Rx_buf[0]= bills + 0x30;    //Conver HEX to character
	Rx_buf[1]= mill100s + 0x30;

    Rx_buf[2]= mill10s + 0x30;
    Rx_buf[3]= mill + 0x30;
    Rx_buf[4]= thousand100s + 0x30;
    Rx_buf[5]= thousand10s + 0x30;

    Rx_buf[6]= thousands + 0x30;
    Rx_buf[7]= hundreds + 0x30;

    Rx_buf[8]= tens   + 0x30;
    Rx_buf[9]= ones + 0x30;
    return;
}


void uchar_str( uint8_t count )
{
    uint8_t ones;
    uint8_t tens;
    uint8_t hundreds;

	hundreds 		= 0;
	tens  			= 0;
	ones 			= 0;


	while ( count >= 100 )
	{
		count -= 100;               // subtract 100
		hundreds++;                 // increment hundreds
	}
	while ( count >= 10 )
	{
		count -= 10;				// subtract 10
		tens++;						// increment tens
	}
		ones = count;				// remaining count equals ones


    Rx_buf[0]= hundreds + 0x30;
    Rx_buf[1]= tens   + 0x30;
    Rx_buf[2]= ones + 0x30;
    return;
}












