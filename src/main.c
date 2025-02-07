#include <time.h>
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
	const char *home = getenv("HOME");
	if (!home) {
		fprintf(stderr, "Error: HOME environment variable not set.\n");
	}

	char filepath[1024];
	snprintf(filepath, sizeof(filepath), "%s/.dsl", home);

	FILE *file = fopen(filepath, "r");
	if (!file) {
		file = fopen(filepath, "w");
		if (!file) {
			perror("Error creating file");
			return 1;
		}
		fclose(file);

		file = fopen(filepath, "r");
		if (!file) {
			perror("Error opening file after creation");
			return 1;
		}
	}


	printf("Days Since Last\n");
}
