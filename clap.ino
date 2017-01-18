#include <avr.io.h>
#include <avr/interrupt.h>

#define sound_sensor_pin 7

int claps = 0;
int counter = 0;
int sensorcheck();

void setup(){
	pinMode(sound_sensor_pin, INPUT);

	//initialize timer
	cli();
	TCCR1A = 0;
	TCCR1B = 0;

	//enable Timer1 overflow flag
	TIMSK1 = (1<<TOIE1);
	TCCR1B |= (1<<CS10);
	sei();
}

void loop(){
	while(claps == 0){
		if(sensorcheck()){
			//delay for 300ms before looking for 2nd clap
			delay(300);
			
			//120 = checking sensor for 0.5s
			while(counter < 120){
				if(sensorcheck()){
					
					//check for 3rd clap to prevent false triggering
					delay(300);

					counter = 0;
					while(counter < 120){
						sensorcheck();
					}
				}
			}
		}
	}


	if(claps == 2){
		//switch realay


		//reset claps and counter
		claps = 0;
		counter = 0;
	}
}

int sensorcheck(){
	int state = digitalRead(sound_sensor_pin);

	if(state){
		claps += 1;
		return 1;
	}
	else{
		return 0;
	}
}


//define timer cycle
//increment if first clap triggered
//triggers every system clock cycle (16MHz) = 0.0041s (243.9 times/s)
ISR(TIMER1_OVF_vect){
	if(claps>0){
		counter += 1
	}
}
