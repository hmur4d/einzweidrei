/* 
main(..) indirection:
either calls cameleon_main(..) in the normal case
or a test main(..)
*/

#include <string.h>

int cameleon_main(int argc, char** argv);
int test_main(int argc, char** argv);
int epcq_image_main(int argc, char** argv);

int main(int argc, char** argv) {
	if (argc > 1 && strcmp(argv[1], "test") == 0) {
		return test_main(argc-1, argv+1);
	}
	if (argc > 1 && strcmp(argv[1], "image") == 0) {
		return epcq_image_main(argc - 1, argv + 1);
	}
	else {
		return cameleon_main(argc, argv);
	}
}