// PA Board - Benchtop spectrometer

#include <termios.h>
#include <time.h>

#include "log.h"
#include "hw_pa.h"

// The file descriptor for the PA board UART
int pa_uart_fd = -1;
// Whether verbose messages should be logged or not
bool pa_verbose = false;

//////////////////
// Utility methods

/**
 * Allocate a string. This ensures the first character is 0, so
 * you can safely use strlen(), strcat, etc. on it.
 * @return A newly allocated string
 */
char* malloc_string(size_t size) {
	char* new_string = malloc(size);
	new_string[0] = 0;
	return new_string;
}

/**
 * Join two strings together into a newly allocated string.
 * The arguments are freed, so do not use them after.
 * @param first The first string
 * @param second The second string
 * @return A newly allocated string
 */
char* join_strings(char* first, char* second) {
	size_t first_size = strlen(first);
	size_t second_size = strlen(second);
	size_t new_size = first_size + second_size + 1;
	char* joined = malloc_string(new_size);
	strncat(joined, first, first_size);
	strncat(joined, second, second_size);
	free(first);
	free(second);
	return joined;
}

/**
 * Get the monotonic time, in milliseconds
 */
long long monotonic_ms()
{
	struct timespec ts_now = {0};
	clock_gettime(CLOCK_MONOTONIC, &ts_now);
	long long now = ts_now.tv_sec * 1000 + ts_now.tv_nsec / 1000000;
	return now;
}

///////////////
// UART methods

/**
 * Delete the contents of the input and output buffers
 * @return true if the flush succeeded.
 */
bool pa_uart_flush() {
	int result = tcflush(pa_uart_fd, TCIOFLUSH);
	if (result != 0) {
		log_error_errno("Could not flush buffers");
	}
	return result == 0;
}

/**
 * Put the serial port in blocking mode (for writing)
 */
bool pa_uart_set_blocking() {
	int flags = fcntl(pa_uart_fd, F_GETFL, 0);
	if (flags == -1) {
		log_error_errno("fcntl(F_GETFL) failed");
		return false;
	}
	flags &= ~O_NONBLOCK;
	if (fcntl(pa_uart_fd, F_SETFL, flags) == -1) {
		log_error_errno("fcntl() failed");
		return false;
	}
	return true;
}

/**
 * Put the serial port in nonblocking mode (for reading)
 */
bool pa_uart_set_nonblocking() {
	int flags = fcntl(pa_uart_fd, F_GETFL, 0);
	if (flags == -1) {
		log_error_errno("fcntl(F_GETFL) failed");
		return false;
	}
	flags |= O_NONBLOCK;
	if (fcntl(pa_uart_fd, F_SETFL, flags) == -1) {
		log_error_errno("fcntl() failed");
		return false;
	}
	return true;
}

/**
 * Open the UART for the PA board
 * @param uart_dev The filename of the UART device, e.g. /dev/ttyS1
 * @return The file descriptor, or -1 on error.
 */
int pa_uart_open(char* uart_dev, unsigned int baudrate) {
	log_info("PA: Opening %s", uart_dev);

	// Open the port
	pa_uart_fd = open(uart_dev, O_RDWR | O_NOCTTY | O_NONBLOCK);
	if (pa_uart_fd < 0) {
		log_error_errno("PA: Could not open port %s", uart_dev);
	}
	// Restore blocking
	else if (fcntl(pa_uart_fd, F_SETFL, 0) < 0) {
		log_error_errno("PA: fcntl(F_SETFL) failed");
	}
	else {
		// Save current port settings
		struct termios oldtty;
		tcgetattr(pa_uart_fd, &oldtty);
		// Set serial port speed
		cfsetispeed(&oldtty, baudrate);
		cfsetospeed(&oldtty, baudrate);
		// Set 8N1
		oldtty.c_cflag &= ~PARENB;
		oldtty.c_cflag &= ~CSTOPB;
		oldtty.c_cflag &= ~CSIZE;
		oldtty.c_cflag |= CS8 | CLOCAL | CREAD;
		// Do not use RTSCTS
		oldtty.c_cflag &= ~CRTSCTS;
		// Do not map \r to \n
		oldtty.c_iflag &= ~ICRNL;
		// Ignore break conditions
		oldtty.c_iflag |= IGNBRK;
		// Do not use software flow control
		oldtty.c_iflag &= ~(IXON | IXOFF | IXANY);
		// Use raw input
		oldtty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
		// Use raw output
		oldtty.c_oflag &= ~(OPOST);
		// Delete the contents of the input and output buffers
		pa_uart_flush();
		// Apply new settings
		if (tcsetattr(pa_uart_fd, TCSANOW, &oldtty) != 0) {
			log_error_errno("Could not set serial port permissions for %s", uart_dev);
			close(pa_uart_fd);
			pa_uart_fd = -1;
		}
	}
	return pa_uart_fd;
}

