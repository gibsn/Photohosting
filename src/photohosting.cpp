#include "photohosting.h"

#include <errno.h>
#include <fcntl.h>

#include "WebAlbumCreator.h"

#include "auth.h"
#include "cfg.h"
#include "common.h"
#include "exceptions.h"
#include "log.h"
#include "web_album_creator_helper.h"


Photohosting::Photohosting(Config &cfg, AuthDriver *_auth)
    : auth(_auth)
{
    path_to_store = strdup(cfg.path_to_store);
    path_to_tmp_files = strdup(cfg.path_to_tmp_files);
    relative_path_to_css = strdup(cfg.path_to_css);
}


Photohosting::~Photohosting()
{
    if (path_to_store) free(path_to_store);
    if (path_to_tmp_files) free(path_to_tmp_files);
    if (relative_path_to_css) free(relative_path_to_css);
}

void Photohosting::_CreateAlbum(const WebAlbumParams &cfg)
{
    try {
        CreateWebAlbum(cfg);
// TODO: watch for enospace
    } catch (Wac::WebAlbumCreatorEx &ex) {
        LOG_E("WebAlbumCreator: %s", ex.GetErrMsg());

        if (clean_paths(cfg)) {
            LOG_E("Could not clean paths after failing to create album");
        } else {
            LOG_I("Cleaned the paths after failing to create album");
        }

        throw PhotohostingEx();
    }
}


char *Photohosting::SaveTmpFile(ByteArray *file)
{
    char *full_path = (char *)malloc(strlen(path_to_tmp_files) + 1 + sizeof "tmpXXXXXX");
    strcpy(full_path, path_to_tmp_files);
    strcat(full_path, "/");
    strcat(full_path, "tmpXXXXXX");

    int fd = mkstemp(full_path);
    try {
        if (fd == -1) {
            LOG_E("Could not save tmp file %s", full_path);
            throw SaveFileEx(strerror(errno));
        }

        int n = write(fd, file->data, file->size);
        if (file->size != n) {
            LOG_E("Could not save tmp file %s", full_path);
            if (errno == ENOSPC) throw NoSpace();
            throw SaveFileEx(strerror(errno));
        }

        close(fd);
        return full_path;
    }
    catch (SystemEx &ex) {
        LOG_E("%s", ex.GetErrMsg());
        free(full_path);
        if (fd != -1) close(fd);
        throw;
    }
}


char *Photohosting::CreateAlbum(const char *user, const char *archive, const char *title)
{
    int random_id_len = 16;
    char *random_id = _gen_random_string(random_id_len);

    char *user_path = make_user_path(path_to_store, user);

    WebAlbumParams cfg = album_params_helper(user, user_path, random_id);
    cfg.path_to_archive = archive;
    cfg.web_page_title = title;
    cfg.path_to_css = make_path_to_css(relative_path_to_css);

    album_creator_debug(cfg);

    create_user_paths(user_path, cfg.path_to_unpack, cfg.path_to_thumbnails);
    char *path = make_r_path_to_webpage(user, random_id);

    free(random_id);
    free(user_path);

    try {
        _CreateAlbum(cfg);

        if (remove(archive)) LOG_E("Could not delete %s: %s", archive, strerror(errno));
        free_album_params(cfg);

        return path;
    } catch (PhotohostingEx &) {
        LOG_E("Could not create album %s for user %s", title, user);
        free(path);

        if (remove(archive)) LOG_E("Could not delete %s: %s", archive, strerror(errno));
        free_album_params(cfg);
        throw;
    }
}


char *Photohosting::Authorise(const char *user, const char *password)
{
    if (auth->Check(user, password)) {
        return auth->NewSession(user);
    }

    return NULL;
}


void Photohosting::Logout(const char *sid)
{
    if (*sid == '\0') return;

    return auth->DeleteSession(sid);
}


// throws GetUserBySessionEx
char *Photohosting::GetUserBySession(const char *sid) {
    if (!sid || *sid == '\0') return NULL;

    return auth->GetUserBySession(sid);
}

