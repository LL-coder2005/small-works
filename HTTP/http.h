#ifndef _HTTP_
#define _HTTP_



int init_http(int port);

int msg_handle(int sock);

void log_request(const char* method, const char* url, int status);

#endif