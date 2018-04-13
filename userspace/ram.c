#include "ram.h"
#include "memory_map.h"
#include "log.h"


bool ram_find(unsigned int ram_id, int span, ram_descriptor_t* ram) {
	ram->span = span;

	//TODO rams pour composant Lock

	// registers: address is offsetted by RAM_REGISTERS_SELECTED
	if (ram_id >= RAM_REGISTERS_SELECTED && ram_id < RAM_REGISTERS_SELECTED + RAM_REGISTERS_LOCK_SELECTED) {
		ram->address = RAM_REGISTERS_OFFSET + (ram_id - RAM_REGISTERS_SELECTED) * RAM_REGISTERS_OFFSET_STEP;
		return true;
	}

	//rams: address is the ram id, we need to translate it
	//normal case, ram before registers
	if (ram_id < RAM_INTERFACE_NUMBER_OF_RAMS) {
		ram->address = ram_id * RAM_OFFSET_STEP;
		return true;
	}

	//special cases, ram is after registers
	if (ram_id == RAM_B0_WAVEFORM_SELECTED) {
		ram->address = RAM_B0_WAVEFORM_OFFSET;
		return true;
	}
	if (ram_id == RAM_SMART_TTL_ADR_ATT_SELECTED) {
		ram->address = RAM_SMART_TTL_ADR_ATT_OFFSET;
		return true;
	}
	if (ram_id == RAM_SMART_TTL_VALUES_DATA_SELECTED) {
		ram->address = RAM_SMART_TTL_VALUES_DATA_SELECTED;
		return true;
	}

	log_error("Unknown memory base: 0x%x", ram_id);
	return false;
}