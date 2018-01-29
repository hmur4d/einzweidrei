# allows disabling crosscompilation for userspace depending on visual studio active platform
ifneq '$(VS_PLATFORM)' 'x86'
	include ../make-config/cross-compile.mk
	CFLAGS += -std=gnu11
else 
	CFLAGS += -std=gnu99
endif

# allows selecting debug/release from visual studio active configuration
ifeq '$(VS_CONFIG)' 'Debug'
	CFLAGS += -g
	LDFLAGS += -g
endif


TARGET := cameleon
CFLAGS += -static -Wall
LDFLAGS += -pthread -Wall

C_SRCS := $(wildcard *.c)
C_OBJS := ${C_SRCS:.c=.o}

all: $(TARGET)

$(TARGET): $(C_OBJS)
	$(CC) $(LDFLAGS) $^ -o $@
		
clean:
	@- $(RM) $(TARGET)
	@- $(RM) $(C_OBJS)