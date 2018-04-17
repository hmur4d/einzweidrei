#include "std_includes.h"
#include "log.h"

#define init_module(mod, len, opts) syscall(__NR_init_module, mod, len, opts)
#define delete_module(mod, flags) syscall(__NR_delete_module, mod, flags)

static bool read_retry(int fd, void* buffer, size_t len) {
	size_t remaining = len;
	int total = 0;
	do {
		int nread = read(fd, buffer + total, remaining);
		if (nread < 0) {
			log_error_errno("Unable read full module, read %d of %d bytes", total, len);
			close(fd);			
			return false;
		}

		total += nread;
		remaining -= nread;
	} while (remaining > 0);

	return true;
}

bool module_load(const char* filename, const char* param_values) {
	log_info("Loading module %s", filename);

	int fd = open(filename, O_RDONLY);
	if (fd < 0) {
		log_error_errno("Unable to open module file %s", filename);
		return false;
	}

	struct stat fd_stat;
	if (fstat(fd, &fd_stat) == -1) {
		log_error_errno("Error with fstat on module file %s", filename);
		close(fd);
		return false;
	}

	size_t len = fd_stat.st_size;
	void* module_image = malloc(len);
	if (module_image == NULL) {
		log_error_errno("Unable to malloc %d bytes", len);
		close(fd);
		return false;
	}

	if (!read_retry(fd, module_image, len)) {
		close(fd);
		free(module_image);
		return false;
	}

	int res = init_module(module_image, len, param_values);
	if (res != 0 && errno == EEXIST) {
		log_warning("Module %s is already loaded", filename);
		res = 0;
	} else if(res != 0) {
		log_error_errno("Unable to load module %s", filename);
	}

	free(module_image);
	close(fd);

	return res == 0;
}

bool module_unload(const char* filename) {
	log_info("Unloading module %s", filename);
	char* module_path = strdup(filename);
	if (module_path == NULL) {
		log_error_errno("Unable to copy module path");
		return false;
	}

	//find module name from path: remove directories and file extension
	char* module_name = basename(module_path);
	int len = strlen(module_name);
	if (strcmp(module_name + len - 3, ".ko") == 0) {
		module_name[len - 3] = '\0';
	}

	int res = delete_module(module_name, O_NONBLOCK);
	if (res < 0) {
		log_error_errno("Unable to unload module");
	}

	free(module_path);
	return res == 0;
}