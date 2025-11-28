#ifndef SESSION_H
#define SESSION_H

void session_init();
void session_login(char* username);
void session_logout();
int session_is_logged_in();
void session_get_username(char* buffer);

#endif
