; Source File : reporter-8ach-1dig.asm
; Begun :  November 25, 2012
; Latest Revision : June 13, 2014
; Version : 0.1
;
; Spits data values out the serial port and allows the reception of
; pin commands through the serial port.
;
; * Copyright (C) 2012-2014 John B. Rosheck, Jr.
; *
; * This program is free software: you can redistribute it and/or modify
; * it under the terms of the GNU General Public License as published by
; * the Free Software Foundation, either version 3 of the License, or
; * (at your option) any later version.
; *
; * This program is distributed in the hope that it will be useful,
; * but WITHOUT ANY WARRANTY; without even the implied warranty of
; * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; * GNU General Public License for more details.
; *
; * You should have received a copy of the GNU General Public License
; * along with this program.  If not, see <http://www.gnu.org/licenses/>.
; *
;
; Spits out data values out the serial port and
; allows the reception of pin commands through the serial port.  The
; commands have the structure:
;  'L' - the letter L
;  'X' - hex code with the value to set the pins
;  'X' - repeated hex code for error checking
;  '\n' - end of message marker
;  The command code must be correct before the value is written out
;  the four bit ports.  A copy of the current state of the bit codes
;  is sent out the serial port with the other data.  The data output
;  has the form:
;  cntr - word in hex
;  data0 - first data word in hex
;  data1 - second data word in hex
;  data2 - third data word in hex
;  data3 - fourth data word in hex
;  outval - hex byte copy of the current bit settings
;
; pin assignments :
; bit 0 - output 0 - port pin RA0, pin #13
; bit 1 - output 1 - port pin RA1, pin #12
; bit 2 - output 2 - port pin RA2, pin #11
; bit 3 - output 3 - port pin RA4, pin #3
; ADC4 - analog input data 0 - port pin RC0, pin #10
; ADC5 - analog input data 1 - port pin RC1, pin #9
; ADC6 - analog input data 2 - port pin RC2, pin #8
; ADC7 - analog input data 3 - port pin RC3, pin #7
;
;
;
; define PIC device type
;
	processor	pic16f688

;
; include PIC register definitions, and some macros
;
	include "p16f688.inc"
;
;
; define config fuses
;
;	config	CP=off,WDT=off,PWRT=off,OSC=hs

;	config FOSC=INTOSCIO,WDTE=0,PWRTE=1,MCLRE=1,CP=1,CPD=1,BOREN=0,IESO=0,FCMEN=0
	org 0x2007
	data	0x00d4
;	config FOSC=EC,WDTE=0,PWRTE=1,MCLRE=1,CP=1,CPD=1,BOREN=0,IESO=0,FCMEN=0
;	org 0x2007
;	data	0x00c2
;
;
; define some register file variables
;

;	org	0x020 

delay_cnt1	equ	0x20
delay_cnt2	equ	0x21
str_num		equ	0x22
index		equ	0x23
temp		equ	0x24
cntr		equ	0x25
temp1		equ	0x26
temp2		equ	0x27
cnth		equ	0x28
cntl		equ	0x29
adc_ch		equ	0x30
adc_hi		equ	0x31
adc_lo		equ	0x32
adc_tmp		equ	0x33
adc_store	equ	0x34
ach0_lo		equ	0x34
ach0_hi		equ	0x35
ach1_lo		equ	0x36
ach1_hi		equ	0x37
ach2_lo		equ	0x38
ach2_hi		equ	0x39
ach3_lo		equ	0x3a
ach3_hi		equ	0x3b
ach4_lo		equ	0x3c
ach4_hi		equ	0x3d
ach5_lo		equ	0x3e
ach5_hi		equ	0x3f
ach6_lo		equ	0x40
ach6_hi		equ	0x41
ach7_lo		equ	0x42
ach7_hi		equ	0x43
ach_cnt		equ	0x44
cmd_cnt		equ	0x45
cmd_ch1		equ	0x46
cmd_ch2		equ	0x47
cmd_ch3		equ	0x48
srx_cnt		equ	0x49
srx_tmp		equ	0x50
prt_tmp		equ	0x51
ghv_tmp		equ	0x52
range_low	equ	0x53
range_high	equ	0x54
range_tmp	equ	0x55
ecode		equ	0x56
w_temp		equ	0x70
stat_temp	equ	0x71
;
;
; code start
;
	org	0
	goto	init_code
	org	0x004
	goto	int_code

	org	0x005

