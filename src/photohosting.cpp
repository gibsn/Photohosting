#include "photohosting.h"

#include <errno.h>

#include "WebAlbumCreator.h"

#include "auth.h"
#include "common.h"
#include "exceptions.h"
#include "log.h"
#include "web_album_creator_helper.h"


Photohosting::Photohosting(
    const char *_path_to_static,
    const char *_relative_path_to_css,
    AuthDriver *_auth)
    : auth(_auth)
{
    path_to_static = strdup(_path_to_static);
    relative_path_to_css = strdup(_relative_path_to_css);
}


Photohosting::~Photohosting()
{
    if (path_to_static) free(path_to_static);
    if (relative_path_to_css) free(relative_path_to_css);
}

void Photohosting::_CreateAlbum(const WebAlbumParams &cfg)
{
    try {
        CreateWebAlbum(cfg);
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

char *Photohosting::CreateAlbum(const char *user, const char *archive, const char *title)
{
    int random_id_len = 16;
    char *random_id = gen_random_string(random_id_len);

    char *user_path = make_user_path(path_to_static, user);

    WebAlbumParams cfg = album_params_helper(user, user_path, random_id);
    cfg.path_to_archive = archive;
    cfg.web_page_title = title;
    cfg.path_to_css = make_path_to_css(relative_path_to_css);

    album_creator_debug(cfg);

    create_user_paths(user_path, cfg.path_to_unpack, cfg.path_to_thumbnails);
    char *path = make_r_path_to_webpage(user, random_id);

    free(random_id);
    free(user_path);

    bool err = false;
    try {
        _CreateAlbum(cfg);
    } catch (PhotohostingEx &) {
        LOG_E("Could not create album %s for user %s", title, user);
        free(path);
        err = true;
    }

    if (remove(archive)) LOG_E("Could not delete %s: %s", archive, strerror(errno));
    free_album_params(cfg);

    if (err) throw PhotohostingEx();

    return path;
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
