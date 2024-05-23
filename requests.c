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
#include "buffer.h"
#include "parson.h"

// function to allocate memory for message and line
void memory_allocator (char **message, char**line) 
{
    *message = calloc(BUFLEN, sizeof(char));
    *line = calloc(LINELEN, sizeof(char));
}

char *compute_get_request(char *host, char *url, char *token,
                            char **cookies, int cookies_count)
{
    char *message, *line;
    memory_allocator(&message, &line);
    
    sprintf(line, "GET %s HTTP/1.1", url);
    compute_message(message, line);

    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    if (cookies && strlen(*cookies)) {
        sprintf(line, "Cookie:");

        for (int i = 0; i < cookies_count; i++) {
            strncat(line, " ", 1);
            strcat(line, cookies[i]);
        }

        compute_message(message, line);     
    }

    if(token && strlen(token)) { 
        sprintf(line, "Authorization: Bearer %s", token);
        compute_message(message, line);
    }

    compute_message(message, "\n");
    free(line);

    return message;
}

char *compute_post_request(char *host, char *url, char* content_type, char **body_data,
                            int body_data_fields_count, char **cookies, int cookies_count, char* token)
{
    char *message, *line;
    memory_allocator(&message, &line);
    
    sprintf(line, "POST %s HTTP/1.1", url);
    compute_message(message, line);
    
    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    if(token && strlen(token)) {
        sprintf(line, "Authorization: Bearer %s", token);
        compute_message(message, line);   
    }

    char *body_data_buffer = calloc(LINELEN, sizeof(char));
    strncat(body_data_buffer, "{\n   ", 6);

    for (int i = 0; i < body_data_fields_count; i++) {
        strcat(body_data_buffer, body_data[i]);

        (i < body_data_fields_count - 1) ? ( strncat(body_data_buffer, ",\n   ", 6)) :
                                            (strncat(body_data_buffer, "\n}", 3));
    }

    sprintf(line, "Content-Type: %s", content_type);
    compute_message(message, line);

    sprintf(line, "Content-Length: %ld", strlen(body_data_buffer));
    compute_message(message, line);

    if (cookies && strlen(*cookies)) {
        strncat(line, "Cookie:", 7);

        for (int i = 0; i < cookies_count; i++) {
            sprintf(line, " %s;", cookies[i]);
            compute_message(message, line);
        }
    }
    
    memset(line, 0, LINELEN);

    compute_message(message, "");
    compute_message(message, body_data_buffer);

    free(body_data_buffer);
    free(line);

    return message;
}


char *compute_delete_request(char *host, char *url, char *token, char **cookies)
{
    char *message, *line;
    memory_allocator(&message, &line);

    sprintf(line, "DELETE %s HTTP/1.1", url);
    compute_message(message, line);

    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    if (cookies && (strlen(*cookies) > 0)) {
        sprintf(line, "Cookie:");
        strncat(line, " ", 1);
        strcat(line, *cookies);

        compute_message(message, line);    
    }

    if(token && strlen(token)) {
        sprintf(line, "Authorization: Bearer %s", token);
        compute_message(message, line);    
    }

    compute_message(message, "\n");

    return message;
}

const char* get_token(char *response) 
{
    char* token = strtok(response, "\r\n");

    while(token) {
        if(*token == '{') {
            JSON_Value *tip = json_parse_string(token);
            JSON_Object *element = json_value_get_object(tip);

            if(json_object_get_string(element, "token")) {
                printf("Token: %s\n\n", json_object_get_string(element, "token"));

                return json_object_get_string(element, "token");
            }
        }
        token = strtok(NULL,"\r\n");
    }

    return "Error in extracting the token\n";
}

char* get_cookie(char *response)
{
    char* parse = strtok(response, "\r\n");

    while(parse) {   
        if(!strncmp(parse, "Set", 3)) {
            char* cookie = strtok(parse, " ");

            while(cookie) {
                if(*cookie == 'c') {
                    printf("Cookie: %s\n", cookie);
                    return cookie;
                } else {
                    cookie = strtok (NULL, " ;");
                }
            }
        }

        parse = strtok(NULL,"\r\n");
    }
    
    return "Error in getting the cookie\n";
}



