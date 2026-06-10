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

#include <avr/io.h>

#define HINA 3
#define LINA 5
#define HINB 9
#define LINB 8
#define HINC 10
#define LINC 2

#define PHASE_A 3
#define PHASE_B 2
#define PHASE_C 1

#define MIN_OPEN_DELAY 2500

#define PWM_CYCLE 100
volatile uint16_t throttle = 10;

volatile uint8_t step = 0;
volatile uint8_t wait = 0;
volatile uint16_t timer_1_delay = 4999;

#define BLANKING_TICKS 0
volatile uint16_t step_ticks = 0;

volatile uint8_t closed_loop = 0;
volatile uint8_t zc_detected = 0;
volatile uint8_t first_time  = 1;
volatile uint16_t raw_period  = 0;
volatile unsigned long int last_period = 0;

#define PERIOD_FILTER_N 1
volatile uint16_t periods[PERIOD_FILTER_N];
volatile uint8_t period_ptr = 0;


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
  TCCR1B = (0 << WGM12) | (1 << CS11);
  OCR1A  = timer_1_delay;                       
  TIMSK1 = (1 << OCIE1A);

  // Timer2 setup - CTC, /8 prescaler, 8µs initial period
  TCCR2A &= ~(1 << WGM21);                // NO CTC mode
  TCCR2B = (1 << CS21);                   // /8 prescaler (1 tick = 0.5 us)
  OCR2A  = throttle;                      // 8µs (0-indexed)
  OCR2B  = PWM_CYCLE;                     
  TIMSK2 = (1 << OCIE2A) | (1 << OCIE2B); // enable compare match interrupt

  // init perioods digital filter
  for(int i = 0; i < PERIOD_FILTER_N; i++){
    periods[i] = MIN_OPEN_DELAY;
  }

  Serial.begin(115200);
}

ISR(TIMER1_COMPA_vect) {
  //
  // OPEN LOOP STEP ADVANCE INTERRUPT
  //
  TCNT1 = 0;
  step++;
  step_ticks = 0;
  if(step >= 6){step = 0;}
  timer_1_delay -= 1;
  OCR1A  = timer_1_delay;

  if(timer_1_delay <= MIN_OPEN_DELAY) {                         // Switch to "closed loop" mode (SETUP CLOSED LOOP)
    closed_loop = 1;

    // Disable COMPA (which is the open loop initial ramp us speed handler)
    timer_1_delay = MIN_OPEN_DELAY;       
    TIMSK1 &= ~(1 << OCIE1A);                                   // Disable COMPA, COMPB will be operated by ZC detection comp ISR
    TCCR1B &= ~(1 << WGM12);                                    // Remove CTC

    // Setup the comparator
    DIDR0 |= (1 << ADC1D) | (1 << ADC2D) | (1 << ADC3D);  // disable dig. inpt buffers on phase pins (reduce noise, more efficient)
    ADCSRB |= (1 << ACME);                                      // Enable comp mux
    ADCSRA &= ~(1 << ADEN);                                     // disable ADC, make MUX availabl to comp

    // Set ADMUX depending on the final ramp-up step
    switch(step){                                               
      case 0: case 3: ADMUX = PHASE_B; break;
      case 1: case 4: ADMUX = PHASE_A; break;
      case 2: case 5: ADMUX = PHASE_C; break;
    }

    // Set monitored EDGE
    switch(step & 1){                                               
      case 1: ACSR |= (1 << ACIS1) | (0 << ACIS0); break; // Falling Edge
      case 0: ACSR |= (1 << ACIS1) | (1 << ACIS0); break; // rising edge
    }

    // Make sure comp ISR is diabled by default, only COMPB2 cwill manage it
    ACSR |= (1 << ACI);           // Clear pendin interrupt if any
    ACSR &= ~(1<< ACIE);
    last_period = MIN_OPEN_DELAY; // init last_period to final ramp up value
  }
}

ISR(TIMER1_COMPB_vect) {
  //
  // CLOSED LOOP STEP ADVANCE INTERRUPT
  //
  step++;
  if(step >= 6){step = 0;}
  step_ticks = 0;
  zc_detected = 0;

  // Set ADMUX depending on the step
  switch(step){                                               
    case 0: case 3: ADMUX = PHASE_B; break;
    case 1: case 4: ADMUX = PHASE_A; break;
    case 2: case 5: ADMUX = PHASE_C; break;
  }

  // Set monitored EDGE (EDGE ORDER RELATED)
  switch(step & 1){                                               
    case 1: ACSR |= (1 << ACIS1) | (0 << ACIS0); break; // Falling Edge
    case 0: ACSR |= (1 << ACIS1) | (1 << ACIS0); break; // Rising edge
  }

  // Disble ISR 1B, COMP ISR will reenable it and take car of the scheduling
  TIMSK1 &= ~(1 << OCIE1B);
}

