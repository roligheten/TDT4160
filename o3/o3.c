#include "o3.h"
#include "gpio.h"
#include "systick.h"

/**************************************************************************//**
 * @brief Konverterer nummer til string 
 * Konverterer et nummer mellom 0 og 99 til string
 *****************************************************************************/
void int_to_string(char *timestamp, unsigned int offset, int i) {
    if (i > 99) {
        timestamp[offset]   = '9';
        timestamp[offset+1] = '9';
        return;
    }

    while (i > 0) {
	    if (i >= 10) {
		    i -= 10;
		    timestamp[offset]++;
		
	    } else {
		    timestamp[offset+1] = '0' + i;
		    i=0;
	    }
    }
}

/**************************************************************************//**
 * @brief Konverterer 3 tall til en timestamp-string
 * timestamp-argumentet må være et array med plass til (minst) 7 elementer.
 * Det kan deklareres i funksjonen som kaller som "char timestamp[7];"
 * Kallet blir dermed:
 * char timestamp[7];
 * time_to_string(timestamp, h, m, s);
 *****************************************************************************/
void time_to_string(char *timestamp, int h, int m, int s) {
    timestamp[0] = '0';
    timestamp[1] = '0';
    timestamp[2] = '0';
    timestamp[3] = '0';
    timestamp[4] = '0';
    timestamp[5] = '0';
    timestamp[6] = '\0';

    int_to_string(timestamp, 0, h);
    int_to_string(timestamp, 2, m);
    int_to_string(timestamp, 4, s);
}

typedef struct {
	volatile word CTRL;
	volatile word MODEL;
	volatile word MODEH;
	volatile word DOUT;
	volatile word DOUTSET;
	volatile word DOUTCLR;
	volatile word DOUTTGL;
	volatile word DIN ;
	volatile word PINLOCKN;
} gpio_port_map_t;

typedef struct {
	volatile gpio_port_map_t port[6];
	volatile word unused_space [10];
	volatile word EXTIPSELL;
	volatile word EXTIPSELH;
	volatile word EXTIRISE;
	volatile word EXTIFALL;
	volatile word IEN;
	volatile word IF;
	volatile word IFS;
	volatile word IFC;
	volatile word ROUTE;
	volatile word INSENSE;
	volatile word LOCK;
	volatile word CTRL;
	volatile word CMD;
	volatile word EM4WUEN;
	volatile word EM4WUPOL;
	volatile word EM4WUCAUSE;
} gpio_map_t;

typedef struct {
	volatile word CTRL;
	volatile word LOAD;
	volatile word VAL;
	volatile word CALIB;
} systick_t;

//Define constants
#define LED_PIN 2
#define LED_PORT GPIO_PORT_E
#define BUTTON_PORT GPIO_PORT_B
#define BUTTON_PIN 9

#define SECONDS_STATE 0
#define MINUTES_STATE 1
#define HOURS_STATE 2
#define COUNTDOWN_STATE 3
#define ALARM_STATE 4

//Globals
int seconds, minutes, hours;
int state;
volatile gpio_map_t* GPIO_map;

void set_LED(int on) {
    if(on == 1)
    	GPIO_map->port[LED_PORT].DOUTSET = 0b0100;
    else
    	GPIO_map->port[LED_PORT].DOUTCLR = 0b0100;

}

//PB1 press handler
void GPIO_EVEN_IRQHandler(void) {
    switch (state) {
    	case HOURS_STATE:
    		if(hours + minutes + seconds == 0) {
    			state = ALARM_STATE;
    			set_LED(1);
    		}
    		else
    			state++;
    		break;
    	case ALARM_STATE:
    		state = SECONDS_STATE;
    		set_LED(0);
    		break;
    	case COUNTDOWN_STATE:
    		break;
    	default:
    		state++;
    };

    GPIO_map->IFC = 1<<10;
}

//PB0 press handler
void GPIO_ODD_IRQHandler(void) {
    switch (state) {
		case SECONDS_STATE:
			seconds++;
			break;
		case MINUTES_STATE:
			minutes++;
			break;
		case HOURS_STATE:
			hours++;
			break;
    };

    GPIO_map->IFC = 1<<9;
}

void SysTick_Handler(void) {
    switch (state) {
		case COUNTDOWN_STATE:
			seconds--;
    		if(hours == 0 && minutes == 0 && seconds == 0) {
    			state = ALARM_STATE;
    			set_LED(1);
    		}
			if(seconds == -1) {
				minutes--;
				seconds = 59;
				if(minutes == -1) {
					hours--;
					minutes = 59;
				}
			}
    };
}

int main(void) {
    init();

    GPIO_map = (gpio_map_t*) GPIO_BASE;

    //Start system clock
    volatile systick_t* sys_tick;
    sys_tick = (systick_t*) SYSTICK_BASE;
    sys_tick->CTRL = 0b0111;
    sys_tick->LOAD = FREQUENCY;

    //Set LED to output
    GPIO_map->port[LED_PORT].DOUT = 0;
    GPIO_map->port[LED_PORT].MODEL = ((~(0b1111<<8))&GPIO_map->port[LED_PORT].MODEL)|(GPIO_MODE_OUTPUT<<8);

	//Button interrupt setup

    //Set input mode button pin
    GPIO_map->port[BUTTON_PORT].DOUT = 0;
    GPIO_map->port[BUTTON_PORT].MODEH = ((~(0b1111<<4))&GPIO_map->port[BUTTON_PORT].MODEH)|(GPIO_MODE_INPUT<<4);
	//Set select port B pin 9
    GPIO_map->EXTIPSELH = ((~(0b1111<<4))&GPIO_map->EXTIPSELH)|(0b0001<<4);
	//Set falling edge trigger pin 9
    GPIO_map->EXTIFALL = GPIO_map->EXTIFALL|(1<<9);
	//Set IF to 0 for safety
    GPIO_map->IFC = GPIO_map->IFC|(1<<9);
	//Set interrupt enable pin 9
    GPIO_map->IEN = GPIO_map->IEN|(1<<9);

    //Set input mode button pin 2
    GPIO_map->port[BUTTON_PORT].MODEH = ((~(0b1111<<8))&GPIO_map->port[BUTTON_PORT].MODEH)|(GPIO_MODE_INPUT<<8);
	//Set select port B pin 10
    GPIO_map->EXTIPSELH = ((~(0b1111<<8))&GPIO_map->EXTIPSELH)|(0b0001<<8);
	//Set falling edge trigger pin 10
    GPIO_map->EXTIFALL = GPIO_map->EXTIFALL|(1<<10);
	//Set IF to 0 for safety
    GPIO_map->IFC = GPIO_map->IFC|(1<<10);
	//Set interrupt enable pin 10
    GPIO_map->IEN = GPIO_map->IEN|(1<<10);

    //Initial state setup
    seconds = 0;
    minutes = 0;
    hours = 0;
    state = SECONDS_STATE;

    //Loop to keep alive
    while(1) {
    	char str[7];
    	time_to_string(str, hours, minutes, seconds);
    	lcd_write(str);
    }

    return 0;
}

