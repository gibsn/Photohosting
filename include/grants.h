#ifndef GRANTS_H_SENTRY
#define GRANTS_H_SENTRY

#include <string.h>
#include <stdlib.h>

#include "common.h"


class Grants
{
    char *path_to_grants;

public:
    Grants(const char *path);
    virtual ~Grants();

    virtual void Init() const;

    char *GetPathToGrants() const { return strdup(path_to_grants); }
};


// Grants are stored using filesystem hierarchically, order
// is important. For example, grant to create albums for
// user gibsn@provider.com is stored as
// grants/domain/provider.com/user/gibsn/app/photohosting/create_album

class GrantsBuilder
{
    char *path_to_grants;

    // user is considered local if domain is not specified
    char *domain;
    char *user;


    ByteArray *BuildDirPath() const;
    ByteArray *BuildGrantPath(const char *grant) const;

    virtual char *GrantParamsToPath() const = 0;

public:
    GrantsBuilder(const char *path);
    GrantsBuilder(const char *_path, const char *_domain);
    GrantsBuilder(const char *_path, const char *_domain, const char *_user);
    virtual ~GrantsBuilder();

    void SetUser(const char *_user) {
        free(user);
        user = strdup(_user);
    }
    void ResetUser() {
        free(user);
        user = NULL;
    }
    void SetDomain(const char *_domain) {
        free(domain);
        domain = strdup(_domain);
    }
    void ResetDomain() {
        free(domain);
        domain = NULL;
    }

    bool HasGrant(const char *grant) const;
    void SetGrant(const char *grant) const;
    void RemoveGrant(const char *grant) const;
};


#endif