init_code
	bsf	STATUS,RP0
	bcf	STATUS,RP1
	; set osc for 8MHz operation
	movlw	0x70
	; set osc for external clock
	; movlw	0x08
	movwf	OSCCON
	; movlw	0x20	; set clock to fosc/32 about 1.8uS at 12MHz
	movlw	0x60	; set clock to fosc/64 
	movwf	ADCON1
	; movlw	0xf0
	; movwf	ANSEL	; set analog input on 4,5,6, and 7
	movlw	0xff
	movwf	ANSEL	; set analog input on all inputs
	movlw	0x2f
	movwf	TRISC
	bcf	STATUS,RP0
	bcf	STATUS,RP1
	movlw	0x00
	movwf	PORTA	; set all outputs to low
	bsf	STATUS,RP0
	bcf	STATUS,RP1
	;movlw	0x00
	;movwf	TRISA	; now set all pins to outputs
	movlw	0x1F
	movwf	TRISA	; set to all inputs except RA5
	bcf	STATUS,RP0
	bcf	STATUS,RP1
	clrf	PORTC
	movlw	00h
	movwf	CMCON0
	movlw	0x81
	movwf	ADCON0	; turn on the adc
	clrw
	call	delay
	clrw
	call	delay
	;banksel	ANSEL
	; all port C pins are digital outputs
	bcf	STATUS,RP0
	bcf	STATUS,RP1
	call	setup_serial
	clrw
	call	delay
	; send "HI"
;	movlw	0x48	; 'H'
;	call	send_char
;	movlw	0x49	; 'I'
;	call	send_char
;	movlw	0x0d	; '/r'
;	call	send_char
;	movlw	0x0a	; '/a'
;	call	send_char
	clrw
	call	delay
;	movlw	0
;	movwf	str_num
;	call	send_string
	clrw
	movwf	cntr
	clrw
	movwf	cnth
	movwf	cntl
	movwf	srx_cnt
	movwf	ecode
	movwf	cmd_cnt
	movwf	cmd_ch1
	movwf	cmd_ch2
	movwf	cmd_ch3
	movlw	0x00
	movwf	adc_ch		; start at channel zero
	call	clr_ach_acc	; clear accum's and cycle counter
main_loop
	; get a reading
	movf	adc_ch,W
	call	start_adc
	call	get_adc_value
	call	add_ch_val	; put it in the channel's accumulator
	; advance to the next channel if possible
	incf	adc_ch,F
	movf	adc_ch,W
	andlw	0x7
	btfss	STATUS,Z
	goto	main_loop	; not done with all channels yyet
	movlw	0x00
	movwf	adc_ch		; restart at channel zero
;	goto	dump_data
	; done with one series of channel reads, advance cycles count
	incf	ach_cnt,F
	movf	ach_cnt,W
	andlw	0x1f		; see if its got to 32
	btfss	STATUS,Z
	goto	main_loop
	; done entire set of 32 readings over all eight channels
	call	div_ch_vals	; shift all readings down (/32) for average
	; now send it all out the serial port
	; first send counter
dump_data
	movf	cnth,W
	call	send_hex_byte
	movf	cntl,W
	call	send_hex_byte
	; now send all eight channel values
	movlw	0x8
	movwf	adc_tmp
	bcf	STATUS,IRP
	movlw	adc_store
	movwf	FSR	; set pointer to base of storage (lo of channel 0)
ch_send_loop
	movlw	0x20
	call	send_char
	movf	0,W
	movwf	adc_lo	; store it
	incf	FSR,F
	movf	0,W
	call	send_hex_byte
	movf	adc_lo,W
	call	send_hex_byte
	incf	FSR,F
	decfsz	adc_tmp,F
	goto	ch_send_loop
	; done with analog, now send system status
	movlw	0x20
	call	send_char
	movf	PORTA,W
	call	send_hex_byte	; copy PORTA to stream
	movlw	0x20
	call	send_char
	movf	srx_cnt,W
	call	send_hex_byte	; copy RX bytes received to stream
	movlw	0x20
	call	send_char
	movf	ecode,W
	call	send_hex_byte
	movlw	0x20
	call	send_char
	movf	cmd_cnt,W
	call	send_hex_byte
	movlw	0x20
	call	send_char
	movf	cmd_ch1,W
	call	send_hex_byte
	movlw	0x20
	call	send_char
	movf	cmd_ch2,W
	call	send_hex_byte
	movlw	0x20
	call	send_char
	movf	cmd_ch3,W
	call	send_hex_byte
	; all done, wrap up and prepare for next set
	movlw	0x0d
	call	send_char
	movlw	0x0a
	call	send_char
	call	inc_counter
	call	clr_ach_acc	; clear accum's and cycle counter
	; wait if needed
