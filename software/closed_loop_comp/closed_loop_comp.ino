#define HINA 3
#define LINA 5
#define HINB 9
#define LINB 8
#define HINC 10
#define LINC 2

#define MIN_OPEN_DELAY 2500

#define PWM_CYCLE 100
volatile uint16_t throttle = 20;

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
  TCCR1B = (1 << WGM12);                  // CTC
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
  if(step >= 6){step = 0;}
  OCR1A  = timer_1_delay;
  timer_1_delay -= 1;

  if(timer_1_delay <= MIN_OPEN_DELAY) {   // Switch to "closed loop" mode
    timer_1_delay = MIN_OPEN_DELAY;       
    TIMSK1 &= ~(1 << OCIEA1);             // Disable COMPA
  }
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