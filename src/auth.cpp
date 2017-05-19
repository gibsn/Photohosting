#include "auth.h"

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>

#include "common.h"
#include "exceptions.h"
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


void UsersList::Append(const char *_login, const char *_password)
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


void SessionsList::Append(const char *_sid, const char *_login)
{
    SessionsListNode *new_node = new SessionsListNode(_sid, _login);

    if (head == 0) {
        head = new_node;
        last = new_node;
        return;
    }

    last->next = new_node;
    last = last->next;
}


const char *SessionsList::GetUserBySession(const char *sid) const
{
    SessionsListNode *curr = head;
    while(curr) {
        int min_len = MIN(strlen(sid), strlen(head->sid));
        if (0 == strncmp(sid, curr->sid, min_len)) {
            return curr->login;
        }

        curr = curr->next;
    }

    return NULL;
}


SessionsList::~SessionsList()
{
    SessionsListNode *curr = head;
    SessionsListNode *next = head;

    while (curr) {
        next = curr->next;
        delete curr;
        curr = next;
    }
}


bool Auth::Init(const char *filepath, const char *path_to_tokens)
{
    tokens_path = strdup(path_to_tokens);

    char *login;
    char *password;
    AuthFileParser parser;

    FILE *file = fopen(filepath, "r");
    if (!file) {
        LOG_E("Could not open auth file %s", filepath);
        goto fin;
    }

    parser =  AuthFileParser(file);
    while (!parser.CheckEof()) {
        login = parser.ParseLogin();
        if (!login) {
            LOG_E("auth: error parsing login");
            goto fin;
        }

        password = parser.ParsePassword();
        if (!password) {
            LOG_E("auth: error parsing password");
            goto fin;
        }

        users_list.Append(login, password);
    }

    fclose(file);
    return true;

fin:
    LOG_E("Could not initialise auth");
    return false;
}


bool Auth::Check(const char *_login, const char *_password) const
{
    return users_list.Check(_login, _password);
}


char *Auth::NewSession(const char *user)
{
    char *sid = _gen_random_string(SID_LEN);

    char *path = (char *)malloc(strlen(tokens_path) + sizeof "/" + SID_LEN);
    memcpy(path, tokens_path, strlen(tokens_path) + 1);
    strcat(path, "/");
    strcat(path, sid);

    bool err = true;
    int n;
    int fd = open(path, O_CREAT | O_WRONLY, 0666);
    if (fd == -1) {
        goto fin;
    }

    n = write(fd, user, strlen(user));
    if (n != strlen(user)) {
        goto fin;
    }

    err = false;

fin:
    if (fd != -1) close(fd);
    if (path) free(path);

    if (err) {
        LOG_E("Could not save new session to disk: %s", strerror(errno));
        if (sid) free(sid);
        throw NewSessionEx();
    }

    return sid;
}


void Auth::DeleteSession(const char *sid)
{
    char *path = (char *)malloc(strlen(tokens_path) + sizeof "/" + SID_LEN);
    memcpy(path, tokens_path, strlen(tokens_path) + 1);
    strcat(path, "/");
    strcat(path, sid);

    if (remove(path)) {
        LOG_E("Could not delete session %s: %s", sid, strerror(errno));
        if (path) free(path);

        throw DeleteSessionEx();
    }

    if (path) free(path);
}


char *Auth::GetUserBySession(const char *sid) const
{
    char *path = (char *)malloc(strlen(tokens_path) + sizeof "/" + SID_LEN);
    memcpy(path, tokens_path, strlen(tokens_path) + 1);
    strcat(path, "/");
    strcat(path, sid);

    char user[256], *ret = NULL;
    int n;
    bool err = true;
    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        if (errno == ENOENT) {
            LOG_W("Got a request with the non-existing session id %s", sid);
            err = false;
        }

        goto fin;
    }

    n = read(fd, user, sizeof user);
    if (n == -1) {
        goto fin;
    }

    ret = strndup(user, n);
    err = false;

fin:
    if (fd != -1) close(fd);
    if (path) free(path);

    if (err) {
        LOG_E("Could not read user's session token from disk: %s", strerror(errno));
        throw GetUserBySessionEx();
    }

    return ret;
}


char *Auth::ParseLoginFromReq(const char *body, int body_len)
{
    char *body_copy = strndup(body, body_len);

    char *start = strstr(body_copy, "login=");
    if (!start) return NULL;

    start += sizeof "login=" - 2;
    char *save_start = start + 1;

    while (isalnum(*++start));

    return strndup(save_start, start - save_start);
}


char *Auth::ParsePasswordFromReq(const char *body, int body_len)
{
    char *body_copy = strndup(body, body_len);

    char *start = strstr(body_copy, "password=");
    if (!start) return NULL;

    start += sizeof "password=" - 2;
    char *save_start = start + 1;

    while (isalnum(*++start));

    return strndup(save_start, start - save_start);
}
