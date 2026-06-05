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
volatile uint16_t throttle = 20;

volatile int step = 0;
volatile int wait = 0;
volatile int timer_1_delay = 4999;

#define BLANKING_TICKS 2
volatile int step_ticks = 0;

volatile int closed_loop = 0;
volatile int first_time  = 1;
volatile int last_period = 0;
volatile int raw_period  = 0;

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
  TCCR1B &= ~(1 << WGM12);                // NO CTC mode
  TCCR1B |= (1 << CS11);                  // /8 prescaler (1 tick = 0.5 us)
  OCR1A  = timer_1_delay;                        
  TIMSK1 = (1 << OCIE1A);

  // Timer2 setup - CTC, /8 prescaler, 8µs initial period
  TCCR2A &= ~(1 << WGM21);                // NO CTC mode
  TCCR2B = (1 << CS21);                   // /8 prescaler (1 tick = 0.5 us)
  OCR2A  = throttle;                      // 8µs (0-indexed)
  OCR2B  = PWM_CYCLE;                     
  TIMSK2 = (1 << OCIE2A) | (1 << OCIE2B); // enable compare match interrupt
}

ISR(TIMER1_COMPA_vect) {
  //
  // OPEN LOOP STEP ADVANCE INTERRUPT
  //
  TCNT1 = 0;
  step++;
  step_ticks = 0;
  if(step >= 6){step = 0;}
  OCR1A  = timer_1_delay;
  timer_1_delay -= 1;

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
    switch(step){                                               
      case 0: case 1: case 2: ACSR |= (1 << ACIS1) | (0 << ACIS0); break; // Falling Edge
      case 3: case 4: case 5: ACSR |= (1 << ACIS1) | (1 << ACIS0); break; // rising edge
    }

    // Make sure comp ISR is diabled by default, only COMPB2 cwill manage it
    ACSR |= (1 << ACI);           // Clear pendin interrupt if any
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

  // Set ADMUX depending on the step
  switch(step){                                               
    case 0: case 3: ADMUX = PHASE_B; break;
    case 1: case 4: ADMUX = PHASE_A; break;
    case 2: case 5: ADMUX = PHASE_C; break;
  }

  // Set monitored EDGE
  switch(step){                                               
    case 0: case 1: case 2: ACSR |= (1 << ACIS1) | (0 << ACIS0); break; // Falling Edge
    case 3: case 4: case 5: ACSR |= (1 << ACIS1) | (1 << ACIS0); break; // Rising edge
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

  // Enforced dead time 
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

  step_ticks++;

  ACSR &= ~(1<< ACIE); // diable comp ISR during PMW OFF period
}

ISR(TIMER2_COMPB_vect) {
  //
  // PWM (turn ON) THROTTLE INTERRUPT
  //

  TCNT2 = 0;

  // TURN EVERYTHIN OFF for induced DEADTIME !
  PORTD = B00000000;
  PORTB = B00000000;

  // Enforced dead time 
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

  // Enable comparator on PWM ON (if blanking period is over)
  if(step_ticks < BLANKING_TICKS || closed_loop == 0) return;
  ACSR |= (1<< ACI);  // clear eventual pending
  ACSR |= (1<< ACIE); // enable comp ISR
}

ISR(ANALOG_COMP_vect){
  // BEMF ZC DETECTED DURING PWM ON

  // Measure the raw period since last ZC and update last_period, unless it's the first time, TCNT1 is NOT gonna be valid !
  if(first_time == 0){
    raw_period = TCNT1;
    last_period = (raw_period + last_period) >> 1; // simple 2 sample filtering
  }
  first_time = 0;
  TCNT1 = 0;

  // Schedule next commutation bnase on last_period
  TIMSK1 |= (1 << OCIE1B);    // Enble TIMER1B ISR
  OCR1B = last_period >> 1;  // Set TIMERB intr to 30e°, i.e. last_period / 2
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