/**
 * Find a the end of a PA board microcontroller response.
 * @param response The response text to search
 * @param start_position The idnex ohe first character to start searching from
 * @return the index of the first response character, or -1 if it was not found
 */
int find_response_end(const char* response, unsigned int start_position) {
	int response_index = -1;
	int response_length = strlen(response);
	// The prompt, indicating the end of a response
	size_t prompt_length = strlen(PA_PROMPT_RESPONSE);
	// Only check if there is enough data to search in the response
	if (start_position + prompt_length <= response_length) {
		char* prompt_location = strstr(response, PA_PROMPT_RESPONSE);
		if (prompt_location != NULL) {
			response_index = prompt_location - response;
		}
	}
	return response_index;
}

/**
 * Send a command to the PA microcontroller
 * @param command The command to send
 * @return true if it was sent, false if it was not sent
 */
bool pa_write_command(const char* command) {
	// Set blocking mode for writing
	if (pa_uart_set_blocking() == false)
		return false;

	unsigned int total_written = 0;
	unsigned int chars_to_write = strlen(command);
	while (total_written < chars_to_write) {
		int write_result = write(pa_uart_fd, command, strlen(command));
		if (write_result < 0) {
			log_error_errno("Write failed");
			return false;
		}
		total_written += write_result;
		if (pa_verbose) {
			log_info("Sent %d bytes, %d total", write_result, total_written);
		}
	}
	// If the command did not end in '\n', send it now
	if (command[chars_to_write - 1] != '\r') {
		int write_result = 0;
		while (write_result < 1) {
			write_result = write(pa_uart_fd, "\r", 1);
			if (write_result < 0) {
				log_error_errno("Final write failed");
				return false;
			}
		}
	}
	return true;
}

/**
 * Read a response from the PA microcontroller until either:
 * - A prompt is found
 * - A timeout occurred
 * If a timeout occurred, the data is logged and discarded.
 */
char* pa_read_until_prompt(unsigned int timeout_ms)
{
#ifdef INTERACTIVE_TESTS
	// Set blocking mode for reading (for stdio)
	if (pa_uart_set_blocking() == false)
#else
	// Set non-blocking mode for reading
	if (pa_uart_set_nonblocking() == false)
#endif // INTERACTIVE_TESTS
		return NULL;

	// Reserve 256 bytes of memory at a time
	unsigned int chunk_size = 256;
	// Allocate initial memory for the response
	char* response = malloc_string(chunk_size);
	if (response == NULL) {
		log_error_errno("Could not allocate memory");
		return NULL;
	}

	// Record the start time, for the timeout
	long long start_ms = monotonic_ms();
	long long elapsed_ms = 0;
	unsigned int response_buffer_size = chunk_size;
	unsigned int total_response_read = 0;
	int response_position = -1;
	int prompt_length = strlen(PA_PROMPT_RESPONSE);
	// Note: This uses a lot of CPU.
	//       It could be rewritten to use select(), but it is a bit more complicated.
	while (elapsed_ms < timeout_ms) {
		// Read one byte at a time
		ssize_t bytes_read = read(pa_uart_fd, response + total_response_read, 1);
		if (bytes_read > 0)
			total_response_read += bytes_read;
		// See if the response was found
		int possible_response_position = total_response_read - prompt_length;
		if (possible_response_position < 0)
			possible_response_position = 0;
		response_position = find_response_end(response, possible_response_position);
		if (response_position != -1) {
			// Done!
			// Remove the prompt from the response before returning.
			response[response_position] = 0;
			break;
		}
		// Make sure there is room for one more byte plus a nul terminator
		if ((total_response_read + 2) == response_buffer_size) {
			// Grow the response buffer
			response_buffer_size += chunk_size;
			char* new_response = realloc(response, response_buffer_size);
			if (new_response == NULL) {
				log_error_errno("Could not grow response");
				free(response);
				return NULL;
			}
			response = new_response;
		}
		// Update the elapsed time
		elapsed_ms = monotonic_ms() - start_ms;
	}
	if (pa_verbose) {
		log_info("Read took %d ms", elapsed_ms);
	}

	if (response_position == -1) {
		// A timeout occurred and the full response was not found
		response[total_response_read] = 0;
		log_error("Prompt not found from the microcontroller after %d ms, read %d bytes:\n%s\n",
			timeout_ms, total_response_read, response);
		free(response);
		return NULL;
	}

	return response;
}