;	clrw
;	call	delay
	; change indicator light state
;	btfsc	PORTA, 5
;	goto	set_ra5_high
;	movlw	0x20
;	iorwf	PORTA,F
	goto	main_loop
;set_ra5_high
;	movlw	0x1f
;	andwf	PORTA,F
;	goto	main_loop
;
;
start_adc
	movwf	adc_tmp
	rlf	adc_tmp,F
	rlf	adc_tmp,W
	andlw	0x1c
;	iorlw	0x81
	iorlw	0xc1		; use external Vref
	movwf	ADCON0		; set the channel to read
	; wait a certain time to make sure the reading will be correct
	movlw	10
	call	short_delay	; about 10uS
	; start the conversion
	bsf	ADCON0,GO
	return
;
get_adc_value
	btfsc	ADCON0,GO
	goto	get_adc_value
	bcf	STATUS,IRP
	movlw	ADRESL
	movwf	FSR
	movf	0,W
	movwf	adc_lo
	movf	ADRESH,w
	movwf	adc_hi
	return
;
; shift all channel accumulators by 5 bits 
div_ch_vals
	movlw	adc_store
	movwf	FSR	; point to base of data
	movlw	0x8	; number of channels to do
	movwf	adc_tmp
dcv_ch_loop
	movlw	0x5
	movwf	temp	; number times to shift
dcv_bit_loop
	bcf	STATUS,C
	incf	FSR,F
	rrf	0,F
	decf	FSR,F
	rrf	0,F
	decfsz	temp,F
	goto	dcv_bit_loop
	incf	FSR,F	; last ch hi val
	incf	FSR,F	; next ch lo val
	decfsz	adc_tmp,F
	goto	dcv_ch_loop
	return
;
; add a new value to the channel accumulator
add_ch_val
	bcf	STATUS,C
	rlf	adc_ch,W	; *2 for 2 bytes per accum per ch
	addlw	adc_store	; offset by base of data space
	bcf	STATUS,IRP	; make sure its pointing to the right block
	movwf	FSR	; point to low value of accumulator
	movf	0,W	; get low value of channel accumulator
	addwf	adc_lo,W
	movwf	0	; store new low value
	incf	FSR,F	; pointer to high byte
	btfsc	STATUS,C	; no carry, no work to do
	incf	adc_hi,F	; advance value if there was a carry
	movf	0,W
	addwf	adc_hi,W
	movwf	0	; store new high value
	return
;
; clear the channel accumulator registers
clr_ach_acc
	bcf	STATUS,IRP
	movlw	0x10
	movwf	adc_tmp	; set count value
	movlw	adc_store
	movwf	FSR
caa_loop
	clrf	0
	incf	FSR,F
	decfsz	adc_tmp,F
	goto	caa_loop
	movlw	ach_cnt
	movwf	FSR
	clrf	0	; take care of ach_cnt
	return
	

;
; passed byte in W and output hex code
send_hex_byte
	movwf	temp1
	movwf	temp2
	rrf	temp2,F
	rrf	temp2,F
	rrf	temp2,F
	rrf	temp2,W
	andlw	0x0f
	movwf	temp2
	movlw	0x0a
	subwf	temp2,W
	btfss	STATUS,C
	goto	shb_num1
	; char is an alpha, W contains val - 9 or 1 to 6
	movf	temp2,W
	addlw	0x37
	call	send_char
	goto	shb_low_nib
shb_num1
	movf	temp2,W
	addlw	0x30
	call	send_char
shb_low_nib
	movf	temp1,W
	andlw	0x0f
	movwf	temp2
	movlw	0x0a
	subwf	temp2,W
	btfss	STATUS,C
	goto	shb_num2
	movf	temp2,W
	addlw	0x37
	call	send_char
	return
shb_num2
	movf	temp2,W
	addlw	0x30
	call	send_char
	return
;
inc_counter
	incfsz	cntl,F
	return
	incf	cnth,F
	return
;
; with a 12.288MHz the instruction time is 0.326uS
; loop is about 1uS long - so about 1uS per count passed in W
short_delay
	movwf	delay_cnt1
sd_loop
	nop
	decfsz	delay_cnt1,F
	goto	sd_loop
	return
	
;
; delay subroutine
; input: delay count in W
;
; inner loop duration approx:
; 5*256+3 = 1283 cycles ->
; 1.28ms with 4MHz crystal (1MHz instruction time)
; 0.64ms with 8MHz internal oscillator (2MHz instruction time)
;
delay	movwf	delay_cnt1
	clrf	delay_cnt2
