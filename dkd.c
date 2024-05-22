/* https://github.com/f-h1/dkd/blob/master/dkd.c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libevdev/libevdev.h>
#include <libevdev/libevdev-uinput.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <getopt.h>

#define MAX_CONFIG_LINES 100
#define MAX_KEYS_PER_BIND 10

typedef struct {
    char keys[MAX_KEYS_PER_BIND][32];
    char command[256];
} KeyBind;

KeyBind keyBinds[MAX_CONFIG_LINES];
int keyBindCount = 0;
int verbose = 0;

void trim(char *str) {
    char *end;
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return;
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    *(end+1) = 0;
}

void read_config(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Unable to open configuration file: %s\n", filename);
        exit(1);
    }
    char line[512];
    while (fgets(line, sizeof(line), file)) {
        trim(line); // Trim the line to remove any trailing newlines/spaces
        char *keyPart = strtok(line, ":");
        char *commandPart = strtok(NULL, ":");
        
        if (keyPart && commandPart) {
            trim(keyPart);
            trim(commandPart);
            int keyIndex = 0;
            char *key = strtok(keyPart, "+");
            while (key && keyIndex < MAX_KEYS_PER_BIND) {
                trim(key);
                strcpy(keyBinds[keyBindCount].keys[keyIndex], key);
                keyIndex++;
                key = strtok(NULL, "+");
            }
            strcpy(keyBinds[keyBindCount].command, commandPart);
            keyBindCount++;
        }
    }
    fclose(file);
}

void execute_command(const char *command) {
    pid_t pid = fork();
    if (pid == 0) { // Child process
        execlp("sh", "sh", "-c", command, (char *)NULL);
        perror("execlp"); // If execlp fails
        exit(1);
    } else if (pid < 0) { // Fork failed
        perror("fork");
    }
    // Parent process continues
}

int main(int argc, char *argv[]) {
    char *config_file = NULL;
    char *input_device = NULL;
    int opt;

    while ((opt = getopt(argc, argv, "c:i:v")) != -1) {
        switch (opt) {
            case 'c':
                config_file = optarg;
                break;
            case 'i':
                input_device = optarg;
                break;
            case 'v':
                verbose = 1;
                break;
            default:
                fprintf(stderr, "Usage: %s -c <config file> -i <input device> [-v]\n", argv[0]);
                return 1;
        }
    }

    if (!config_file || !input_device) {
        fprintf(stderr, "Usage: %s -c <config file> -i <input device> [-v]\n", argv[0]);
        return 1;
    }

    read_config(config_file);

	//int fd = open(input_device, O_RDONLY | O_NONBLOCK);
    int fd = open(input_device, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "Unable to open input device: %s\n", input_device);
        return 1;
    }

    struct libevdev *dev = NULL;
    int rc = libevdev_new_from_fd(fd, &dev);
    if (rc < 0) {
        fprintf(stderr, "Failed to init libevdev (%s)\n", strerror(-rc));
        close(fd);
        return 1;
    }

    libevdev_grab(dev, LIBEVDEV_GRAB);

    if (verbose) {
        printf("Listening for events. Press Ctrl+C to terminate.\n");
    }
    struct input_event ev;
    char active_keys[KEY_CNT][32] = {0};  // Tracks active keys by name

    while (1) {
        int rd = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_BLOCKING, &ev);
        if (verbose) {
        	printf("----- Input received -----\n");
        }
        if (rd == LIBEVDEV_READ_STATUS_SYNC) {
            if (verbose) {
                printf("Sync: Drop events.\n");
            }
        } else if (rd == LIBEVDEV_READ_STATUS_SUCCESS && ev.type == EV_KEY) {
            const char* keyName = libevdev_event_code_get_name(ev.type, ev.code);
            if (ev.value == 1) { // Key press
                strcpy(active_keys[ev.code], keyName);
                if (verbose) {
                    printf("Key Pressed: %s\n", keyName);
                }
            } else if (ev.value == 0) { // Key release
                if (verbose) {
                    printf("Key Released: %s\n", keyName);
                }
                memset(active_keys[ev.code], 0, sizeof(active_keys[ev.code]));
            }

            // Check for matching key combination
            for (int i = 0; i < keyBindCount; i++) {
                int match = 1;
                for (int j = 0; j < MAX_KEYS_PER_BIND && keyBinds[i].keys[j][0] != '\0'; j++) {
                    int key_found = 0;
                    for (int k = 0; k < KEY_CNT; k++) {
                        if (strcmp(active_keys[k], keyBinds[i].keys[j]) == 0) {
                            key_found = 1;
                            break;
                        }
                    }
                    if (!key_found) {
                        match = 0;
                        break;
                    }
                }
                if (match) {
                    if (verbose) {
                        printf("Executing command: %s\n", keyBinds[i].command);
                    }
                    execute_command(keyBinds[i].command);
                }
            }
        }
    }

    libevdev_free(dev);
    close(fd);
    return 0;
}
