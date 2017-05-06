CC=arm-hisiv300-linux-gcc
LD=arm-hisiv300-linux-ld
OBJCOPY=arm-hisiv300-linux-objcopy

CFLAGS= -O2 -g
ASFLAGS= -O2 -g
LDFLAGS=-Tdaios.lds -Ttext 0x00000000

OBJS=start.o boot.o uart.o print.o irq_handle.o timer.o irq.o nand.o mem.o chip.o libc.o storage.o daifs.o fs.o

.c.o:
	$(CC) $(CFLAGS) -c $<
.s.o:
	$(CC) $(ASFLAGS) -c $<

daios:$(OBJS)
	$(CC) -static -nostartfiles -nostdlib -v $(LDFLAGS) $? -o $@ -lgcc
	$(OBJCOPY) -O binary $@ daios.bin

clean:
	rm *.o daios daios.bin -f
  
