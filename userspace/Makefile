# allows selecting ARM or x86 platforms from Visual Studio
ifneq '$(VS_PLATFORM)' 'x86'
	include ../make-config/cross-compile.mk
	CFLAGS += -std=gnu11
else 
	CFLAGS += -std=gnu99 -DX86
endif

# allows selecting Debug/Release configurations from Visual Studio
ifeq '$(VS_CONFIG)' 'Debug'
	CFLAGS += -g
	LDFLAGS += -g
endif


TARGET := cameleon
CFLAGS += -static -Wall 
LDFLAGS += -pthread -lm -Wall 

C_SRCS := $(wildcard *.c)
C_OBJS := ${C_SRCS:.c=.o}

all: $(TARGET)

$(TARGET): $(C_OBJS)
	$(CC) $(LDFLAGS) $^ -o $@
		
clean:
	@- $(RM) $(TARGET)
	@- $(RM) $(C_OBJS)