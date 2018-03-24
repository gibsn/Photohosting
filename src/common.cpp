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

#include "cfg.h"
#include "log.h"


static void print_help()
{
    fprintf(stderr,
        "Photohosting by gibsn\n\n"
        "Available options:\n"

        " -c [string]: path to config file\n"
        " -h         : print this message\n"
    );
}


bool process_cmd_arguments(int argc, char **argv, Config &cfg)
{
    int c;
    while ((c = getopt(argc, argv, "hc:")) != -1) {
        switch(c) {
        case 'c':
            cfg.path_to_cfg = strdup(optarg);
            break;
        case 'h':
            print_help();
            exit(0);
        case '?':
        case ':':
            return false;
        default:
            ;
        }
    }

    return true;
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

ByteArray::~ByteArray()
{
    free(data);
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


void ByteArray::Reset()
{
    this->~ByteArray();
    memset(this, 0, sizeof(*this));
}


ByteArray *read_file(const char *path)
{
    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        if (errno == ENOENT) {
            LOG_W("read_file: ould not read file %s: %s", path, strerror(errno));
        } else {
            LOG_E("read_file: ould not read file %s: %s", path, strerror(errno));
        }

        return NULL;
    }

    struct stat file_stat;
    int ret = fstat(fd, &file_stat);
    if (ret) {
        close(fd);
        LOG_E("read_file: could not fstat %s: %s", path, strerror(errno));
        return NULL;
    }

    char *buf = (char *)malloc(file_stat.st_size);
    int n = read(fd, buf, file_stat.st_size);
    close(fd);
    if (n != file_stat.st_size) {
        LOG_E("read_file: ould not read file %s", path);
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

char *_gen_random_string(int length) {
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
    if (!path) {
        return -1;
    }

    int ret = 0;
    char *_path = strdup(path);
    char *curr_dir = strchr(_path + 1, '/');

    while(curr_dir) {
        char c = *curr_dir;
        *curr_dir = '\0';

        if (!file_exists(_path)) {
            if (mkdir(_path, mode)) {
                ret = -1;
                break;
            }
        }

        *curr_dir = c;
        curr_dir = strchr(++curr_dir, '/');
    }

    free(_path);
    return ret;
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
        LOG_E("rm: could not delete %s: %s", path, strerror(errno));
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
            LOG_E("rm: filepath excedeed the size of buf");
            return -1;
        }

        if (!is_regular_file(new_path)) rm_rf(new_path);

        int err = remove(new_path);
        if (err) {
            LOG_E("rm: could not delete %s: %s", new_path, strerror(errno));
            return -1;
        }

        free(new_path);
    }

    closedir(dir);

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
        LOG_E("runas: wrong runas option %s", runas);
        goto fin;
    }

	*colon = '\0';

	username = _runas;
	groupname = colon + 1;

    pwd = getpwnam(username);
	if (pwd == NULL) {
		LOG_E("runas: wrong username %s", username);
        goto fin;
    }

    grp = getgrnam(groupname);
	if (grp == NULL) {
		LOG_E("runas: wrong groupname %s", groupname);
        goto fin;
    }

	if (setgid(grp->gr_gid) || setuid(pwd->pw_uid)) {
		LOG_E("runas: setgid/setuid failed: %s", strerror(errno));
        goto fin;
    }

#ifdef __linux__
    // Linux will prohibit creating core dumps after the uid has been changed
	prctl(PR_SET_DUMPABLE, 1, 0, 0, 0);
#endif

    ret = true;
    LOG_I("runas: running as %s", runas);

fin:
    if (_runas) free(_runas);
    return ret;
}
