#include<avr/io.h>
#include<avr/interrupt.h>
#include<stdint.h>
#include<util/delay.h>
#define SET(PORT,BIT1,BIT2) PORT|=((1<<BIT1)|(1<<BIT2))
#define CLEAR(PORT,BIT1,BIT2) PORT&=~((1<<BIT1)|(1<<BIT2))
#define SET_BIT(PORT,BIT)  PORT |= (1<<BIT)
#define CLR_BIT(PORT,BIT)  PORT &= ~(1<<BIT)
#define F_CPU 16000000
#define MIN_PWM 4     // in ms (change this according to your servo)
#define MAX_PWM 1    // in ms (change this according to your servo)
#define PWM_FREQUENCY 100  // in Hz
#define PRESCALAR 256
#define UNIT_PULSE ((F_CPU/(PRESCALAR*PWM_FREQUENCY) - 1)/20)  // timer counts for 1ms
#define ENGINE PD2
#define OVER_STEER PB2
#define ENGINE_LED PD4
#define UNDER_STEER PB3
#define adcpin 0
#define adcpin1 1
#define adcpin2 2
#define adcpin3 3
#define adcpin3 3

uint16_t vehicle_Speed;
uint16_t Steering_Angle;
uint16_t Yaw_Angle;
uint16_t difference_Angle;
uint16_t col_sensor;
uint16_t ADC_value;
uint8_t count;
uint16_t adc_input=0;


struct{
  volatile unsigned int FLAG1:1;
  volatile unsigned int FLAG2:1;
  volatile unsigned int FLAG_ENG:1;;
   volatile unsigned int BRAKE_ENG:1;
}FLAG_BIT;

uint16_t value=0x0000;
int main()
{
  //FLAG_BIT.BRAKE=0;
  Serial.begin(9600);
  DDRD|=(1<<PD5);
  DDRD&=~(1<<PD3);
  EICRA|=(1<<ISC10);
  EICRA&=~(1<<ISC11);
  EIMSK|=(1<<INT1);//local interrupt enable
  SET_BIT(DDRB,PB2);
  CLR_BIT(DDRD,PD3);
  SET_BIT(DDRB,PB3);
  SET(DDRD,PD7,PD6);
  CLR_BIT(DDRD,PD2);
  CLEAR(PORTD,PD7,PD6);
  EICRA|=(1<<ISC00);
  EIMSK|=(1<<INT0);
  EICRA|=(1<<ISC10);
  //EICRA&=~(1<<ISC11);
  EIMSK|=(1<<INT1);//local interrupt enable
  sei();
  FLAG_BIT.FLAG1=0;
  FLAG_BIT.FLAG2=0;

  while(1)
  {
    PORTD&=~(1<<ENGINE_LED);
    if(FLAG_BIT.FLAG_ENG==0)
    {
      //Serial.println(FLAG_BIT.FLAG_ENG);
      CLR_BIT(PORTD,PD5);
    }
    if(FLAG_BIT.FLAG1==1)
    { 
      PORTD|=(1<<ENGINE_LED);
      ADCSRA|=(1<<ADEN);
      cli();
      Serial.begin(9600);
      InitADC();
      DDRB |= (1 <<PB1);// Set Angle here                 
                  //DDRB = (1 << 1);  // Set PB.1 as output
                  init_pwm();
      adc_input= ReadADC(4);
      Serial.println(adc_input);
      adc_input=adc_input/5.68;
      OCR1A = UNIT_PULSE * MIN_PWM + (adc_input * UNIT_PULSE*(MAX_PWM - MIN_PWM))/180;
      Serial.println(UNIT_PULSE);
      Serial.println(OCR1A);  
      //_delay_ms(500);
      value=adc_read(adcpin);
      vehicle_Speed = speed_read(adcpin1);
      vehicle_Speed = vehicle_Speed*0.234375;

      Serial.println(value);
      Serial.println(vehicle_Speed);
      
      if(FLAG_BIT.BRAKE_ENG==0)
    {
     // CLR_BIT(PORTD,PD5);
    }
      if(value>200)
      {
        TCCR0B|=((1<<CS02)|(1<<CS00)); 
        OCR0B=100;
        PORTD|=(1<<PD7);
        Serial.println("a");
      }
      else 
      {
       TCCR0B=0x00;
        PORTD&=~(1<<PD7);
        Serial.println("b");
      }
      sei();
      
      if(vehicle_Speed > 20)
      {
        Steering_Angle = steer_read(adcpin2);
        Yaw_Angle = gyro_read(adcpin3);
       Serial.println(Steering_Angle);
       Serial.println(Yaw_Angle);
        Steering_Angle = Steering_Angle*0.351625;
        Yaw_Angle = Yaw_Angle*0.351625;
        difference_Angle = (Steering_Angle-Yaw_Angle);
         if((difference_Angle)>20)
         {
           PORTB|=(1<<OVER_STEER);
           PORTB&=~(1<<UNDER_STEER);
         }
         else if((difference_Angle)<-20)
         {
           PORTB|=(1<<UNDER_STEER);
           PORTB&=~(1<<OVER_STEER);
         }
        
      }
      else
      {
         PORTB&=~(1<<UNDER_STEER);
         PORTB&=~(1<<OVER_STEER);
      }

    }
    else{
        CLR_BIT(PORTD,PD4);
        CLR_BIT(PORTB,PB3);
        CLR_BIT(PORTB,PB2);
        CLR_BIT(PORTD,PD7);
        CLR_BIT(PORTD,PD5);
    }
  }
}

