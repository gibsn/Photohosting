#ifndef AUTH_H_SENTRY
#define AUTH_H_SENTRY

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


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
    AuthFileParser(FILE *_file): state(new_line), file(_file), buf_idx(0) {}

    bool CheckEof();
    char *ParseLogin();
    char *ParsePassword();
};


struct UsersListNode {
    char *login;
    char *password;
    UsersListNode *next;

    UsersListNode(char *_login, char *_password): next(0) {
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

    void Append(char *_login, char *_password);
    bool Check(const char *_login, const char *_password) const;
};


class Auth {
    UsersList users_list;
    FILE *file;

public:
    Auth(): file(0) {};
    ~Auth() {};

    bool Init(const char *filepath);
    bool Check(const char *_login, const char *_password) const;
};

#endif
