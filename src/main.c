#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MAX_LINE_LENGTH 1024
#define MAX_EVENTS 256

typedef struct {
	char event_title[MAX_LINE_LENGTH];
	long long event_time;
} Event;

long long current_time_ms() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (long long)(tv.tv_sec) * 1000 + (tv.tv_usec / 1000);
}

int main(int argc, char *argv[]) {
	Event *events = malloc(MAX_EVENTS * sizeof(Event));
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

	char line[MAX_LINE_LENGTH];
	int i = 0;
	while (fgets(line, sizeof(line), file)) {
		line[strcspn(line, "\n")] = '\0';
		if (i%2 == 0) {
			strncpy(events[i/2].event_title, line, MAX_LINE_LENGTH);
		} else {
			long long timeMS = 0LL;
			sscanf(line, "%lld", &timeMS);
			events[i/2].event_time = timeMS;
		}
		i++;
	}

	if (argc > 1) {
		if (strcmp(argv[1], "-add") == 0) {
			if (argc < 3) {
				perror("Please include the name of the thing to track");
				return 1;
			}
			fclose(file);

			file = fopen(filepath, "a");
			if (!file) {
				perror("Error opening file to append");
				return 1;
			}

			fprintf(file, "%s\n", argv[2]);
			fprintf(file, "%lld\n", current_time_ms());
		} else if (strcmp(argv[1], "-clearall") == 0) {
			file = fopen(filepath, "w");
			fclose(file);
		} else if (strcmp(argv[1], "-remove") == 0) {
			if (argc < 3) {
				fprintf(stderr, "Error: Please provide the event title to remove.\n");
				return 1;
			}

			// Open temp file
			FILE *temp_file = fopen("temp.dsl", "w");
			if (!temp_file) {
				perror("Error opening temporary file");
				return 1;
			}

			char event_title[MAX_LINE_LENGTH];
			char event_time[MAX_LINE_LENGTH];
			int removed = 0;
			rewind(file);

			printf("check start\n");
			while (fgets(event_title, sizeof(event_title), file) && fgets(event_time, sizeof(event_time), file)) {
				event_title[strcspn(event_title, "\n")] = '\0';
				printf("checking %s, %s\n", event_title, argv[2]);
				if (strcmp(event_title, argv[2]) == 0) {
					removed = 1;
					continue;
				}
				fprintf(temp_file, "%s\n", event_title);
				fprintf(temp_file, "%s", event_time);
			}
			printf("check end\n");

			fclose(file);
			fclose(temp_file);

			if (!removed) {
				printf("No event found with title: %s\n", argv[2]);
				remove("temp.dsl");  // Clean up temp file
				return 1;
			}

			// Replace the original file with the temporary file
			if (remove(filepath) != 0) {
				perror("Error deleting original file");
				return 1;
			}
			if (rename("temp.dsl", filepath) != 0) {
				perror("Error renaming temporary file");
				return 1;
			}

			printf("Event '%s' removed successfully.\n", argv[2]);
		}
	} else {
		printf("Days Since Last\n");
		rewind(file);

		char line[MAX_LINE_LENGTH];
		while (fgets(line, sizeof(line), file)) {
			printf("%s", line);
		}

		fclose(file);
		return 0;
	}
}
