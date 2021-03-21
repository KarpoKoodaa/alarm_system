FILENAME = main
PORT = /dev/tty.usbmodem142201
DEVICE = atmega2560
PROGRAMMER = wiring
BAUD = 115200
COMPILE = avr-gcc -Wall -Os -mmcu=$(DEVICE)



default: compile flash clean

compile:
	$(COMPILE) -c $(FILENAME).c -o $(FILENAME).o
	$(COMPILE) -o $(FILENAME).elf  $(FILENAME).o 
	avr-objcopy -j .text -j .data -O ihex $(FILENAME).elf $(FILENAME).hex
	avr-size --format=avr --mcu=$(DEVICE) $(FILENAME).elf

flash:
	avrdude -v -p $(DEVICE) -c $(PROGRAMMER) -P $(PORT) -b $(BAUD) -D -U flash:w:$(FILENAME).hex:i

clean:
	rm $(FILENAME).o
	rm $(FILENAME).elf
	rm $(FILENAME).he