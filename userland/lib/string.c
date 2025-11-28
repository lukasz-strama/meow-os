#include "string.h"

size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len])
        len++;
    return len;
}

void* memcpy(void* dest, const void* src, size_t n) {
    char* d = (char*)dest;
    const char* s = (const char*)src;
    for (size_t i = 0; i < n; i++)
        d[i] = s[i];
    return dest;
}

void* memset(void* s, int c, size_t n) {
    char* p = (char*)s;
    for (size_t i = 0; i < n; i++)
        p[i] = (char)c;
    return s;
}

char* strcpy(char* dest, const char* src) {
    char* d = dest;
    while ((*d++ = *src++));
    return dest;
}

int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

int strncmp(const char* s1, const char* s2, size_t n) {
    while (n > 0 && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0) return 0;
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

int atoi(const char* str) {
    int res = 0;
    int sign = 1;
    int i = 0;

    if (str[0] == '-') {
        sign = -1;
        i++;
    }

    for (; str[i] != '\0'; ++i) {
        if (str[i] >= '0' && str[i] <= '9') {
            res = res * 10 + str[i] - '0';
        } else {
            break;
        }
    }
    return sign * res;
}

char* strtok(char* str, const char* delim) {
    static char* next_token = 0;
    if (str) next_token = str;
    if (!next_token) return 0;

    // Skip leading delimiters
    while (*next_token) {
        int is_delim = 0;
        for (const char* d = delim; *d; d++) {
            if (*next_token == *d) {
                is_delim = 1;
                break;
            }
        }
        if (!is_delim) break;
        next_token++;
    }

    if (!*next_token) return 0;

    char* start = next_token;

    // Find end of token
    while (*next_token) {
        int is_delim = 0;
        for (const char* d = delim; *d; d++) {
            if (*next_token == *d) {
                is_delim = 1;
                break;
            }
        }
        if (is_delim) {
            *next_token = '\0';
            next_token++;
            return start;
        }
        next_token++;
    }

    return start;
}

char* strcat(char* dest, const char* src) {
    char* ptr = dest + strlen(dest);
    while (*src) {
        *ptr++ = *src++;
    }
    *ptr = '\0';
    return dest;
}