ISR(TIMER2_COMPA_vect){
  //
  // PWM (turn OFF) THROTTLE INTERRUPT
  //

  // TURN EVERYTHIN OFF for induced DEADTIME !
  PORTD = B00000000;
  PORTB = B00000000;

  // enforced dead time using NOPs
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");

  // Apply step
  switch(step){
    case 0: step01(); break;
    case 1: step11(); break;
    case 2: step21(); break;
    case 3: step31(); break;
    case 4: step41(); break;
    case 5: step51(); break;
  }

  // Enable comparator on PWM ON (if blanking period is over)
  if(step_ticks < BLANKING_TICKS || closed_loop == 0 || zc_detected == 1) return;
  ACSR |= (1<< ACI);  // clear eventual pending
  ACSR |= (1<< ACIE); // enable comp ISR
}

ISR(TIMER2_COMPB_vect) {
  //
  // PWM (turn ON) THROTTLE INTERRUPT
  //

  TCNT2 = 0;

  // TURN EVERYTHIN OFF for induced DEADTIME !
  PORTD = B00000000;
  PORTB = B00000000;

  // enforced dead time using NOPs
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");

  // Apply step
  switch(step){
    case 0: step0(); break;
    case 1: step1(); break;
    case 2: step2(); break;
    case 3: step3(); break;
    case 4: step4(); break;
    case 5: step5(); break;
  }

  step_ticks++;
  ACSR &= ~(1<< ACIE); // diable comp ISR during PMW OFF period
}

ISR(ANALOG_COMP_vect){
  // BEMF ZC DETECTED DURING PWM ON

  // Debounce (EDGE ORDER RELATED)
  for(uint8_t i = 0; i < 5; i ++){
    uint8_t zc_state = (ACSR >> ACO) & 1;
    switch(step & 1){                                              
      case 0: if(zc_state != 0) return; break; // odd step = falling, want ACO=0 after
      case 1: if(zc_state != 1) return; break;
    }
  }

  // If passed denouncing, marked this ZC as detected so it won't misfire before next zc
  zc_detected = 1;
  ACSR &= ~(1 << ACIE);   // Disable comp

  // Measure the raw period since last ZC and update last_period, unless it's the first time, TCNT1 is NOT gonna be valid !
  if(first_time == 0){
    raw_period = TCNT1;
    // reject samples that are implausibly far from current estimate (+- 25% tolerence)
    uint16_t upper = last_period + (last_period >> 2); // +25%
    uint16_t lower = (last_period > (last_period >> 2)) ? last_period - (last_period >> 2) : 0; // -25%, no underflow
    if(raw_period >= lower && raw_period <= upper){
        period_ptr++;
        if(period_ptr >= PERIOD_FILTER_N) period_ptr = 0;
        periods[period_ptr] = raw_period;
    }

    // apply period digital filter
    last_period = 0;
    for(int i = 0; i < PERIOD_FILTER_N; i ++){
      last_period += periods[i];
    }
    last_period /= PERIOD_FILTER_N;

  }else{
    first_time = 0;
  }
  TCNT1 = 0;

  // Schedule next commutation bnase on last_period
  OCR1B = last_period >> 1;  // Set TIMERB intr to 30e°, i.e. last_period / 2 (- some margin, experimenting)
  TIMSK1 |= (1 << OCIE1B);    // Enble TIMER1B ISR
}

void step0(){
  // flow A to C
  PORTD = B00001100; // HINA (PD3), LINC (PD2)
  PORTB = B00000000;
}
void step01(){
  // PWM off: keep LINC
  PORTD = B00000100; // LINC (PD2)
  PORTB = B00000000;
}
void step1(){
  // flow B to C
  PORTD = B00000100; // LINC (PD2)
  PORTB = B00000010; // HINB (PB1)
}
void step11(){
  // PWM off: keep LINC
  PORTD = B00000100; // LINC (PD2)
  PORTB = B00000000;
}
void step2(){
  // flow B to A
  PORTD = B00100000; // LINA (PD5)
  PORTB = B00000010; // HINB (PB1)
}
void step21(){
  // PWM off: keep LINA
  PORTD = B00100000; // LINA (PD5)
  PORTB = B00000000;
}
void step3(){
  // flow C to A
  PORTD = B00100000; // LINA (PD5)
  PORTB = B00000100; // HINC (PB2)
}
void step31(){
  // PWM off: keep LINA
  PORTD = B00100000; // LINA (PD5)
  PORTB = B00000000;
}
void step4(){
  // flow C to B
  PORTD = B00000000;
  PORTB = B00000101; // HINC (PB2), LINB (PB0)
}
void step41(){
  // PWM off: keep LINB
  PORTD = B00000000;
  PORTB = B00000001; // LINB (PB0)
}
void step5(){
  // flow A to B
  PORTD = B00001000; // HINA (PD3)
  PORTB = B00000001; // LINB (PB0)
}
void step51(){
  // PWM off: keep LINB
  PORTD = B00000000;
  PORTB = B00000001; // LINB (PB0)
}

void loop(){
  Serial.println(raw_period);
  delay(10);
}