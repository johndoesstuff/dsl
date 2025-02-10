#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>

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

void restore_cursor(int signum) {
	(void)signum;
	printf("\033[?25h\n");
	fflush(stdout);
	exit(1);
}

void disable_keyboard() {
	int dev_null = open("/dev/null", O_RDONLY);
	dup2(dev_null, STDIN_FILENO); 
	close(dev_null);
}

int main(int argc, char *argv[]) {
	//exit handler
	struct sigaction sa;
	sa.sa_handler = restore_cursor;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction(SIGINT, &sa, NULL);

	disable_keyboard();

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
	int event_count = 0;
	while (fgets(line, sizeof(line), file)) {
		line[strcspn(line, "\n")] = '\0';
		if (i%2 == 0) {
			strncpy(events[i/2].event_title, line, MAX_LINE_LENGTH);
			event_count++;
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
			printf("Event %s added successfully.\n", argv[2]);
		} else if (strcmp(argv[1], "-clearall") == 0) {
			file = fopen(filepath, "w");
			fclose(file);
			printf("Events cleared successfully.\n");
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

			while (fgets(event_title, sizeof(event_title), file) && fgets(event_time, sizeof(event_time), file)) {
				event_title[strcspn(event_title, "\n")] = '\0';
				if (strcmp(event_title, argv[2]) == 0) {
					removed = 1;
					continue;
				}
				fprintf(temp_file, "%s\n", event_title);
				fprintf(temp_file, "%s", event_time);
			}

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
		} else if (strcmp(argv[1], "-update") == 0) {
			if (argc < 3) {
				fprintf(stderr, "Error: Please provide the event title to update.\n");
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
			int updated = 0;
			rewind(file);

			while (fgets(event_title, sizeof(event_title), file) && fgets(event_time, sizeof(event_time), file)) {
				event_title[strcspn(event_title, "\n")] = '\0';
				if (strcmp(event_title, argv[2]) == 0) {
					updated = 1;
					fprintf(temp_file, "%s\n", event_title);
					fprintf(temp_file, "%lld\n", current_time_ms());
				} else {
					fprintf(temp_file, "%s\n", event_title);
					fprintf(temp_file, "%s", event_time);
				}
			}

			fclose(file);
			fclose(temp_file);

			if (!updated) {
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

			printf("Event '%s' updated successfully.\n", argv[2]);
		} else if (strcmp(argv[1], "-help") == 0) {
			printf("dsl [<arguments>]\n");
			printf("\t-remove [event]\n");
			printf("\t\tRemove an event\n");
			printf("\t-add [event]\n");
			printf("\t\tAdd an event starting at the time of creation\n");
			printf("\t-clearall\n");
			printf("\t\tClear all active events\n");
			printf("\t-update [event]\n");
			printf("\t\tReset the counter of time since last event\n");
		}

	} else {
		printf("Days Since Last\n");
		rewind(file);
		printf("\033[?25l");

		while (1) {
			for (int i = 0; i < event_count; i++) {
				long long event_time_ms = events[i].event_time;
				long long now_time_ms = current_time_ms();
				long long time_since = now_time_ms - event_time_ms;
				int seconds = (int)(time_since/1000);
				int minutes = seconds/60;
				int hours = minutes/60;
				int days = hours/24;
				int years = days/365;
				seconds %= 60;
				minutes %= 60;
				hours %= 24;
				days %= 365;

				if (years > 0) {
					printf("%s: %dy, %dd, %02d:%02d:%02d                    \n", events[i].event_title, years, days, hours, minutes, seconds);
				} else if (days > 0) {
					printf("%s: %dd, %02d:%02d:%02d                    \n", events[i].event_title, days, hours, minutes, seconds);
				} else if (hours > 0) {
					printf("%s: %d:%02d:%02d                    \n", events[i].event_title, hours, minutes, seconds);
				} else if (minutes > 0) {
					printf("%s: %d:%02d                    \n", events[i].event_title, minutes, seconds);
				} else {	
					printf("%s: %d seconds                    \n", events[i].event_title, seconds);		
				}
			}

			//move line up to update
			for (int i = 0; i < event_count; i++) {
				printf("\033[A");
			}
		}
		fclose(file);
		return 0;
	}
}
