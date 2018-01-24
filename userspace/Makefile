include ../make-config/cross-compile.mk

TARGET := cameleon

CFLAGS += -static -g -Wall -std=gnu11
LDFLAGS += -pthread -g -Wall

C_SRCS := $(wildcard *.c)
C_OBJS := ${C_SRCS:.c=.o}

all: $(TARGET)

$(TARGET): $(C_OBJS)
	$(CC) $(LDFLAGS) $^ -o $@
		
clean:
	@- $(RM) $(TARGET)
	@- $(RM) $(C_OBJS)