delay_a	nop
	nop
	incfsz	delay_cnt2,F
	goto	delay_a
	decfsz	delay_cnt1,F
	goto	delay_a
	return

setup_serial
	; setup the baud rate generator
	movlw	0x00
	movwf	BAUDCTL
	movlw	0x0c	; set baud to 9615 with 8MHz clock
;	movlw	0x02	; set baud to 9600 with 1.8432MHz clock
;	movlw	0x13	; set baud to 9600 with 12.288 MHz crystal
	movwf	SPBRG
	; setup the uart tx
	; enable TX, 9 bit data, 9th bit is 1 or mark (second stop bit)
	movlw	0x61
	; just TX enable
;	movlw	0x22
	movwf	TXSTA
	; setup the uart rx
	; enable serial port, 8 bit RX
	movlw	0x90
	movwf	RCSTA
	; setup uart rx to cause an interrupt
	; enable the RCIE bit of the PIE1 register
	bsf	STATUS,RP0
	bcf	STATUS,RP1
	bsf	PIE1,RCIE
	bcf	STATUS,RP0
	bcf	STATUS,RP1
	; set the PEIE bit of INTCON
	bsf	INTCON,PEIE
	; set the GIE bit of INTCON
	bsf	INTCON,GIE
	return

send_char	; char passed in W
	;	btfss	TXSTA, TRMT
	btfss	PIR1, TXIF
	goto	send_char
	movwf	TXREG
	return

get_char	; return char in W
	btfss	PIR1, RCIF	; test for a rx char
	goto	get_char
	movf	RCREG,W		; this resets the interrupt flag
	return
get_ichar	; chase new char and put it somewhere (already in bank 0)
	btfsc	RCSTA,FERR	; test for a framing error on this data
	goto	gic_fail	; reset the uart receiver
	btfsc	RCSTA,OERR	; test for an overrun error
	goto	gic_fail
	; no errors, deal with the data
	movf	RCREG,W		; this resets the interrupt flag
	movwf	srx_tmp		; save the character
	incf	srx_cnt,F	; advance the received character counter
	btfsc	PIR1,RCIF
	bcf	PIR1,RCIF	; make sure its reset
	sublw	0x0d		; ignore a CR
	btfsc	STATUS,Z
	goto	gic_finish	; all done
	; store using command count
	movf	cmd_cnt,W
	btfss	STATUS,Z
	goto	scmdt_1
	; store first command character and exit
	movf	srx_tmp,W
	movwf	cmd_ch1
	sublw	0x0a	; see if its the end of a command (error)
	btfsc	STATUS,Z
	goto	scmd_reset	; restart command parse if LF
	incf	cmd_cnt,F
	goto	gic_finish
scmdt_1	; check for command char 2
	movf	cmd_cnt,W
	sublw	0x1
	btfss	STATUS,Z
	goto	scmdt_2
	movf	srx_tmp,W
	movwf	cmd_ch2
	sublw	0x0a	; see if its the end of a command (error)
	btfsc	STATUS,Z
	goto	scmd_reset	; restart command parse if LF
	incf	cmd_cnt,F
	goto	gic_finish
scmdt_2 ; check for command char 3
	movf	cmd_cnt,W
	sublw	0x2
	btfss	STATUS,Z
	goto	scmdt_end
	movf	srx_tmp,W
	movwf	cmd_ch3
	sublw	0x0a	; see if its the end of a command (error)
	btfsc	STATUS,Z
	goto	scmd_reset	; restart command parse if LF
	incf	cmd_cnt,F
	goto	gic_finish
scmdt_end	; must be the terminating char, then do command
	movf	srx_tmp,W
	sublw	0x0a	; test for a LF
	btfss	STATUS,Z	; must be LF or error
	goto	scmd_reset	; restart command
	; should have a valid command, test the parts first
	movf	cmd_ch1,W
	sublw	'L'		; first char must be an 'L' command
	btfss	STATUS,Z
	goto	scmd_reset
	movf	cmd_ch2,W
	subwf	cmd_ch3,W
	btfss	STATUS,Z	; both chars must be the same
	goto	scmd_reset
	; data chars match, now decode to hex if possible
	movf	cmd_ch2,W
	call	get_hex_value	; returns value in W, carry set if error
	btfss	STATUS,C
	goto	make_real
	movlw	0x80
	iorwf	ecode,F
	goto	scmd_reset
