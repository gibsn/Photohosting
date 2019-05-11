#include "grants.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "exceptions.h"
#include "log.h"


Grants::Grants(const char *path)
    : path_to_grants(NULL)
{
    path_to_grants = strdup(path);
}

Grants::~Grants()
{
    free(path_to_grants);
}

void Grants::Init() const
{
    if (!access(path_to_grants, R_OK | W_OK | X_OK)) {
        return;
    }

    if (errno == ENOENT) {
        if (mkdir(path_to_grants, 0777)) {
            throw InitEx(strerror(errno));
        }

        LOG_I("grants: created directory %s", path_to_grants);

        return;
    }

    throw InitEx(strerror(errno));
}

GrantsBuilder::GrantsBuilder(const char *_path)
    : path_to_grants(NULL),
    domain(NULL),
    user(NULL)
{
    path_to_grants = strdup(_path);
    domain = strdup("localhost");
}

GrantsBuilder::GrantsBuilder(const char *_path, const char *_domain)
    : path_to_grants(NULL),
    domain(NULL),
    user(NULL)
{
    path_to_grants = strdup(_path);
    domain = strdup(_domain);
}

GrantsBuilder::GrantsBuilder(const char *_path, const char *_domain, const char *_user)
    : path_to_grants(NULL),
    domain(NULL),
    user(NULL)
{
    path_to_grants = strdup(_path);
    domain = strdup(_domain);
    user = strdup(_user);
}

GrantsBuilder::~GrantsBuilder()
{
    free(path_to_grants);
    free(domain);
    free(user);
}

ByteArray *GrantsBuilder::BuildDirPath() const
{
    if ((!domain) && (!user)) {
        throw GrantEx("neither domain nor user has been specified");
    }

    char *params_path_part = GrantParamsToPath();
    if (!params_path_part) {
        throw GrantEx("empty app-specific path");
    }

    ByteArray *buf = new ByteArray(path_to_grants);

    buf->Append("/domain/");
    if (domain) {
        buf->Append(domain);
    } else {
        buf->Append("localhost");
    }

    if (user) {
        buf->Append("/user/");
        buf->Append(user);
    }

    buf->Append("/app/");
    buf->Append(params_path_part);

    free(params_path_part);

    return buf;
}

ByteArray *GrantsBuilder::BuildGrantPath(const char *grant) const
{
    ByteArray *buf = BuildDirPath();
    buf->Append("/");
    buf->Append(grant);

    return buf;
}

bool GrantsBuilder::HasGrant(const char *grant) const
{
    ByteArray *grant_path = BuildGrantPath(grant);
    char *grant_path_s = grant_path->GetString();
    bool ret = true;

    try {
        if (access(grant_path_s, F_OK)) {
            if (errno == ENOENT) {
                ret = false;
                goto fin;
            }

            throw StatEx(strerror(errno));
        }

fin:
        delete(grant_path);
        free(grant_path_s);
        return ret;

    } catch (const Exception &) {
        delete(grant_path);
        free(grant_path_s);
        throw;
    }
}

void GrantsBuilder::SetGrant(const char *grant) const
{
    if (HasGrant(grant)) {
        return;
    }

    ByteArray *buf = BuildDirPath();
    char *dir_path_s = buf->GetString();

    buf->Append("/");
    buf->Append(grant);
    char *grant_path_s = buf->GetString();

    try {
        if (access(dir_path_s, R_OK | W_OK | X_OK)) {
            if (errno != ENOENT) {
                throw GrantEx(strerror(errno));
            }
            if (mkdir_p(dir_path_s, 0777)) {
                throw GrantEx(strerror(errno));
            }
        }

        if (-1 == creat(grant_path_s, 0666)) {
            throw CreatEx(strerror(errno));
        }

        delete buf;
        free(grant_path_s);
    } catch (const Exception &) {
        delete buf;
        free(grant_path_s);
        throw;
    }
}

void GrantsBuilder::RemoveGrant(const char *grant) const
{
    if (!HasGrant(grant)) {
        return;
    }

    ByteArray *buf = BuildGrantPath(grant);
    char *grant_path_s = buf->GetString();

    try {
        if (unlink(grant_path_s)) {
            throw RmEx(strerror(errno));
        }

        delete buf;
        free(grant_path_s);
    } catch (const Exception &) {
        delete buf;
        free(grant_path_s);
        throw;
    }
}
