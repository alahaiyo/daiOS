CC=arm-hisiv300-linux-gcc
LD=arm-hisiv300-linux-ld
OBJCOPY=arm-hisiv300-linux-objcopy

CFLAGS= -O2 -g
ASFLAGS= -O2 -g
LDFLAGS=-Tdaios.lds -Ttext 0x00000000
#LDFLAGS += -static -nostartfiles -nostdlib

TARGET = daios
BIN = daios.bin


SRC += ${wildcard  *.c} 
SRC += ${wildcard  shell/*.c} 

OBJS += start.o
OBJS += ${patsubst %.c, %.o, ${SRC}}

all:$(TARGET)

$(OBJ):%.o:%.c
	$(CC) $(CFLAGS) -c $<

start.o:start.S
	$(CC) $(ASFLAGS) -c $<

$(TARGET):$(OBJS)
	$(CC) -static -nostartfiles -nostdlib -v $(LDFLAGS) $? -o $@ -lgcc
	$(OBJCOPY) -O binary $@ $(BIN)

clean:
	rm *.o daios daios.bin -f
  
