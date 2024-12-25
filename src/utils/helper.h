#ifndef HELPER_H
#define HELPER_H

void clear_line();
void send_response(int socket, int code, const char *message);

#endif  // HELPER_H