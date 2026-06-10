#include "photohosting.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "WebAlbumCreator.h"

#include "auth.h"
#include "cfg.h"
#include "common.h"
#include "exceptions.h"
#include "log.h"
#include "web_album_creator_helper.h"


PhotohostingGrants::PhotohostingGrants(const char *_path_to_grants)
    : Grants(_path_to_grants)
{
}

PhotohostingGrants::~PhotohostingGrants()
{
}

void PhotohostingGrants::Init() const
{
    Grants::Init();
}

PhotohostingGrantsBuilder *PhotohostingGrants::Builder()
{
    return new PhotohostingGrantsBuilder(GetPathToGrants());
}

PhotohostingGrantsBuilder::PhotohostingGrantsBuilder(const char *path_to_grants)
    : GrantsBuilder(path_to_grants),
    album(NULL),
    photo(NULL)
{
}

PhotohostingGrantsBuilder::~PhotohostingGrantsBuilder()
{
    free(album);
    free(photo);
}

char *PhotohostingGrantsBuilder::GrantParamsToPath() const
{
    ByteArray buf;
    buf.Append("photohosting");

    if (album) {
        buf.Append("/album/");
        buf.Append(album);
    }

    if (photo) {
        buf.Append("/photo/");
        buf.Append(photo);
    }

    return buf.GetString();
}

Photohosting::Photohosting(Config &cfg, AuthDriver *_auth)
    : auth(_auth)
{
    path_to_store = strdup(cfg.path_to_store);

    path_to_users = (char *)malloc(strlen(cfg.path_to_store) + sizeof "/users/");
    strcpy(path_to_users, path_to_store);
    strcat(path_to_users, "/users/");

    path_to_tmp_files = strdup(cfg.path_to_tmp_files);
    relative_path_to_css = strdup(cfg.path_to_css);

    grants = new PhotohostingGrants(cfg.path_to_grants);
}


Photohosting::~Photohosting()
{
    free(path_to_store);
    free(path_to_users);
    free(path_to_tmp_files);
    free(relative_path_to_css);

    delete grants;
}

bool Photohosting::Init(const Config &cfg)
{
    if (!file_exists(cfg.path_to_store)) {
        LOG_I("photohosting: could not find path_to_store, creating %s", cfg.path_to_store);

        if (mkdir(cfg.path_to_store, 0777)) {
            LOG_E("photohosting: could not mkdir %s: %s", cfg.path_to_store, strerror(errno));
            return false;
        }
    }

    if (!file_exists(cfg.path_to_tmp_files)) {
        LOG_I("photohosting: could not find path_to_tmp_files, creating %s",
                cfg.path_to_tmp_files);

        if (mkdir(cfg.path_to_tmp_files, 0777)) {
            LOG_E("photohosting: could not mkdir %s: %s", cfg.path_to_tmp_files, strerror(errno));
            return false;
        }
    }

    grants->Init();

    return true;
}

void Photohosting::_CreateAlbum(const WebAlbumParams &cfg)
{
    try {
        try {
            CreateWebAlbum(cfg);
        } catch (Wac::Exception &ex) {
            LOG_E("webalbumcreator: %s", ex.GetErrMsg());

            if (clean_paths(cfg)) {
                LOG_E("photohosting: could not clean paths after failing to create album");
            } else {
                LOG_I("photohosting: cleaned the paths after failing to create album");
            }

            throw;
        }
    } catch (Wac::NoSpace &ex) {
        throw NoSpace("failed to create album due to the lack of space");
    } catch (Wac::LibArchiveEx &ex) {
        throw BadArchive(ex.GetErrMsg());
    } catch (Wac::CorruptedImage &ex) {
        throw BadImage(ex.GetErrMsg());
    } catch (Wac::Exception &ex) {
        throw PhotohostingEx(ex.GetErrMsg());
    }
}


char *Photohosting::GenerateTmpFilePathTemplate()
{
    char *full_path = (char *)malloc(strlen(path_to_tmp_files) + 1 + sizeof "tmpXXXXXX");
    strcpy(full_path, path_to_tmp_files);
    strcat(full_path, "/");
    strcat(full_path, "tmpXXXXXX");

    return full_path;
}


char *Photohosting::SaveTmpFile(ByteArray *file)
{
    char *new_file_path = GenerateTmpFilePathTemplate();

    int fd = mkstemp(new_file_path);
    try {
        if (fd == -1) {
            LOG_E("photohosting: could not save tmp file %s: %s", new_file_path, strerror(errno));
            throw SaveFileEx(strerror(errno));
        }

        int n = write(fd, file->data, file->size);
        if (file->size != n) {
            LOG_E("photohosting: could not save tmp file %s: %s", new_file_path, strerror(errno));
            if (errno == ENOSPC) throw NoSpace(strerror(errno));
            throw SaveFileEx(strerror(errno));
        }

        close(fd);

        return new_file_path;
    } catch (SystemEx &ex) {
        free(new_file_path);
        if (fd != -1) close(fd);

        throw;
    }
}


char *Photohosting::CreateAlbum(const char *user, const char *archive, const char *title)
{
    int random_id_len = 16;
    char *random_id = _gen_random_string(random_id_len);

    char *user_path = make_user_path(path_to_users, user);

    WebAlbumParams cfg = album_params_helper(user, user_path, random_id);
    cfg.path_to_archive = archive;
    cfg.web_page_title = title;
    cfg.path_to_css = make_path_to_css(relative_path_to_css);

    create_user_paths(cfg.path_to_unpack, cfg.path_to_thumbnails);
    char *path = make_r_path_to_webpage(user, random_id);

    free(random_id);
    free(user_path);

    try {
        _CreateAlbum(cfg);

        if (remove(archive)) {
            LOG_E("photohosting: could not delete %s: %s", archive, strerror(errno));
        }

        free_album_params(cfg);

        return path;
    } catch (const Exception &ex) {
        LOG_E("photohosting: could not create album %s for user %s", title, user);

        if (remove(archive)) {
            LOG_E("photohosting: could not delete %s: %s", archive, strerror(errno));
        }

        free(path);
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


bool Photohosting::UserExists(const char *user) const
{
    char *path_to_user = (char *)malloc(strlen(path_to_users) + strlen(user));
    strcpy(path_to_user, path_to_users);
    strcat(path_to_user, user);

    bool ret = file_exists(path_to_user);
    free(path_to_user);

    return ret;
}


char *Photohosting::GetUserDir(const char *user) const
{
    char *path_to_user = (char *)malloc(strlen(path_to_users) + strlen(user));
    strcpy(path_to_user, path_to_users);
    strcat(path_to_user, user);

    return path_to_user;
}