make_real
	; make value in W real
	movwf	srx_tmp	; store it
	movlw	0x40
	iorwf	ecode,F
	clrf	prt_tmp
	btfsc	srx_tmp,0
	bsf	prt_tmp,5
	;btfsc	srx_tmp,1
	;bsf	prt_tmp,1
	;btfsc	srx_tmp,2
	;bsf	prt_tmp,2
	;btfsc	srx_tmp,3
	;bsf	prt_tmp,4
	movf	PORTA,W		; get the current state of PORTA
	andlw	0x1f		; leave some bits alone
	iorwf	prt_tmp,W	; add to the given command
	movwf	PORTA	; write it to the output port and reset
scmd_reset
	clrf	cmd_cnt	; reset the command count, ready for a new one
	goto	gic_finish
gic_fail	; go here when a bad char is received
	movf	RCREG,W		; read the data to clear the interrupt
	bcf	RCSTA,SPEN	; reset the receiver
	bsf	RCSTA,SPEN	; restart it
	movlw	0x20
	iorwf	ecode,F
	; when done, jump to end of isr
gic_finish
	goto	end_int

int_code	; run an interrupt.  Only serial rx for now.
	; save state
	movwf	w_temp
	swapf	STATUS,W
	movwf	stat_temp
	; now for the isr, find the right one
	bcf	STATUS,RP0
	bcf	STATUS,RP1	; point to bank 0
	btfsc	PIR1,RCIF	; test for RX interrupt
	goto	get_ichar
	; nothing here - force all zeros to PIR1
	clrf	PIR1
	; and drop through
end_int		; after doing a specific isr, end out here
	swapf	stat_temp,W
	movwf	STATUS
	swapf	w_temp,F
	swapf	w_temp,W
	retfie		; return from the interrupt

get_hex_value	; passed char in W, return binary in W with C=0 if ok
	movwf	ghv_tmp	; store it
	; test char for '0' to '9' range
	movlw	'0'
	movwf	range_low
	movlw	'9'
	movwf	range_high
	movf	ghv_tmp,W
	call	test_range
	btfss	STATUS,C
	goto	ghv_range2
	; value is 0 to 9, compute and return
	movlw	'0'
	subwf	ghv_tmp,W
	bcf	STATUS,C
	return
ghv_range2
	; test char for 'A' to 'F' range
	movlw	'A'
	movwf	range_low
	movlw	'F'
	movwf	range_high
	movf	ghv_tmp,W
	call	test_range
	btfss	STATUS,C
	goto	ghv_range3
	; value is A to F, compute and return
	movlw	'A'
	subwf	ghv_tmp,W
	addlw	0x0a
	bcf	STATUS,C
	return
ghv_range3
	; test char for 'a' to 'F' range
	movlw	'a'
	movwf	range_low
	movlw	'f'
	movwf	range_high
	movf	ghv_tmp,W
	call	test_range
	btfss	STATUS,C
	goto	ghv_bad
	; value is a to f, compute and return
	movlw	'a'
	subwf	ghv_tmp,W
	addlw	0x0a
	bcf	STATUS,C
	return
ghv_bad
	bsf	STATUS,C
	return

test_range	; passed value in W, range in high/low registers
		; return with C=1 if in given inclusive range
	movwf	range_tmp
	subwf	range_high,W
	btfss	STATUS,C
	goto	range_fail
	movf	range_tmp,W
	subwf	range_low,W	
	btfss	STATUS,C
	goto	range_pass
	btfss	STATUS,Z
	goto	range_fail
range_pass
	bsf	STATUS,C
	return
range_fail
	bcf	STATUS,C
	return


	org	0x300

send_string
	movlw	0x3	; set to string page
	movwf	PCLATH
	movf	str_num,W
	call	get_string_address
	movwf	index	; returns the offset to the string start

sstring1
	movf	index,W
	call	get_str_char
	addlw	0
	btfsc	STATUS,Z
	goto	sstring2
	movwf	temp
	clrw
	movwf	PCLATH		; need to reference first page for call
	movf	temp,W
	call	send_char
	movlw	0x3
	movwf	PCLATH		; need to reference third page for this
	incf	index,F
	goto	sstring1
sstring2
	return

get_string_address
	addwf	PCL,F
	retlw	msg0-msg0
	retlw	msg1-msg0
	retlw	msg2-msg0

get_str_char
	addwf	PCL,F	
msg0	dt "hello, world", 0x0d, 0x0a, 0x00
msg1	dt "ver 1.0", 0x0d, 0x0a, 0x00
msg2	dt "copyright (c) John Rosheck, 2012", 0x0d, 0x0a, 0x00
	end
