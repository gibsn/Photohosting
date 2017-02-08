#include "http_server.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include "WebAlbumCreator.h"

#include "common.h"
#include "http_request.h"
#include "http_response.h"
#include "http_session.h"
#include "log.h"
#include "web_album_creator_preprocessor.h"


char *HttpServer::AddPathToStaticPrefix(const char *path) const
{
    int file_path_len = strlen(path) - (sizeof "/static" - 1) + 1;
    file_path_len += path_to_static_len;

    char *file_path = (char *)malloc(file_path_len);
    if (!file_path) return NULL;

    strcpy(file_path, path_to_static);
    strcat(file_path, path + sizeof "/static" - 1);

    return file_path;
}


//TODO: not sure if it is good to return TcpSession * here
TcpSession *HttpServer::CreateNewSession()
{
    TcpSession *tcp_session = TcpServer::CreateNewSession();
    // TODO: here TcpServer returns NULL when EAGAIN happens as well
    if (!tcp_session) {
        LOG_D("Could not open a new HTTP-session");
        return NULL;
    }

    HttpSession *http_session = new HttpSession(tcp_session, this);

    tcp_session->SetSessionDriver(http_session);

    return tcp_session;
}


// TODO: should differentiate situations when file doesnt exist and is forbidden
ByteArray *HttpServer::GetFileByLocation(const char *path)
{
    char *file_path = AddPathToStaticPrefix(path);

    ByteArray *file = read_file(file_path);
    free(file_path);

    return file;
}


char *HttpServer::SaveFile(ByteArray *file, char *name)
{
    //TODO: name can be too long
    char *full_path = (char *)malloc(strlen(path_to_tmp_files) + 2 + strlen(name));
    strcpy(full_path, path_to_tmp_files);
    strcat(full_path, "/");
    strcat(full_path, name);

    int n;
    int fd = open(full_path, O_CREAT | O_WRONLY, 0666);
    if (fd == -1) {
        LOG_E("%s", strerror(errno));
        goto err;
    }

    n = write(fd, file->data, file->size);

    if (file->size != n) {
        LOG_E("Could not write file %s", full_path);
        goto err;
    }

    return full_path;

err:
    if (full_path) free(full_path);

    return NULL;
}


bool HttpServer::CreateUserPaths(
        const char *user_path,
        const char *srcs_path,
        const char *thmbs_path) const
{
    if (!file_exists(user_path)) {
        if (mkdir(user_path, 0777)) {
            LOG_E("Could not create directory %s: %s", user_path, strerror(errno));
            return false;
        }

        LOG_I("Created directory %s", user_path);
    }

    if (mkdir_p(srcs_path, 0777)) {
        LOG_E("Could not create directory %s: %s", srcs_path, strerror(errno));
        return false;
    }

    LOG_I("Created directory %s", srcs_path);

    if (mkdir_p(thmbs_path, 0777)) {
        LOG_E("Could not create directory %s: %s", thmbs_path, strerror(errno));
        return false;
    }

    LOG_I("Created directory %s", thmbs_path);

    return true;
}


char *HttpServer::CreateAlbum(const char *user, char *archive, const char *title)
{
    int random_id_len = 16;
    char *random_id = gen_random_string(random_id_len);
    WebAlbumParams cfg;

    cfg.path_to_archive = archive;
    cfg.web_page_title = title;

    char *user_path = make_user_path(path_to_static, (char *)user);

    cfg.path_to_unpack = make_path_to_unpack(user_path, random_id);
    cfg.path_to_thumbnails = make_path_to_thumbs(user_path, random_id);
    cfg.path_to_webpage = make_path_to_webpage(user_path, random_id);

    cfg.path_to_css = make_path_to_css(path_to_css);

    cfg.relative_path_to_originals = make_r_path_to_srcs((char *)user, random_id);
    cfg.relative_path_to_thumbnails = make_r_path_to_thmbs((char *)user, random_id);

    char *r_path_to_web_page = make_r_path_to_webpage((char *)user, random_id);

    CreateUserPaths(user_path, cfg.path_to_unpack, cfg.path_to_thumbnails);

    LOG_D("path_to_archive: %s", cfg.path_to_archive);
    LOG_D("web_page_title: %s", cfg.web_page_title);
    LOG_D("path_to_srcs: %s", cfg.path_to_unpack);
    LOG_D("path_to_thumbs: %s", cfg.path_to_thumbnails);
    LOG_D("path_to_webpage: %s", cfg.path_to_webpage);
    LOG_D("path_to_css: %s", cfg.path_to_css);
    LOG_D("relative_path_to_srcs: %s", cfg.relative_path_to_originals);
    LOG_D("relative_path_to_thmbs: %s", cfg.relative_path_to_thumbnails);

    // TODO: need diags here
    CreateWebAlbum(cfg);

    int err = remove(archive);
    if (err) {
        LOG_E("Could not delete %s: %s", archive, strerror(errno));
    }

    free(random_id);

    free(user_path);
    free((char *)cfg.path_to_unpack);
    free((char *)cfg.path_to_thumbnails);
    free((char *)cfg.path_to_webpage);
    free((char *)cfg.path_to_css);
    free((char *)cfg.relative_path_to_originals);
    free((char *)cfg.relative_path_to_thumbnails);

    return r_path_to_web_page;
}


HttpServer::HttpServer()
    : path_to_static(NULL),
    path_to_static_len(0),
    path_to_tmp_files(NULL)
{
}


HttpServer::HttpServer(const Config &cfg)
    : TcpServer(cfg),
    path_to_static_len(0)
{
    path_to_static = strdup(cfg.path_to_static);
    assert(path_to_static);
    path_to_static_len = strlen(path_to_static);

    path_to_tmp_files = strdup(cfg.path_to_tmp_files);
    assert(path_to_tmp_files);

    //TODO: will make it the proper way when I have config
    path_to_css = strdup("css/");
    assert(path_to_css);
}


HttpServer::~HttpServer()
{
    if (path_to_static) free(path_to_static);
    if (path_to_tmp_files) free(path_to_tmp_files);
    if (path_to_css) free(path_to_css);
}

