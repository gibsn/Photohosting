#include "auth.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <sys/wait.h>
#include <unistd.h>

#include "common.h"
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


void AuthSlave::ServeAuth(int read_fd, int write_fd)
{
    LOG_I("Auth service has been initialised successfully");

    auth_task_t task_type;
    while (true) {
        int n = read(read_fd, &task_type, sizeof(task_type));
        assert(n == sizeof(task_type));

        switch (task_type) {
            case check:
                break;
            case new_session:
                break;
            case user_by_session:
                break;
        }

        // n = write(write_fd,,);
    }
}


bool AuthSlave::Check(const char *_login, const char *_password) const
{

    return users_list.Check(_login, _password);
}


char *AuthSlave::NewSession(const char *user)
{
    char *sid = gen_random_string(SID_LEN);

    active_sessions.Append(sid, user);

    return sid;
}


const char *AuthSlave::GetUserBySession(const char *sid) const
{
    return active_sessions.GetUserBySession(sid);
}


bool AuthSlave::ParseFile(const char *filepath)
{
    char *login;
    char *password;
    AuthFileParser parser;

    FILE *file = fopen(filepath, "r");
    if (!file) {
        LOG_E("auth: could not open file %s", filepath);
        return false;
    }

    parser = AuthFileParser(file);
    while (!parser.CheckEof()) {
        login = parser.ParseLogin();
        if (!login) {
            LOG_E("auth: error parsing login");
            return false;
        }

        password = parser.ParsePassword();
        if (!password) {
            LOG_E("auth: error parsing password");
            return false;
        }

        users_list.Append(login, password);
    }

    fclose(file);

    return true;
}


void Auth::Init(const char *filepath)
{
    int n;
    int pipe1[2], pipe2[2];
    if (pipe(pipe1) || pipe(pipe2)) {
        LOG_E("here");
        goto fin;
    }

    n = fork();
    if (n == 0) {
        if (!slave.ParseFile(filepath)) exit(-1);

        n = fork();
        if (n == 0) {
            close(pipe1[1]);
            close(pipe2[0]);

            slave.ServeAuth(pipe1[0], pipe2[1]);
        } else if (n == -1) {
            exit(-1);
        }

        exit(0);
    } else if (n == -1) {
        goto fin;
    }

    int status;
    waitpid(n, &status, 0);
    if (WEXITSTATUS(status) != 0) {
        goto fin;
    }

    write_fd = pipe1[1];
    close(pipe1[0]);

    read_fd = pipe2[0];
    close(pipe2[1]);

    return;

fin:
    LOG_E("auth: %s", strerror(errno));
    exit(-1);
}


bool Auth::Check(const char *_login, const char *_password) const
{
    return true;
}


char *Auth::NewSession(const char *user)
{
    return NULL;
}


const char *Auth::GetUserBySession(const char *sid) const
{
    return NULL;
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
