#include "common.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "log.h"


Config::Config()
    : port(0),
    addr(0),
    n_workers(0),
    max_log_level(-1),
    path_to_static(0)
{}


Config::~Config()
{
    if (addr) free(addr);
    if (path_to_static) free(path_to_static);
}


void Config::Check()
{
    if (!port) {
        fprintf(stderr,
            "You have not specified any port, "
            "listening on 80 by default\n");
        port = 80;
    }

    if (!addr) {
        fprintf(stderr,
            "You have not specified any ip, "
            "listening on 0.0.0.0 by default\n");
        addr = strdup("0.0.0.0");
    }

    if (!n_workers) {
        fprintf(stderr,
            "You have not specified amount of workers, "
            "using 1 by default\n");
        n_workers = 1;
    }

    if (max_log_level == -1) {
        fprintf (stderr,
            "You have not specified level of logging, "
            "using INFO by default\n");
        max_log_level = LOG_INFO;
    }

    if (!path_to_static) {
        fprintf (stderr,
            "You have not specified path to the static files, "
            "using the current directory by default\n");
        path_to_static = strdup(".");
    }
}


ByteArray::ByteArray(char *_data, int _size)
    : data(NULL),
    size(_size)
{
    if (_data) {
        data = (char *)malloc(_size);
        memcpy(data, _data, _size);
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
        "  -l [0-7]: level of logging\n"
        "  -s [string]: path to static files\n"
    );
}


bool process_cmd_arguments(int argc, char **argv, Config &cfg)
{
    if (argc == 1) {
        print_help();
        return false;
    }

    int c;

    //TODO: fix strtol bug
    while ((c = getopt(argc, argv, "hi:p:n:l:s:")) != -1) {
        switch(c) {
        case 'i':
            cfg.addr = strdup(optarg);
            if (!inet_aton(cfg.addr, 0)) {
                fprintf(stderr, "Invalid IP (%s), must be A.B.C.D", optarg);
                return false;
            }
            break;
        case 'p':
            cfg.port = strtol(optarg, (char **)NULL, 10);
            if (!cfg.port) {
                fprintf(stderr, "Wrong port value (%s), must be int\n", optarg);
                return false;
            }
            break;
        case 'n':
            cfg.n_workers = strtol(optarg, (char **)NULL, 10);
            if (!cfg.n_workers) {
                fprintf(stderr, "Wrong workers value (%s), must be int\n",
                    optarg);
                return false;
            }
            break;
        case 'l':
            cfg.max_log_level = strtol(optarg, (char**)NULL, 10);
            if (!cfg.max_log_level ) {
                fprintf(stderr, "Wrong log level (%s), must be [0-7]\n",
                    optarg);
                return false;
            }
            break;
        case 's':
            cfg.path_to_static = strdup(optarg);
            if (!cfg.path_to_static) {
                fprintf(stderr, "Wrong path to static files (%s), must be string\n",
                    optarg);
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


ByteArray *read_file(const char *path)
{
    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        LOG_E("Could not read file %s: %s", path, strerror(errno));
        return NULL;
    }

    struct stat file_stat;
    int ret = fstat(fd, &file_stat);
    if (ret) {
        close(fd);
        LOG_E("%s", strerror(errno));
        return NULL;
    }

    char *buf = (char *)malloc(file_stat.st_size);
    int n = read(fd, buf, file_stat.st_size);
    close(fd);
    if (n != file_stat.st_size) {
        LOG_E("Could not read file %s", path);
        free(buf);
        return NULL;
    }

    ByteArray *file = new ByteArray(NULL, file_stat.st_size);
    file->data = buf;

    return file;
}


bool file_exists(const char *path)
{
    return !access(path, R_OK);
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
