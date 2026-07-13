NAME=$(shell basename $(CURDIR))

CC=arm-none-eabi-gcc

CFLAGS=-nostdinc -nostdlib -mcpu=cortex-r52 -o0 -g -marm -I.

all:
	$(CC) $(CFLAGS) -c setup.s -o setup.o
	$(CC) $(CFLAGS) -c gicv3.c -o gicv3.o
	$(CC) $(CFLAGS) -c main.c -o main.o
	$(CC) $(CFLAGS) setup.o gicv3.o main.o -o $(NAME).elf -Wl,-Tlinker.ld
	arm-none-eabi-objcopy -Obinary $(NAME).elf $(NAME).bin

clean:
	rm -f *.o *.elf *.bin
