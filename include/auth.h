#ifndef AUTH_H_SENTRY
#define AUTH_H_SENTRY

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "auth_driver.h"

#define SID_LEN 32


enum auth_file_parser_state_t {
    new_line,
    login,
    password,
    fin
};


class AuthFileParser {
    auth_file_parser_state_t state;
    FILE *file;
    char buf[256];
    int buf_idx;

public:
    AuthFileParser(): file(NULL), buf_idx(0) {};
    AuthFileParser(FILE *_file): state(new_line), file(_file), buf_idx(0) {}

    bool CheckEof();
    char *ParseLogin();
    char *ParsePassword();
};


struct UsersListNode {
    char *login;
    char *password;
    UsersListNode *next;

    UsersListNode(const char *_login, const char *_password): next(0) {
        login = strdup(_login);
        password = strdup(_password);
    }

    ~UsersListNode() {
        free(login);
        free(password);
    }
};


class UsersList {
    UsersListNode *head;
    UsersListNode *last;

public:
    UsersList(): head(0), last(0) {}
    ~UsersList();

    void Append(const char *_login, const char *_password);
    bool Check(const char *_login, const char *_password) const;
};


struct SessionsListNode {
    char *sid;
    char *login;
    SessionsListNode *next;

    SessionsListNode(const char *_sid, const char *_login): next(0) {
        sid = strdup(_sid);
        login = strdup(_login);
    }

    ~SessionsListNode() {
        free(sid);
        free(login);
    }
};


class SessionsList {
    SessionsListNode *head;
    SessionsListNode *last;

public:
    SessionsList(): head(0), last(0) {}
    ~SessionsList();

    void Append(const char *_sid, const char *_login);
    const char *GetUserBySession(const char *sid) const;
};


class Auth: public AuthDriver{
    UsersList users_list;

    char *tokens_path;

public:
    Auth(): tokens_path(NULL) {};
    ~Auth() { if (tokens_path) free(tokens_path); };

    void Init(const char *filepath, const char *path_to_tokens);

    bool Check(const char *_login, const char *_password) const;
    char *NewSession(const char *user);
    void DeleteSession(const char *sid);
    char *GetUserBySession(const char *sid) const;

    static char *ParseLoginFromReq(const char *body, int body_len);
    static char *ParsePasswordFromReq(const char *body, int body_len);
};

#endif
