#include "core/session.h"

static char current_user[32];
static int logged_in = 0;

void session_init() {
    logged_in = 0;
    current_user[0] = '\0';
}

void session_login(char* username) {
    int i = 0;
    while(username[i] && i < 31) {
        current_user[i] = username[i];
        i++;
    }
    current_user[i] = '\0';
    logged_in = 1;
}

void session_logout() {
    logged_in = 0;
    current_user[0] = '\0';
}

int session_is_logged_in() {
    return logged_in;
}

void session_get_username(char* buffer) {
    if (!logged_in) {
        buffer[0] = '\0';
        return;
    }
    int i = 0;
    while(current_user[i]) {
        buffer[i] = current_user[i];
        i++;
    }
    buffer[i] = '\0';
}
