#include "auth.h"

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "common.h"
#include "exceptions.h"
#include "log.h"


const int Auth::SidLen = 32;


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

    if (!head) {
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

    if (!head) {
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

    if (!file_exists(path_to_tokens)) {
        LOG_I("auth: could not find path_to_tokens, creating %s", path_to_tokens);

        if (mkdir(path_to_tokens, 0777)) {
            LOG_E("auth: could not mkdir %s: %s", path_to_tokens, strerror(errno));
            return false;
        }
    }

    char *login;
    char *password;
    AuthFileParser parser;

    FILE *file = fopen(filepath, "r");
    if (!file) {
        LOG_W("auth: could not open users file %s, starting without auth", filepath);
        return true;
    }

    parser = AuthFileParser(file);
    while (!parser.CheckEof()) {
        login = parser.ParseLogin();
        if (!login) {
            LOG_E("auth: error parsing login");
            goto err;
        }

        password = parser.ParsePassword();
        if (!password) {
            LOG_E("auth: error parsing password");
            goto err;
        }

        users_list.Append(login, password);

        free(login);
        free(password);
    }

    fclose(file);

    return true;

err:
    LOG_E("auth: could not initialise");
    if (file) fclose(file);

    return false;
}


bool Auth::Check(const char *_login, const char *_password) const
{
    return users_list.Check(_login, _password);
}


char *Auth::NewSession(const char *user)
{
    char *sid = _gen_random_string(SidLen);

    char *path = (char *)malloc(strlen(tokens_path) + sizeof "/" + SidLen);
    memcpy(path, tokens_path, strlen(tokens_path) + 1);
    strcat(path, "/");
    strcat(path, sid);

    int fd = open(path, O_CREAT | O_WRONLY, 0666);
    free(path);

    try {
        if (fd == -1) {
            throw NewSessionEx(strerror(errno));
        }

        int n = write(fd, user, strlen(user));
        if (n != strlen(user)) {
            throw NewSessionEx(strerror(errno));
        }

        close(fd);

        return sid;
    } catch (const NewSessionEx &ex) {
        LOG_E("auth: could not save new session to disk: %s", ex.GetErrMsg());

        if (fd != -1) close(fd);
        free(sid);

        throw;
    }
}


void Auth::DeleteSession(const char *sid)
{
    char *path = (char *)malloc(strlen(tokens_path) + sizeof "/" + SidLen);
    memcpy(path, tokens_path, strlen(tokens_path) + 1);
    strcat(path, "/");
    strcat(path, sid);

    if (remove(path)) {
        LOG_E("auth: could not delete session %s", sid);
        free(path);

        throw DeleteSessionEx(strerror(errno));
    }

    free(path);
}


char *Auth::GetUserBySession(const char *sid) const
{
    char *path = (char *)malloc(strlen(tokens_path) + sizeof "/" + SidLen);
    memcpy(path, tokens_path, strlen(tokens_path) + 1);
    strcat(path, "/");
    strcat(path, sid);

    int fd = open(path, O_RDONLY);
    free(path);

    try {
        if (fd == -1) {
            if (errno == ENOENT) {
                LOG_W("auth: got a request with the non-existing session id %s", sid);
                return NULL;
            }

            throw GetUserBySessionEx(strerror(errno));
        }

        char user[256];
        int n = read(fd, user, sizeof user);
        if (n == -1) {
            throw GetUserBySessionEx(strerror(errno));
        }

        close(fd);

        return strndup(user, n);
    } catch (const GetUserBySessionEx &ex) {
        LOG_E("auth: could not read user's session token from disk: %s", ex.GetErrMsg());
        if (fd != -1) close(fd);

        throw;
    }
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
