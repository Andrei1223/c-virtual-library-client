#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"

char *compute_get_request(char *host, char *url, char *query_params, char *JWT,
                            char **cookies, int cookies_count)
{
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));

    // write the method name, URL, request params and protocol type
    if (query_params != NULL) {
        sprintf(line, "GET %s?%s HTTP/1.1", url, query_params);
    } else {
        sprintf(line, "GET %s HTTP/1.1", url);
    }

    compute_message(message, line);

    // add the host
    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    if (JWT != NULL) {
        sprintf(line, "Authorization: Bearer %s", JWT);
        compute_message(message, line);
    }

    // add headers and/or cookies, according to the protocol format
    if (cookies != NULL) {
        sprintf(line, "Cookie: %s", cookies[0]);
        char *temp = calloc(LINELEN, sizeof(char));
        for (int i = 1; i < cookies_count; i++) {
            sprintf(temp, "; %s", cookies[i]);
            strcat(line, temp);
        }
        free(temp);
    }
    compute_message(message, line);
    free(line);

    // add final new line
    compute_message(message, "");
    return message;
}

char *compute_post_request(char *host, char *url, char* content_type, char **body_data,
                            int body_data_fields_count, char *JWT, char **cookies, int cookies_count)
{
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));
    char *body_data_buffer = calloc(LINELEN, sizeof(char));

    // write the method name, URL and protocol type
    sprintf(line, "POST %s HTTP/1.1", url);
    compute_message(message, line);

    // add the host
    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    /* add necessary headers in order to write Content-Length you must first compute the message size
    */

    if (JWT != NULL) {
        sprintf(line, "Authorization: Bearer %s", JWT);
        compute_message(message, line);
    }

    int len = 0;
    for(int i = 0; i < body_data_fields_count; i++) {
        strcat(body_data_buffer, body_data[i]);
        len += strlen(body_data[i]);
    }

    sprintf(line, "Content-Type: %s", content_type);
    compute_message(message, line);
    sprintf(line, "Content-Length: %d", len);
    compute_message(message, line);
    sprintf(line, "Connection: keep-alive");
    compute_message(message, line);
    
    // add cookies
    if (cookies != NULL) {
       sprintf(line, "Cookie: %s", cookies[0]);
        char *temp = calloc(LINELEN, sizeof(char));

        for(int i = 1; i < cookies_count; i++) {
            sprintf(temp, "; %s", cookies[i]);
            strcat(line, temp);
        }

        free(temp);
        compute_message(message, line);
    }
    // add new line at end of header
    compute_message(message, "");
    // add the actual payload data
    memset(line, 0, LINELEN);
    strcat(message, body_data_buffer);

    free(line);
    free(body_data_buffer);
    return message;
}
