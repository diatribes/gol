#define _POSIX_C_SOURCE 199309L
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include "sleep_ms.h"
#include "isnumeric.h"


#define ANSI_COLOR_INVERT   "\x1b[7m"
#define ANSI_COLOR_WHITE    "\x1b[107m"
#define ANSI_COLOR_RED      "\x1b[41m"
#define ANSI_COLOR_GREEN    "\x1b[32m"
#define ANSI_COLOR_YELLOW   "\x1b[33m"
#define ANSI_COLOR_BLUE     "\x1b[44m"
#define ANSI_COLOR_MAGENTA  "\x1b[35m"
#define ANSI_COLOR_CYAN     "\x1b[36m"
#define ANSI_COLOR_RESET    "\x1b[0m"
#define ANSI_CURSOR_RESET   "\x1b[1;1f"
#define ANSI_TERM_CLEAR     "\x1B[1;1f\x1B[J"

#define default_rows 30
#define default_cols 80
#define print(s) fputs((s), stdout);

#define cell_state_dead 0
#define cell_state_alive 1
#define cell_state_none 2

struct config {
    int seed;
    int rows;
    int cols;
    int generate_terrain;
} config = { 0, default_rows, default_cols, 0 };

void (*cell_print)(int, int);
char *universe = NULL;
volatile int done = 0;

static void print_usage()
{
    fprintf(stderr,
            "Usage: gol [OPTIONS]\n"
            "   -t     generate terrain\n"
            "   -r N   set number of rows to N.\n"
            "   -c N   set number of columns to N.\n");
    fprintf(stderr, "e.g., gol -t -r 20 -c 30\n");
}

static void handle_signal(int signum)
{
    switch (signum) {
    case SIGINT:
    case SIGTERM:
        done = 1;
        break;
    }
}

static void cursor_reset()
{
    print(ANSI_CURSOR_RESET);
}

static char cell_get(int x, int y)
{
    return universe[((y + config.rows) % config.rows) * config.cols
                    + ((x + config.cols) % config.cols)];
}

static void cell_set(int x, int y, char value)
{
    universe[((y + config.rows) % config.rows) * config.cols
             + ((x + config.cols) % config.cols)] = value;
}

static int cell_tick(int x, int y)
{
    int r1, c1;
    int n = 0;
    for (r1 = y - 1; r1 <= y + 1; r1++) {
        for (c1 = x - 1; c1 <= x + 1; c1++) {
            if (cell_get(c1, r1) == cell_state_alive) {
                n++;
            }
        }
    }

    int was_alive = 0;
    if (cell_get(x, y) == cell_state_alive) {
        was_alive = 1;
        n--;
    }

    int cell_state = cell_get(x, y);
    int is_alive = (n == 3 || (n == 2 && cell_state == cell_state_alive));
    if (was_alive && !is_alive) {
        cell_state = cell_state_dead;
    } else if (is_alive) {
        cell_state = cell_state_alive;
    }

    return cell_state;
}

static void cell_print_plain(int x, int y)
{
    switch (cell_get(x, y)) {
    case cell_state_dead:
        print(ANSI_COLOR_WHITE " ");
        break;
    case cell_state_alive:
        print(ANSI_COLOR_RED " ");
        print(ANSI_COLOR_WHITE);
        break;
    case cell_state_none:
        print(ANSI_COLOR_WHITE " ");
        break;
    }
}

static void cell_print_terrain(int x, int y)
{
    switch (cell_get(x, y)) {
    case cell_state_dead:
        print(ANSI_COLOR_GREEN "#");
        break;
    case cell_state_alive:
        print(ANSI_COLOR_YELLOW "^");
        break;
    case cell_state_none:
        print(ANSI_COLOR_BLUE ";");
        break;
    }
}

static void universe_init()
{
    int y, x;
    srand(config.seed);
    universe = malloc(config.rows * config.cols * sizeof(char));
    if (!universe) {
        print("Out of memory");
        exit(1);
    }
    for (y = 0; y < config.rows; y++) {
        for (x = 0; x < config.cols; x++) {
            if (rand() < RAND_MAX / 2) {
                cell_set(x, y, cell_state_alive);
            } else {
                cell_set(x, y, cell_state_none);
            }
        }
    }
}

static void universe_tick()
{
    char new[config.rows * config.cols];
    int y, x;
    int cell_state;

    for (y = 0; y < config.rows; y++) {
        for (x = 0; x < config.cols; x++) {
            cell_state = cell_tick(x, y);
            new[y * config.cols + x] = cell_state;
        }
    }

    for (y = 0; y < config.rows; y++) {
        for (x = 0; x < config.cols; x++) {
            cell_state = new[y * config.cols + x];
            cell_set(x, y, cell_state);
        }
    }

}

static void universe_print()
{
    int y, x;

    cursor_reset();
    for (y = 0; y < config.rows; y++) {
        for (x = 0; x < config.cols; x++) {
            cell_print(x, y);
        }
        print("\n");
    }
}

static void set_options(int argc, char **argv)
{
    char c;
    cell_print = cell_print_plain;
    while ((c = getopt(argc, argv, "tgr:c:s:-")) != -1) {
        switch (c) {
        case 't':
            config.generate_terrain = 1;
            cell_print = cell_print_terrain;
            break;
        case 'r':
            if (isnumeric(optarg)) {
                config.rows = atoi(optarg);
            } else {
                print("Option -r requires a numeric argument");
                print_usage();
                exit(1);
            }
            break;
        case 'c':
            if (isnumeric(optarg)) {
                config.cols = atoi(optarg);
            } else {
                print("Option -r requires a numeric argument");
                print_usage();
                exit(1);
            }
            break;
        case 's':
            if (isnumeric(optarg)) {
                config.seed = atoi(optarg);
            } else {
                print("Option -s requires a numeric argument");
                print_usage();
                exit(1);
            }
            break;
        case 'h':
        case '-':
            print_usage();
            exit(1);
            break;
        case '?':
            if (optopt == 'r' || optopt == 'c' || optopt == 's') {
                fprintf(stderr, "Option -%c requires an argument.\n",
                        optopt);
            } else if (isprint(optopt)) {
                fprintf(stderr, "Unknown option `-%c'.\n", optopt);
            } else {
                fprintf(stderr, "Unknown option character `\\x%x'.\n",
                        optopt);
            }
            print_usage();
            exit(1);
        }
    }
}

int main(int argc, char *argv[])
{
    config.seed = time(NULL);
    set_options(argc, argv);
    printf("\n%d\n", config.seed);
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    universe_init();
    print(ANSI_COLOR_RESET);
    print(ANSI_TERM_CLEAR);

    if (config.generate_terrain) {
        int x, y;
        universe_tick();
        universe_tick();
        for (y = 0; y < config.rows; y++) {
            for (x = 0; x < config.cols; x++) {
                if (cell_get(x, y) == cell_state_dead) {
                    cell_set(x, y, cell_state_none);
                }
            }
        }
    }

    while (!done) {
        universe_print();
        sleep_ms(100);
        universe_tick();
    }

    free(universe);
    print(ANSI_COLOR_RESET);
    fflush(stdout);

    return 0;
}
