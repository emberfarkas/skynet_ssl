#include "shttp.h"
#include <string.h>

const char * shttp(char buf[4096], int cap, char *METHOD, char *path, char *content, int *size) {
	strcat(buf, "GET / HTTP/1.0\r\n");
	strcat(buf, "\r\n");
	//strcat(buf, "Connection: Keep-Alive\r\n");
	strcat(buf, "HOST: www.baidu.com\r\n");
	strcat(buf, "Content-Type: application/x-www-form-urlencoded\r\n");
	strcat(buf, "Content-Length: 0");
	//strcat(buf, strlen(content));
	strcat(buf, "\r\n");
	strcat(buf, "\r\n");
	//strcat(buf, content);
	strcat(buf, "\r\n\r\n");

	*size = strlen(buf);

	return buf;
}