#ifndef _INTERRUPT_HANDLERS_H_
#define _INTERRUPT_HANDLERS_H_

/*
Association between interrupt codes and callback functions
*/

#include "std_includes.h"

//Callback used to to handle an interrupt
typedef void(*interrupt_handler_f) (int8_t code);



//Registers a new command handler.
//This macro gets the interrupt name automatically from the constant name.
#define register_interrupt_handler(code, handler) _register_interrupt_handler(code, #code, handler);

//Registers a new interrupt handler. 
//Don't use directly, define a INTERRUPT_xxx constant and use the macro.
bool _register_interrupt_handler(int8_t code, const char* name, interrupt_handler_f handler);

//Finds then calls the handler associated to the interrupt code.
//Doesn't return anything, so it can be used directly as a callback to interrupt_reader_start(..)
void call_interrupt_handler(int8_t code);

//Destroys the interrupt code / callback map.
//Should only be called on program termination.
void destroy_interrupt_handlers();

#endif