ISR(INT1_vect)
{ 
  cli();
FLAG_BIT.FLAG_ENG=!FLAG_BIT.FLAG_ENG;
  SET_BIT(PORTD,PD5);
sei();
}

void timer()
{
  OCR1A=15624;     // Timer is set for 1s
  Serial.println("timer on");
  TCCR1A=0;
  TCCR1B|=(1<<CS12)|(0<<CS11)|(1<<CS10); // prescalar value :1024
  TIMSK1|=(1<<OCIE1A)|(1<<TOIE1);   // Enable Output compareA interrupt 
  sei();
}

uint16_t speed_read(uint8_t adc)
{
  ADMUX = adc;
  ADMUX |= (1<<REFS0);
  ADCSRA |= (1<<ADEN);
  ADCSRA |= (1<<ADSC);
  ADCSRA |= ((1<<ADPS1)|(1<<ADPS2));
  while(ADCSRA&(1<<ADSC));

    return(ADC);

}

uint16_t adc_read(uint8_t adc)
{
  ADMUX = adc;
  ADMUX |= (1<<REFS0);
  ADCSRA |= (1<<ADEN);
  ADCSRA |= (1<<ADSC);
  ADCSRA |= ((1<<ADPS1)|(1<<ADPS2));
  while(ADCSRA&(1<<ADSC));

    return(ADC);
}

uint16_t steer_read(uint8_t adc)
{
  ADMUX = adc;
  ADMUX |= (1<<REFS0);
  ADCSRA |= (1<<ADEN);
  ADCSRA |= (1<<ADSC);
  ADCSRA |= ((1<<ADPS1)|(1<<ADPS2));
  while(ADCSRA&(1<<ADSC));

    return(ADC);

}

uint16_t gyro_read(uint8_t adc)
{ ADMUX = adc;
  ADMUX |= (1<<REFS0);
  ADCSRA |= (1<<ADEN);
  ADCSRA |= (1<<ADSC);
  ADCSRA |= ((1<<ADPS1)|(1<<ADPS2));
  while(ADCSRA&(1<<ADSC));

    return(ADC);

}
void init_pwm(){
                
                TCCR1B |= (1 << WGM12) ; //| (1 << WGM13)
    TCCR1B |= (1 << CS12);
    TCCR1A |= (1 << WGM11);
    TCCR1A |= (1 << COM1A1); //non-inverting
ICR1 = F_CPU /(PRESCALAR * PWM_FREQUENCY) -1;
}

void InitADC()
{
ADMUX=(1<<REFS0);             // For Aref=AVcc;
ADCSRA=(1<<ADEN)|(7<<ADPS0);
}

uint16_t ReadADC(uint8_t ch)
{
                //Select ADC Channel ch must be 0-7
                ADMUX&=0xf8;
                ch=ch&0b00000111;
                ADMUX|=ch;
                ADCSRA|=(1<<ADSC);
                while(!(ADCSRA & (1<<ADIF)));
                ADCSRA|=(1<<ADIF);
                return(ADC);
}

ISR(INT0_vect)
{
  
  cli();
  Serial.println("engine switch on");
  if(FLAG_BIT.FLAG1==0)
  {
                FLAG_BIT.FLAG1=1;
                TCNT0=0x00;
                TCCR0A=0x00;
                TCCR0B|=((1<<CS02)|(1<<CS00));  
                TIMSK0|=((1<<OCIE0A)|(1<<OCIE0B));
                sei();
  }
  else if(FLAG_BIT.FLAG1==1)
  {
    FLAG_BIT.FLAG1=0;
    TCCR0B=0x00;
  }
}
  
ISR(TIMER0_COMPA_vect)
{
  PORTD|=(1<<PD6);
}
  
ISR(TIMER0_COMPB_vect)
{
  PORTD&=~(1<<PD6);
}



