/**************************************************************************/
/*!
    @file     HardwarePWM.cpp
    @author   hathach

    @section LICENSE

    Software License Agreement (BSD License)

    Copyright (c) 2017, Adafruit Industries (adafruit.com)
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:
    1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
    3. Neither the name of the copyright holders nor the
    names of its contributors may be used to endorse or promote products
    derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ''AS IS'' AND ANY
    EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
/**************************************************************************/

#include "HardwarePWM.h"

HardwarePWM PWM0(NRF_PWM0);
HardwarePWM PWM1(NRF_PWM1);
HardwarePWM PWM2(NRF_PWM2);

HardwarePWM* PWMx[3] = { &PWM0, &PWM1, &PWM2 };

HardwarePWM::HardwarePWM(NRF_PWM_Type* pwm)
{
  _pwm = pwm;

  _count = 0;
  arrclr(_seq0);

  // Initialize Registers
  _pwm->ENABLE          = 0;
  _pwm->MODE            = PWM_MODE_UPDOWN_Up;
  _pwm->COUNTERTOP      = bit(8) - 1; // default is 8 bit like STD Arduino
  _pwm->PRESCALER       = PWM_PRESCALER_PRESCALER_DIV_1; // 16MHz
  _pwm->DECODER         = PWM_DECODER_LOAD_Individual;
  _pwm->LOOP            = 0;

  _pwm->SEQ[0].PTR      = (uint32_t) _seq0;
  _pwm->SEQ[0].CNT      = MAX_CHANNELS; // default mode is Individual --> count must be 4
  _pwm->SEQ[0].REFRESH  = 0;
  _pwm->SEQ[0].ENDDELAY = 0;

  _pwm->SEQ[1].PTR      = 0;
  _pwm->SEQ[1].CNT      = 0;
  _pwm->SEQ[1].REFRESH  = 0;
  _pwm->SEQ[1].ENDDELAY = 0;

  for(int i=0; i<MAX_CHANNELS; i++) _pwm->PSEL.OUT[i] = 0xFFFFFFFF;
}

void HardwarePWM::printInfo(void)
{
  PRINT_HEX( (uint32_t) _pwm);

  PRINT_INT(_pwm->ENABLE);
  PRINT_INT(_pwm->MODE);
  PRINT_INT(_pwm->COUNTERTOP);
  PRINT_INT(_pwm->PRESCALER);
  PRINT_INT(_pwm->DECODER);
  PRINT_INT(_pwm->LOOP);
  Serial.println();

  PRINT_HEX(_pwm->SEQ[0].PTR);
  PRINT_INT(_pwm->SEQ[0].CNT);
  PRINT_INT(_pwm->SEQ[0].REFRESH);
  PRINT_INT(_pwm->SEQ[0].ENDDELAY);
  Serial.println();

  PRINT_INT(_count);
  for(int i=0; i<MAX_CHANNELS; i++)
  {
    cprintf("%d: pin = %d, value = %04X\n", i, _pwm->PSEL.OUT[i], _seq0[i]);
  }
  Serial.println();
}

void HardwarePWM::setResolution(uint8_t bitnum)
{
  bitnum = min8(bitnum, 15); // max is 15 bit
  _pwm->COUNTERTOP = bit(bitnum) - 1;
}

bool HardwarePWM::addPin(uint8_t pin)
{
  VERIFY( isPinValid(pin) && (_count < MAX_CHANNELS) );

  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);

  if ( enabled() )
  {
    // disable before changing PSEL
    disable();
    _pwm->PSEL.OUT[_count++] = pin;
    enable();
  }else
  {
    _pwm->PSEL.OUT[_count++] = pin;
  }

  return true;
}

void HardwarePWM::start(void)
{
  // update sequence count
  _pwm->SEQ[0].CNT = MAX_CHANNELS;

  // Unmark stopped
//  _pwm->EVENTS_STOPPED = 0;

  // start sequence
  _pwm->TASKS_SEQSTART[0] = 1;
}

void HardwarePWM::stop(void)
{
  // Already stopped
//  if (_pwm->EVENTS_STOPPED) return ;

  _pwm->EVENTS_STOPPED = 0;
  _pwm->TASKS_STOP = 1;

  while( !_pwm->EVENTS_STOPPED ) yield();
}

bool HardwarePWM::writeChannel(uint8_t ch, uint16_t value, bool inverted )
{
  VERIFY( ch < _count );

  _seq0[ch] = value | (inverted ? 0 : bit(15));
  return true;
}

bool HardwarePWM::writePin(uint8_t pin, uint16_t value, bool inverted)
{
  int ch = pin2channel(pin);
  VERIFY( ch >= 0 );

  return writeChannel(ch, value, inverted);
}
