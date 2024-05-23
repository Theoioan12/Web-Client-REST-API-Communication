#include <stdio.h>
#include <stdlib.h>     
#include <unistd.h>     
#include <string.h>    
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <netdb.h>      
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"
#include "parson.h"
#include <ctype.h>

void data_wrapper(char **data, char *message, short type) 
{
    strcat(data[type], "\"");
    strcat(data[type], message);
    strcat(data[type], "\"");
}

void send_to_server_message(char *message) 
{
    printf("Sending to server...\n");
    puts(message);
}

// function to prin the books of the current user
void print_books(char *parse) 
{
    while(parse) {
        if(*parse == '[') {
            printf("-----------\n");

            JSON_Value *type = json_parse_string(parse);
            JSON_Array *books = json_value_get_array(type);

            // getting the number of the books
            int books_number = json_array_get_count(books);

            // iterating and printing the id and the title
            // for each book
            for(int i = 0; i < books_number; i++) {
                JSON_Object *element = json_array_get_object(books, i);
                printf("ID: %d\n", (int)json_object_get_number(element, "id"));
                printf("Title: %s\n", json_object_get_string(element, "title"));
                            
                printf("-----------\n");
            }
        }
        parse = strtok(NULL, "\n");
    }
}

// main function - skel adapted
int main(int argc, char *argv[])
{
    char *message;
    char *response;
    int sockfd;

    char *cookie = calloc(BUFLEN, sizeof(char));
    char **cookies = calloc(1, sizeof(char *));
    *cookies = calloc(BUFLEN, sizeof(char));

    char *token = calloc(BUFLEN, sizeof(char));

    short logged_in = false, 
    in_library = false;
    char *command = calloc(MAX_COMMAND, sizeof(char));
    short server_on = true;

    // server command loop
    while (server_on) {
        // opening the connection
        sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);

        // scanning the next command
        scanf("%s", command);

        // checking if the opening succeded
        if (sockfd < 0) {
            printf("ERROR: Could not open connection\n");
        }
        
        // exit command
        if(strcmp(command, "exit") == 0) {
            // just closing the socket
            close_connection(sockfd);
            server_on = false;

            // simply breaking the loop
            break;
        }

        char **data = calloc(5, sizeof(char *));
        for(int i = 0; i <= 1; ++i) {
            data[i] = calloc(BUFLEN, sizeof(char));
        } 

        // register command
        if(strcmp(command, "register") == 0) {
            if (logged_in == false ) {
                char *username = calloc(BUFLEN, sizeof(char));

                // introducing the username
                printf("username=");
                scanf("%s", username);

                strcpy(data[0], "\"username\": ");
                data_wrapper(data, username, 0);
                free(username);
                
                char *password = calloc(BUFLEN, sizeof(char));
            
                // introducing the password
                printf("password=");
                scanf("%s", password);

                strcpy(data[1], "\"password\": ");
                data_wrapper(data, password, 1);
                free(password);
                
                message = compute_post_request(HOST, "/api/v1/tema/auth/register", PATH_ADDR, data, 2, NULL, 0, NULL);
                
                send_to_server_message(message);
                send_to_server(sockfd, message);

                response = receive_from_server(sockfd);
                printf("Received:\n");

                char *code = strtok(response, "\n\r");
                printf("Response: %s\n", code);

                // checking if the registration succeded
                (strncmp(response, "HTTP/1.1 201", 12) == 0) ? (printf("Succesfully registered\n")) : (printf("User already exists\n"));

                // the user is already logged in
            } else {
                printf("Already logged in\n");
            }

        // login command
        } else if(strcmp(command, "login") == 0) {
            // checking if the user is already logged
            if (logged_in == false) {
                char *username = calloc(BUFLEN, sizeof(char));
                
                // receivving the username
                printf("username=");
                scanf("%s", username);

                strcpy(data[0], "\"username\": ");
                data_wrapper(data, username, 0);
                free(username);

                char *password = calloc(BUFLEN, sizeof(char));

                // receiving the password
                printf("password=");
                scanf("%s", password);

                strcpy(data[1], "\"password\": ");
                data_wrapper(data, password, 1);
                free(password);
                
                printf("\n");
                message = compute_post_request(HOST, "/api/v1/tema/auth/login", PATH_ADDR, data, 2, NULL, 0, NULL);
                
                send_to_server_message(message);
                send_to_server(sockfd, message);

                response = receive_from_server(sockfd);
                printf("Received:\n");

                strcpy(cookie, get_cookie(response));
                *cookies = calloc(BUFLEN, sizeof(char));
                strcpy(*cookies, cookie);
                
                char *code = strtok(response, "\n\r");
                printf("Response: %s\n", code);

                // checking if the login succeded
                if (strncmp(response, "HTTP/1.1 200", 12) == 0) {
                    (printf("Welcome to the server\n"));
                    logged_in = true; 
                } else {
                    (printf("Wrong credidentials\n"));
                    logged_in = false;
                }
                
            } else {
                printf("Already logged in\n");
            }    

            // logout command
        } else if(strcmp(command, "logout") == 0) {
            if (logged_in == true) {
                printf("\n");
                message = compute_get_request(HOST, "/api/v1/tema/auth/logout", NULL, cookies, 1);
                
                send_to_server_message(message);
                send_to_server(sockfd, message);

                response = receive_from_server(sockfd);
                char *code = strtok(response, "\r\n");
                printf("Response: %s\n", code);

                // if the logout succeded
                if (strncmp(response, "HTTP/1.1 200", 12) == 0) {
                    in_library = false;
                    logged_in = false;
                    (printf("Succesfully logged out\n"));
                }
                
                memset(*cookies, 0, BUFLEN);
                memset(token, 0, BUFLEN);

                // if the logout failed
            } else {
                (printf("You are not logged in\n"));
            }
            
            // enter_library command
        } else if(strcmp(command, "enter_library") == 0) {
            // checking if the user is logged
            if (logged_in == true) {
                printf("\n");
                message = compute_get_request(HOST, "/api/v1/tema/library/access", NULL, cookies, 1);
                
                send_to_server_message(message);
                send_to_server(sockfd, message);

                response = receive_from_server(sockfd);
                printf("Received:\n");
                strcpy(token, get_token(response));
                
                char *code = strtok(response, "\n\r");
                printf("Response: %s\n", code);

                // if entering the library succeded
                if (strncmp(response, "HTTP/1.1 200", 12) == 0) {
                    (printf("Welcome to the library\n"));
                    in_library = true;
                } 

                // otherwise he must log in
            } else {
                (printf("Forbidden access, you must log in first\n"));
                in_library = false;
            }

            // add_book command
        }  else if(strcmp(command, "add_book") == 0) {
            // firstly checking if the user is logged
            if (logged_in == true) {

                // checking if the user is in the library
                if (in_library == true) {
                    for(int i = 2; i <= 4; ++i) {
                        data[i] = calloc(BUFLEN, sizeof(char));
                    } 

                    short i = 0;

                    // title
                    char *title = calloc(BUFLEN, sizeof(char));
                    printf("title=");
                    scanf("%s", title);
                    strcpy(data[i], "\"title\": ");
                    strcat(data[i], "\"");
                    strcat(data[i], title);
                    strcat(data[i], "\"");
                    free(title);

                    // author
                    i++;
                    char *author = calloc(BUFLEN, sizeof(char));
                    printf("author=");
                    scanf("%s", author);
                    strcpy(data[i], "\"author\": ");
                    strcat(data[i], "\"");
                    strcat(data[i], author); 
                    strcat(data[i], "\"");
                    free(author);

                    // genre
                    i++;
                    char *genre = calloc(BUFLEN, sizeof(char));
                    printf("genre=");
                    scanf("%s", genre);
                    strcpy(data[i], "\"genre\": ");
                    strcat(data[i], "\"");
                    strcat(data[i], genre);
                    strcat(data[i], "\"");
                    free(genre);

                    // publisher
                    i++;
                    char *publisher = calloc(BUFLEN, sizeof(char));
                    printf("publisher=");
                    scanf("%s", publisher);
                    strcpy(data[i], "\"publisher\": ");
                    strcat(data[i], "\"");
                    strcat(data[i], publisher);
                    strcat(data[i], "\"");
                    free(publisher);

                    // page_count
                    char *page_count = calloc(BUFLEN, sizeof(char));
                    printf("page_count=");
                    scanf("%s", page_count);
                    if(atoi(page_count) == 0) {
                        printf("Invalid number of pages, please retry\n");
                        continue;
                    }

                    i++;
                    strcpy(data[i], "\"page_count\": ");
                    strcat(data[i], page_count);
                    free(page_count);

                    printf("\n");
                    message = compute_post_request(HOST, "/api/v1/tema/library/books", PATH_ADDR, data, 5, NULL, 0, token);
                    send_to_server_message(message);
                    
                    send_to_server(sockfd, message);
                    response = receive_from_server(sockfd);
                    printf("Received:\n");
                    char *code = strtok(response, "\n\r");
                    printf("Response: %s\n", code);

                    // checking if the book was succesfully added
                    if(strncmp(response, "HTTP/1.1 200", 12) == 0) {
                        printf("Succesfully added a book\n");
                    }    
                    for (int j = 0; j <= 4; ++j) {
                        free(data[j]);
                    }

                    // if the user is not in the library
                } else {
                    printf("Forbidden access. You are not in the library\n");
                }

                // if the user is not logged
            } else {
                printf("You are not logged in!\n");
            }

            // get_books command
        } else if(strcmp(command, "get_books") == 0) {
            // checking if the user is in the library
            if (in_library == true) {
                printf("\n");
                message = compute_get_request(HOST, "/api/v1/tema/library/books", token, cookies, 1);

                send_to_server_message(message);
                send_to_server(sockfd, message);

                response = receive_from_server(sockfd);
                printf("Received:\n");

                char* parse;
                parse = strtok(response, "\r\n");
                printf("Response: %s\n", parse);

                if(strncmp(response, "HTTP/1.1 200", 12) == 0) {
                    printf("Succesfully received all books\n");
                }  
            
                print_books(parse);

                // if he is not in the library
            } else {
                printf("Forbidden access. You are not in the library\n");
            }

            // get_book command
        } else if(strcmp(command, "get_book") == 0) {
            // checking if the user is in the library
            if (in_library == true) {
                char *get_id = calloc(MAX_BOOK_ID, sizeof(char));

                // getting the book id
                printf("id=");
                scanf("%s", get_id);

                char *path = calloc(100, sizeof(char));
                strcpy(path, "/api/v1/tema/library/books/");
                strcat(path, get_id);

                message = compute_get_request(HOST, path, token, cookies, 1);
                free(path);

                send_to_server_message(message);
                send_to_server(sockfd, message);

                response = receive_from_server(sockfd);

                printf("Received:\n");
                printf("%s\n", response);

                char* parse;
                parse = strtok(response, "\n");
                printf("Response: %s\n", parse);

                // checking if the book actually exists
                // and the book was received
                if(strncmp(response, "HTTP/1.1 200", 12) == 0) {
                    printf("Succesfully received the book\n");

                } else if (strncmp(response, "HTTP/1.1 404", 12) == 0) {
                    printf("There is no book with this id, please try another one\n");
                } 
                free(get_id);

                // if the user is not in the library
            } else {
                printf("Forbidden access. You are not in the library\n");
            }

            // delete_book function
        } else if(strcmp(command, "delete_book") == 0) {
            if (in_library == true) {
                char *delete_id = calloc(100, sizeof(char));

                printf("id=");
                scanf("%s", delete_id);

                printf("\n");
                char *path = calloc(100, sizeof(char));
                strcpy(path, "/api/v1/tema/library/books/");
                strcat(path, delete_id);
                free(delete_id);

                message = compute_delete_request(HOST, path, token, cookies);
                free(path);
                
                send_to_server_message(message);
                send_to_server(sockfd, message);

                response = receive_from_server(sockfd);
                printf("Received:\n");
                char *code = strtok(response, "\n\r");
                printf("Response: %s\n", code);

                // checking if the book could be deleted
                if(strncmp(response, "HTTP/1.1 200", 12) == 0) {
                    printf("Succesfully deleted the book\n");
                }
                if (strncmp(response, "HTTP/1.1 404", 12) == 0) {
                    printf("There is no book with this id, please try another one\n");
                }
                
                // if the user is not in the library
            } else {
                printf("Forbidden access. You are not in the library\n");
            }

            // in case there is no matching command
        } else {
            printf("Unrecognized command\n");
        }

        close_connection(sockfd);
    }

    free(*cookies);
    free(cookies);
    free(cookie);
    free(token);
    free(command);

    return 0;
}
