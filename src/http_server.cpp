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

#include "auth.h"
#include "common.h"
#include "http_request.h"
#include "http_response.h"
#include "http_session.h"
#include "log.h"
#include "web_album_creator_helper.h"


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


char *HttpServer::CreateAlbum(char *user, char *archive, char *title, char **path)
{
    int random_id_len = 16;
    char *random_id = gen_random_string(random_id_len);

    char *user_path = make_user_path(path_to_static, (char *)user);

    WebAlbumParams cfg = album_params_helper((char *)user, user_path, random_id);
    cfg.path_to_archive = archive;
    cfg.web_page_title = title;
    cfg.path_to_css = make_path_to_css(path_to_css);

    album_creator_debug(cfg);

    create_user_paths(user_path, cfg.path_to_unpack, cfg.path_to_thumbnails);

    char *wac_err = NULL;
    try {
        CreateWebAlbum(cfg);
    }
    catch (WebAlbumCreatorEx &ex) {
        LOG_E("Could not create album %s for user %s", title, user);
        wac_err = strdup(ex.GetErrMsg());

        if (clean_paths(cfg)) {
            LOG_E("Could not clean paths after failing to create album");
        } else {
            LOG_I("Cleaned the paths after failing to create album");
        }
    }

    *path = make_r_path_to_webpage((char *)user, random_id);

    int err = remove(archive);
    if (err) {
        LOG_E("Could not delete %s: %s", archive, strerror(errno));
    }

    free(random_id);
    free(user_path);
    free_album_params(cfg);

    return wac_err;
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

