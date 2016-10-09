#include "common.h"

#include <arpa/inet.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "log.h"


Config::Config()
    : port(0),
    addr(0),
    n_workers(0)
{}


Config::~Config()
{
    if (addr) free(addr);
}


void Config::Check()
{
    if (!port) {
        LOG_W(
            "You have not specified any port, "
            "listening on 80 by default");
        port = 80;
    }

    if (!addr) {
        LOG_W(
            "You have not specified any ip, "
            "listening on 0.0.0.0 by default");
        addr = strdup("0.0.0.0");
    }

    if (!n_workers) {
        LOG_W(
            "You have not specified amount of workers, "
            "using 1 by default");
        n_workers = 1;
    }
}


void print_help()
{
    fprintf(stderr,
        "HTTP-server\n\n"
        "Available options:\n"
        "  -i [a.b.c.d]: address to listen on\n"
        "  -p [int]: port to listen on\n"
        "  -n [int]: amount of workers\n"
    );
}


bool process_cmd_arguments(int argc, char **argv, Config &cfg)
{
    if (argc == 1) {
        print_help();
        return false;
    }

    int c;

    while ((c = getopt(argc, argv, "hi:p:n:")) != -1) {
        switch(c) {
        case 'i':
            cfg.addr = strdup(optarg);
            if (!inet_aton(cfg.addr, 0)) {
                LOG_E("Invalid IP (%s), must be A.B.C.D", optarg);
                return false;
            }
            break;
        case 'p':
            cfg.port = strtol(optarg, (char **)NULL, 10);
            if (!cfg.port) {
                LOG_E("Wrong port value (%s), must be int", optarg);
                return false;
            }
            break;
        case 'n':
            cfg.n_workers = strtol(optarg, (char **)NULL, 10);
            if (!cfg.n_workers) {
                LOG_E("Wrong workers value (%s), must be int", optarg);
                return false;
            }
            break;
        case 'h':
            print_help();
            return false;
        case '?':
            return false;
        case ':':
            return false;
        default:
            ;
        }
    }

    return true;
}


void hexdump(uint8_t *buf, size_t len) {
    const int s = 16;

    for (size_t i = 0; i < len; i+=s) {
        fprintf(stderr, "%s", LOG_COLOUR_PURPLE);
        for (size_t j = i; j < (i + s); j++) {
            if (j < len) {
                fprintf(stderr, "%02x ", buf[j]);
            } else {
                fprintf(stderr, " ");
            }

            if (j == i+(s/2)-1) fprintf(stderr, "  ");
        }

        fprintf(stderr, " | ");

        for (size_t j = i; j < len && j < (i+s); j++) {
            fprintf(stderr, "%c", isprint(buf[j]) ? buf[j] : '.');
            if (j == i+(s/2)-1) fprintf(stderr, "  ");
        }

        fprintf(stderr, "\n");
    }

    fprintf(stderr, "%s", LOG_COLOUR_NORMAL);
}
