#include "auth.h"

#include <ctype.h>

#include "log.h"


bool AuthFileParser::CheckEof()
{
    int c = fgetc(file);
    if (c == EOF) return true;

    ungetc(c, file);
    state = login;

    return false;
}


char *AuthFileParser::ParseLogin()
{
    if (state != login) return NULL;

    int c = fgetc(file);
    while (isalnum(c)) {
        buf[buf_idx] = c;
        buf_idx++;
        c = fgetc(file);
    }

    char *ret = NULL;
    switch(c) {
    case ' ':
        state = password;
        ret = strndup(buf, buf_idx);
        break;
    default:
        state = new_line;
        break;
    }

    buf_idx = 0;
    return ret;
}


char *AuthFileParser::ParsePassword()
{
    if (state != password) return NULL;

    int c = fgetc(file);
    while (isalnum(c)) {
        buf[buf_idx] = c;
        buf_idx++;
        c = fgetc(file);
    }

    char *ret = NULL;
    switch(c) {
    case '\n':
        ret = strndup(buf, buf_idx);
        break;
    default:
        break;
    }

    state = new_line;
    buf_idx = 0;
    return ret;
}


UsersList::~UsersList()
{
    UsersListNode *curr = head;
    UsersListNode *next = head;

    while (curr) {
        next = curr->next;
        delete curr;
        curr = next;
    }
}


void UsersList::Append(char *_login, char *_password)
{
    UsersListNode *new_node = new UsersListNode(_login, _password);

    if (head == 0) {
        head = new_node;
        last = new_node;
        return;
    }

    last->next = new_node;
    last = last->next;
}


bool UsersList::Check(const char *_login, const char *_password) const
{
    UsersListNode *curr = head;
    while (curr) {
        if (0 == strcmp(_login, curr->login) &&
            0 == strcmp(_password, curr->password)
        ) {
            return true;
        }

        curr = curr->next;
    }

    return false;
}


bool Auth::Init(const char *filepath)
{
    file = fopen(filepath, "r");
    if (!file) {
        LOG_E("Could not open auth file %s", filepath);
        return false;
    }

    AuthFileParser parser(file);
    while (!parser.CheckEof()) {
        char *login = parser.ParseLogin();
        if (!login) {
            LOG_E("auth: error parsing login");
            return false;
        }

        char *password = parser.ParsePassword();
        if (!password) {
            LOG_E("auth: error parsing password");
            return false;
        }

        users_list.Append(login, password);
    }

    fclose(file);
    LOG_I("Initialised auth");

    return true;
}


bool Auth::Check(const char *_login, const char *_password) const
{
    return users_list.Check(_login, _password);
}



