// 7 segment display driver for JY-MCU module based on TM1650 chip
// Copyright (c) 2015 Anatoli Arkhipenko
//
// Changelog:
//     2015-02-24 - Initial release 
//	   2015-04-27 - Added support of program memery (PROGMEM) to store the ASCII to Segment Code table 

/* ============================================
7 segment display driver for JY-MCU module based on TM1650 chip
Copyright (c) 2015 Anatoli Arkhipenko

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
===============================================
*/

#include "TM1650.h"

/** Constructor, uses default values for the parameters
 * so could be called with no parameters.
 * aNumDigits - number of display digits (default = 4)
 */
TM1650::TM1650(int aNumDigits) {
	iNumDigits =  aNumDigits > TM1650_NUM_DIGITS ? TM1650_NUM_DIGITS : aNumDigits;
}

/** Initialization
 * initializes the driver. Turns display on, but clears all digits.
 */
void TM1650::init() {
	iPosition = NULL;
	for (int i=0; i<iNumDigits; i++) {
		iBuffer[i] = 0;
		iCtrl[i] = 0;
	}
    Wire.beginTransmission(TM1650_DISPLAY_BASE);
    iActive = (Wire.endTransmission() == 0);
	clear();
	displayOn();
}

/** Set brightness of all digits equally
 * aValue - brightness value with 1 being the lowest, and 7 being the brightest
 */
void TM1650::setBrightness(int aValue) {
	if (!iActive) return;
	if (aValue < 0) aValue = 0;
	if (aValue > TM1650_MAX_BRIGHT) aValue = TM1650_MAX_BRIGHT;

	for (int i=0; i<iNumDigits; i++) {
		Wire.beginTransmission(TM1650_DCTRL_BASE+i);
		iCtrl[i] = (iCtrl[i] & TM1650_MSK_BRIGHT) | ( aValue << TM1650_BRIGHT_SHIFT );
		Wire.write((byte) iCtrl[i]);
		Wire.endTransmission();
	}
}


/** Turns display on or off according to aState
 */
void TM1650::displayState (bool aState)
{
  if (aState) displayOn ();
  else displayOff();
}

/** Turns the display on
 */
void TM1650::displayOn ()
// turn all digits on
{
  if (!iActive) return;
  for (int i=0; i<iNumDigits; i++) {
    Wire.beginTransmission(TM1650_DCTRL_BASE+i);
	iCtrl[i] = (iCtrl[i] & TM1650_MSK_ONOFF) | TM1650_BIT_DOT;
    Wire.write((byte) iCtrl[i]);
    Wire.endTransmission();
  }
}
/** Turns the display off
 */
void TM1650::displayOff ()
// turn all digits off
{
  if (!iActive) return;
  for (int i=0; i<iNumDigits; i++) {
    Wire.beginTransmission(TM1650_DCTRL_BASE+i);
	iCtrl[i] = (iCtrl[i] & TM1650_MSK_ONOFF);
    Wire.write((byte) iCtrl[i]);
    Wire.endTransmission();
  }
}

/** Directly write to the CONTROL register of the digital position
 * aPos = position to set the control register for
 * aValue = value to write to the position
 *
 * Internal control buffer is updated as well
 */
void TM1650::controlPosition(int aPos, byte aValue) {
	if (!iActive) return;
	if (aPos >= 0 && aPos < iNumDigits) {
	    Wire.beginTransmission(TM1650_DCTRL_BASE+aPos);
	    iCtrl[aPos] = aValue;
		Wire.write(aValue);
	    Wire.endTransmission();
	}
}

/** Directly write to the digit register of the digital position
 * aPos = position to set the digit register for
 * aValue = value to write to the position
 *
 * Internal position buffer is updated as well
 */
void TM1650::setPosition(int aPos, byte aValue) {
	if (!iActive) return;
	if (aPos >= 0 && aPos < iNumDigits) {
	    Wire.beginTransmission(TM1650_DISPLAY_BASE+aPos);
	    iBuffer[aPos] = aValue;
	    Wire.write(aValue);
	    Wire.endTransmission();
	}
}

/** Directly set/clear a 'dot' next to a specific position
 * aPos = position to set/clear the dot for
 * aState = display the dot if true, clear if false
 *
 * Internal buffer is updated as well
 */
void	TM1650::setDot(int aPos, bool aState) {
	iBuffer[aPos] = iBuffer[aPos] & 0x7F |(aState ? 0b10000000 : 0);
	setPosition(aPos, iBuffer[aPos]);
}

/** Clear all digits. Keep the display on.
 */
void TM1650::clear()
// clears all digits
{
  if (!iActive) return;
  for (int i=0; i<iNumDigits; i++) {
    Wire.beginTransmission(TM1650_DISPLAY_BASE+i);
 	iBuffer[i] = 0;
	Wire.write((byte) 0);
    Wire.endTransmission();
  }
}

/** Display string on the display 
 * aString = character array to be displayed
 *
 * Internal buffer is updated as well
 * Only first N positions of the string are displayed if
 *  the string is longer than the number of digits
 */
void TM1650::displayString(char *aString)
{
	if (!iActive) return;
	for (int i=0; i<iNumDigits; i++) {
	  byte a = ((byte) aString[i]) & 0b01111111;
	  byte dot = ((byte) aString[i]) & 0b10000000;
#ifndef TM1650_USE_PROGMEM	  
	  iBuffer[i] = TM1650_CDigits[a];
#else
	  iBuffer[i] = pgm_read_byte_near(TM1650_CDigits + a);
#endif
	  if (a) {
	    Wire.beginTransmission(TM1650_DISPLAY_BASE+i);
	    Wire.write(iBuffer[i] | dot);
	    Wire.endTransmission();
	  }
	  else
	    break;
	    
	}
}

/** Display string on the display in a running fashion
 * aString = character array to be displayed
 *
 * Starts with first N positions of the string.
 * Subsequent characters are displayed with 1 char shift each time displayRunningShift() is called
 *
 * returns: number of iterations remaining to display the whole string
 */
int TM1650::displayRunning(char *aString) {

	strncpy(iString, aString, TM1650_MAX_STRING+1);
	iPosition = iString;
	iString[TM1650_MAX_STRING] = '\0'; //just in case.
    	displayString(iPosition);

	int l = strlen(iPosition);
	if (l <= iNumDigits) return 0;
	return (l - iNumDigits);
}

/** Display next segment (shifting to the left) of the string set by displayRunning()
 * Starts with first N positions of the string.
 * Subsequent characters are displayed with 1 char shift each time displayRunningShift is called
 *
 * returns: number of iterations remaining to display the whole string
 */
int TM1650::displayRunningShift() {
    	if (strlen(iPosition) <= iNumDigits) return 0;
    	displayString(++iPosition);
	return (strlen(iPosition) - iNumDigits);
}

