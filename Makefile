
CC=arm-linux-gnueabihf-gcc

target = local_test

target_src += local_cap_play.c
target_src += ringbuffer.c

target_obj := $(target_src:.c=.o)

CPPFLAGS = -I$(shell pwd)/../tinyalsa/build/include/
LDFLAGS += -L../tinyalsa/build/lib/
LDFLAGS += -ltinyalsa -lpthread -ldl

.PYTHON:all clean

all:$(target)


$(target):$(target_obj)
	@$(CC) $^ -o $@ $(LDFLAGS) $(CPPFLAGS)



clean:
	@$(RM) $(target_obj) $(target)