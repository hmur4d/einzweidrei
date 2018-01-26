#include "commands.h"
#include "net_io.h"
#include "command_handlers.h"
#include "log.h"

void whoareyou(clientsocket_t* client, msgheader_t header, void* body) {
	log_info("received whoareyou");
}

//--

void register_all_commands() {
	log_info("Registering all commands");
	register_command_handler(CMD_WHO_ARE_YOU, "WHO_ARE_YOU", whoareyou);
}
