#include "common.h"

#include <arpa/inet.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <string.h>

#ifdef __linux__
#include <sys/prctl.h>
#endif

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "log.h"


Config::Config()
    : port(0),
    addr(0),
    n_workers(0),
    max_log_level(-1),
    path_to_static(NULL),
    path_to_tmp_files(NULL),
    runas(NULL)
{}


Config::~Config()
{
    if (addr) free(addr);
    if (path_to_static) free(path_to_static);
    if (path_to_tmp_files) free(path_to_tmp_files);
    if (runas) free(runas);
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

    if (!path_to_tmp_files) {
        fprintf (stderr,
            "You have not specified path to the tmp files, "
            "using the current directory by default\n");
        path_to_tmp_files = strdup(".");
    }
}


ByteArray::ByteArray(const char *_data, int _size)
    : data(NULL),
    size(_size),
    cap(_size)
{
    if (_data) {
        data = (char *)malloc(_size);
        memcpy(data, _data, _size);
    }
}


void ByteArray::Append(const ByteArray *arr)
{
    if (arr && arr->data) {
        if (cap < size + arr->size) {
            data = (char *)realloc(data, size + arr->size);
            cap = size + arr->size;
        }

        memcpy(data + size, arr->data, arr->size);
        size += arr->size;
    }
}


static void print_help()
{
    fprintf(stderr,
        "HTTP-server\n\n"
        "Available options:\n"
        "  -i [a.b.c.d]: address to listen on\n"
        "  -p [int]: port to listen on\n"
        "  -n [int]: amount of workers\n"
        "  -l [0-7]: level of logging\n"
        "  -s [string]: path to static files\n"
        "  -r [user:group]: setuid/setgid after binding\n"
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
    while ((c = getopt(argc, argv, "hi:p:n:l:s:r:")) != -1) {
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
        case 'r':
            cfg.runas = strdup(optarg);
            if (!cfg.runas) {
                fprintf(stderr, "Wrong runas option (%s), must be user:group\n",
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

char *gen_random_string(int length) {
    static char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890";
    char *result = (char *)malloc(length + 1);

    for (int i = 0; i < length; i++) {
        result[i] = charset[rand() % (sizeof charset - 1)];
    }

    result[length] = '\0';

    return result;
}


int mkdir_p(const char *path, mode_t mode)
{
    if (!path) return true;

    char *curr_dir = strchr(path + 1, '/');
    while(curr_dir) {
        char c = *curr_dir;
        *curr_dir = '\0';

        if (!file_exists(path)) {
            if (mkdir(path, mode)) return -1;
        }

        *curr_dir = c;
        curr_dir = strchr(++curr_dir, '/');
    }

    return 0;
}


static int is_regular_file(const char *path)
{
    struct stat path_stat;
    stat(path, &path_stat);

    return S_ISREG(path_stat.st_mode);
}


int rm_rf(const char *path)
{
    DIR *dir = opendir(path);
    if (!dir) {
        LOG_E("Could not delete %s: %s", path, strerror(errno));
        return -1;
    }

    struct dirent *next_file;
    while ((next_file = readdir(dir)) != NULL) {
        if (0 == strcmp(next_file->d_name, ".") ||
            0 == strcmp(next_file->d_name, "..")
        ) {
            continue;
        }

        char *new_path = (char *)malloc(512);
        int n = snprintf(new_path, 512, "%s/%s", path, next_file->d_name);
        if (n < 0 || n >= 512) {
            LOG_E("rm_all_files(): filepath excedeed the size of buf");
            return -1;
        }

        if (!is_regular_file(new_path)) rm_rf(new_path);

        int err = remove(new_path);
        if (err) {
            LOG_E("Could not delete %s: %s", new_path, strerror(errno));
            return -1;
        }

        free(new_path);
    }

    if (dir) closedir(dir);

    return 0;
}


bool change_user(const char *runas)
{
    bool ret = false;

	struct passwd *pwd;
    struct group *grp;
    char *username, *groupname;

    char *_runas = NULL;
    _runas = strdup(runas);

	char *colon = strchr(_runas, ':');
	if (colon == NULL) {
        LOG_E("Wrong runas option %s", runas);
        goto fin;
    }

	*colon = '\0';

	username = _runas;
	groupname = colon + 1;

    pwd = getpwnam(username);
	if (pwd == NULL) {
		LOG_E("Wrong username %s", username);
        goto fin;
    }

    grp = getgrnam(groupname);
	if (grp == NULL) {
		LOG_E("Wrong groupname %s", groupname);
        goto fin;
    }

	if (setgid(grp->gr_gid) || setuid(pwd->pw_uid)) {
		LOG_E("setgid/setuid failed: %s", strerror(errno));
        goto fin;
    }

#ifdef __linux__
    // Linux will prohibit creating core dumps after the uid has been changed
	prctl(PR_SET_DUMPABLE, 1, 0, 0, 0);
#endif

    ret = true;

fin:
    if (_runas) free(_runas);
    return ret;
}
