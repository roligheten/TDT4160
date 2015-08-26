.thumb
.syntax unified

.include "gpio_constants.s"     // Register-adresser og konstanter for GPIO

.text
	.global Start

Start:
	//Sett R3 til button DIN address
	LDR R0, =BUTTON_PORT
	LDR R1, =PORT_SIZE
	MUL R0, R0, R1
	LDR R1, =GPIO_BASE
	ADD R0, R0, R1
	LDR R1, =GPIO_PORT_DIN
	ADD R3, R0, R1

	//Sett R4 til LED DOUTSET og R7 til LED DOUTCLR address
	LDR R0, =LED_PORT
	LDR R1, =PORT_SIZE
	MUL R0, R0, R1
	LDR R1, =GPIO_BASE
	ADD R0, R0, R1
	CMP R3, #0
	BEQ SET_ON
	LDR R1, =GPIO_PORT_DOUTCLR

	SET_ON:
	LDR R1, =GPIO_PORT_DOUTSET
	ADD R4, R0, R1
	LDR R1, =GPIO_PORT_DOUTCLR
	ADD R7, R0, R1

	//Sett R5 til data for å fjerne uønskede bits fra knapp input
	LDR R0, =#1
	LSL R5, R0, #BUTTON_PIN

	//Sett R6 til output for å sette LED på og R7 til 0
	LDR R0, =#1
	LSL R6, R0, #LED_PIN

	B set_off_start //Start i off state

set_on_start:
	STR R6, [R4] //Mat inn data til LED pin
set_on:
	LDR R0, [R3] //Last inn knapp status
	AND R0, R0, R5 //Fjern alle bits hvis knappen ikke er trykket på
	CMP R0, #0
	BNE set_off_start //Gå til off state
	B set_on //Gjør ny sjekk

set_off_start:
	STR R6, [R7] //Mat inn data til LED pin
set_off:
	LDR R0, [R3] //Last inn knapp status
	AND R0, R0, R5 //Fjern alle bits hvis knappen ikke er trykket på
	CMP R0, #0
	BEQ set_on_start //Gå til on state
	B set_off //Gjør ny sjekk


NOP // Behold denne pÃ¥ bunnen av fila

