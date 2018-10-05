#include "ram.h"
#include "memory_map.h"
#include "log.h"


bool ram_find(unsigned int ram_id, int span_bytes, ram_descriptor_t* ram) {
	ram->id = ram_id;
	ram->is_register = false;
	ram->register_id = -1;
	ram->span_bytes = span_bytes;
	ram->span_int32 = span_bytes / sizeof(int32_t);

	//rams pour composant Lock
	if (ram_id >= RAM_REGISTERS_LOCK_SELECTED && ram_id<RAM_REGISTERS_LOCK_SELECTED + RAM_REGISTERS_LOCK_NUMBER_OF_REGISTERS) {
		ram->is_register = true;
		ram->register_id = ram_id - RAM_REGISTERS_LOCK_SELECTED;
		ram->offset_bytes = RAM_REGISTERS_OFFSET + ram->register_id * RAM_REGISTERS_OFFSET_STEP;
		ram->offset_int32 = ram->offset_bytes / sizeof(int32_t);
		return true;
	}

	// registers: address is offsetted by RAM_REGISTERS_SELECTED
	if (ram_id >= RAM_REGISTERS_SELECTED && ram_id < RAM_REGISTERS_SELECTED + RAM_REGISTERS_NUMBER_OF_REGISTERS) {
		ram->is_register = true;
		ram->register_id = ram_id - RAM_REGISTERS_SELECTED;
		ram->offset_bytes = RAM_REGISTERS_OFFSET + ram->register_id * RAM_REGISTERS_OFFSET_STEP;
		ram->offset_int32 = ram->offset_bytes / sizeof(int32_t);
		return true;
	}


	//rams: address is the ram id, we need to translate it
	//normal case, ram before registers
	if (ram_id < RAM_INTERFACE_NUMBER_OF_RAMS) {
		ram->offset_bytes = ram_id * RAM_OFFSET_STEP;
		ram->offset_int32 = ram->offset_bytes / sizeof(int32_t);
		return true;
	}

	//special cases, ram is after registers
	if (ram_id == RAM_B0_WAVEFORM_SELECTED) {
		ram->offset_bytes = RAM_B0_WAVEFORM_OFFSET;
		ram->offset_int32 = ram->offset_bytes / sizeof(int32_t);
		return true;
	}
	if (ram_id == RAM_SMART_TTL_ADR_ATT_SELECTED) {
		ram->offset_bytes = RAM_SMART_TTL_ADR_ATT_OFFSET;
		ram->offset_int32 = ram->offset_bytes / sizeof(int32_t);
		return true;
	}
	if (ram_id == RAM_SMART_TTL_VALUES_DATA_SELECTED) {
		ram->offset_bytes = RAM_SMART_TTL_VALUES_DATA_OFFSET;
		ram->offset_int32 = ram->offset_bytes / sizeof(int32_t);
		return true;
	}

	log_error("Unknown memory base: 0x%x", ram_id);
	return false;
}