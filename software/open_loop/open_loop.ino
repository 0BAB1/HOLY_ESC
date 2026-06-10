/*
3-Clause BSD NON-AI License

Copyright 2025 BABIN-RIBY Hugo

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer
in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products
derived from this software without specific prior written permission.

4. The source code and the binary form, and any modifications made to them may not be used for the purpose of training or improving machine learning algorithms,
including but not limited to artificial intelligence, natural language processing, or data mining. This condition applies to any derivatives,
modifications, or updates based on the Software code. Any usage of the source code or the binary form in an AI-training dataset is considered a breach of this License.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES,
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#define HINA 3
#define LINA 5
#define HINB 9
#define LINB 8
#define HINC 10
#define LINC 2

#define MIN_OPEN_DELAY 2000
#define THROTTLE 10
#define COOLDOWN 20

volatile int step = 0;
volatile int wait = 0;
volatile int timer_1_delay = 4999;

void setup(){
  // ================
  // SETUP PINS
  // ================

  // Port D: set bits 2, 3, 6 as output
  DDRD |= (1 << PD2) | (1 << PD3) | (1 << PD5);
  // Port B: set bits 0, 1, 2 as output
  DDRB |= (1 << PB0) | (1 << PB1) | (1 << PB2);
  // all LOW initially
  PORTD = B00000000;
  PORTB = B00000000;

  // ================
  // SETUP Interrupts
  // ================

  // https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-7810-Automotive-Microcontrollers-ATmega328P_Datasheet.pdf
  // page 109-110
  // STEP ++ interrupt
  TCCR1A = 0;
  TCCR1B = (1 << WGM12) | (1 << CS11);  // CTC + prescaler /8
  OCR1A  = timer_1_delay;                        
  TIMSK1 = (1 << OCIE1A);

  // Timer2 setup - CTC, /8 prescaler, 8µs initial period
  TCCR2A = (1 << WGM21);              // CTC mode
  TCCR2B = (1 << CS21);               // /8 prescaler
  OCR2A  = THROTTLE;                         // 8µs (0-indexed)
  TIMSK2 = (1 << OCIE2A);             // enable compare match interrupt
}

ISR(TIMER1_COMPA_vect) {
  //
  // OPEN LOOP STEP ADVANCE INTERRUPT
  //
  step++;
  TCNT1 = 0;
  OCR1A  = timer_1_delay; 
  timer_1_delay -= 1;
  if(timer_1_delay <= MIN_OPEN_DELAY) {timer_1_delay = MIN_OPEN_DELAY;}
  if(step >= 6){step = 0;}
}

ISR(TIMER2_COMPA_vect){
  //
  // THROTTLE INTERRUPT
  //

  // TURN EVERYTHIN OFF -> DEADTIME FIX !
  PORTD = B00000000;
  PORTB = B00000000;

  if(wait == 0){
    wait = 1;
    TCNT2 = 0;
    OCR2A = COOLDOWN;
  } else {
    wait = 0;
    TCNT2 = 0;
    OCR2A = THROTTLE;    // 8µs
    //OCR2A = 8;    // 4µs
  }

  // This interrupt fires at very high frequencies
  // so we also use it to apply the step.
  switch(step){
    case 0: if(wait == 0){step0();} else{step01();} break;
    case 1: if(wait == 0){step1();} else{step11();} break;
    case 2: if(wait == 0){step2();} else{step21();} break;
    case 3: if(wait == 0){step3();} else{step31();} break;
    case 4: if(wait == 0){step4();} else{step41();} break;
    case 5: if(wait == 0){step5();} else{step51();} break;
  }
}

void step0(){
  // flow A to C
  PORTD = B00001100; // HINA (PD3), LINC (PD2)
  PORTB = B00000000;
}
void step01(){
  // GND A & C
  PORTD = B00000000; // LINA (PD5), LINC (PD2)
  PORTB = B00000000;
}
void step1(){
  // flow B to C
  PORTD = B00000100; // LINC (PD2)
  PORTB = B00000010; // HINB (PB1)
}
void step11(){
  // GND B & C
  PORTD = B00000000; // LINC (PD2)
  PORTB = B00000000; // LINB (PB0)
}
void step2(){
  // flow B to A
  PORTD = B00100000; // LINA (PD5)
  PORTB = B00000010; // HINB (PB1)
}
void step21(){
  // GND B & A
  PORTD = B00000000; // LINA (PD5)
  PORTB = B00000000; // LINB (PB0)
}
void step3(){
  // flow C to A
  PORTD = B00100000; // LINA (PD5)
  PORTB = B00000100; // HINC (PB2)
}
void step31(){
  // GND C & A
  PORTD = B00000000; // LINA (PD5), LINC (PD2)
  PORTB = B00000000;
}
void step4(){
  // flow C to B
  PORTD = B00000000;
  PORTB = B00000101; // HINC (PB2), LINB (PB0)
}
void step41(){
  // GND C & B
  PORTD = B00000000; // LINC (PD2)
  PORTB = B00000000; // LINB (PB0)
}
void step5(){
  // flow A to B
  PORTD = B00001000; // HINA (PD3)
  PORTB = B00000001; // LINB (PB0)
}
void step51(){
  // GND A & B
  PORTD = B00000000; // LINA (PD5)
  PORTB = B00000000; // LINB (PB0)
}

void loop(){
}