/**
 * Run a PA board command
 * @param command The command to run
 * @param timeout_ms The number of milliseconds to wait for a response
 * @return The response string (must be freed,) or NULL on error or timeout.
 */
char* pa_run_command(const char* command, unsigned int timeout_ms) {
	// Delete the contents of the input and output buffers.
	// If this fails, it may affect the command and response,
	// but there is not much we can do about it.
	pa_uart_flush();

	// Verify there is a command
	if (command == NULL || command[0] == '\0') {
		log_error("No command was given");
		return NULL;
	}
	if (pa_verbose) {
		log_info("Sending %s", command);
	}

	// Send the command
	if (pa_write_command(command) == false)
		return NULL;

	return pa_read_until_prompt(timeout_ms);
}

int pa_main(int argc, char** argv) {
	if (argc < 3) {
		fprintf(stderr, "Usage: cameleon pa <COMMAND> <TIMEOUT_MS>\n");
		return 1;
	}
	if (pa_uart_open(PA_UART_DEVICE, PA_UART_BAUDRATE) == -1)
		return 1;
	char* response = pa_run_command(argv[1], atoi(argv[2]));
	if (response != NULL) {
		printf("Response was: %s\n", response);
		free(response);
	}
	return 0;
}

#ifdef UNIT_TESTS

#define TEST_RETURNS_EXPECTED(EXPECTED, FUNCTION, ...)	\
do {													\
    if(FUNCTION(__VA_ARGS__) != EXPECTED)				\
        printf("Test failed on line %d\n", __LINE__);	\
} while(false)

void test_join_strings() {
	// Join two small strings
	char* t1_first = malloc_string(2);
	strncpy(t1_first, "1", 2);
	char* t1_second = malloc_string(3);
	strncpy(t1_second, "23", 3);
	char* t1_joined = join_strings(t1_first, t1_second);
	if (t1_joined == NULL)
		printf("t1_joined was NULL\n");
	if (strlen(t1_joined) != 3)
		printf("t1_joined had unexpected length of %d\n", (int)strlen(t1_joined));
	if (strcmp(t1_joined, "123") != 0)
		printf("t1_joined was not \"123\": %s\n", t1_joined);
	free(t1_joined);
}

void test_find_response_end() {
	TEST_RETURNS_EXPECTED(4, find_response_end, "asdf\r\n>", 0);
	TEST_RETURNS_EXPECTED(4, find_response_end, "asdf\r\n>", 1);
	TEST_RETURNS_EXPECTED(4, find_response_end, "asdf\r\n>", 4);
	TEST_RETURNS_EXPECTED(-1, find_response_end, "asdf\r\n>", 5);
	TEST_RETURNS_EXPECTED(-1, find_response_end, "asdf\n>", 0);
	TEST_RETURNS_EXPECTED(-1, find_response_end, "\r\n", 0);
}

/**
 * An interactive test of pa_read_until_prompt()
 * Type a response on the terminal, then press Enter twice and finally ">".
 * You will have to press Enter one more time in order for the read() to
 * stop blocking and accept the final ">".
 */
void test_pa_read_until_prompt() {
	int timeout = 5000;
	printf("Enter a response, ending with \\n\\n>, timeout=%dms: ", timeout);
	fflush(stdout);
	char* response = pa_read_until_prompt(timeout);
	if (response != NULL) {
		printf("Found response:\n%s\n", response);
	}
	else {
		printf("Did not find response\n");
	}
}

// To compile and run this:
// gcc -g3 -o /tmp/test -D UNIT_TESTS -D INTERACTIVE_TESTS log.c hw_pa.c -l ubsan && /tmp/test
int main(int argc, char**argv) {
	log_init(LEVEL_ALL, "/tmp/test.log");
	test_join_strings();
	test_find_response_end();
#ifdef INTERACTIVE_TESTS
	// Interactive test:
	// Note: Non-blocking reads do not work well with STDIN_FILENO...
	//       it just returns from read() when enter is pressed.
	pa_uart_fd = STDIN_FILENO;
	test_pa_read_until_prompt();
#endif // INTERACTIVE_TESTS
}
#endif // UNIT_TESTS
