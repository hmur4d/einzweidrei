#include <stdlib.h>
#include "log.h"
#include "interrupt_handlers.h"

#define INTERRUPT_JUMPTABLE_SIZE 20

//use a jump table to map handlers to codes, faster than looping on a list
//the interrupt codes needs to be in a usable range though - the constant matches the index on the table
//so no big offsets are possible.
static interrupt_handler_f handlers[INTERRUPT_JUMPTABLE_SIZE];

static bool initialized = false;

static void reset_handlers() {
	log_debug("Resetting handlers jump table");
	for (int i = 0; i < INTERRUPT_JUMPTABLE_SIZE; i++) {
		handlers[i] = NULL;
	}
}

static void init_handlers() {
	reset_handlers();
	initialized = true;
}


bool _register_interrupt_handler(char code, char* name, interrupt_handler_f handler) {
	if (code < 0 || code >= INTERRUPT_JUMPTABLE_SIZE) {
		log_error("Interrupt code '%d' (%s) outside of range 0-%d", code, name, INTERRUPT_JUMPTABLE_SIZE);
		return false;
	}

	if (!initialized) {
		init_handlers();
	}

	log_debug("Registering interrupt handler for: 0x%x (%s)", code, name);
	handlers[(int)code] = handler;
	return true;
}

void call_interrupt_handler(char code) {
	if (code < 0 || code >= INTERRUPT_JUMPTABLE_SIZE) {
		log_error("Interrupt code '%d' outside of range 0-%d, ignoring", code, INTERRUPT_JUMPTABLE_SIZE);
		return;
	}

	if (!initialized) {
		init_handlers();
	}

	interrupt_handler_f handler = handlers[(int)code];
	if (handler != NULL) {
		log_debug("Calling interrupt handler for code 0x%x", code);
		handler(code);
	}
	else {
		log_warning("No handler found for interrupt code 0x%x, ignoring", code);
	}
}

void destroy_interrupt_handlers() {
	reset_handlers();
	initialized = false;
}
