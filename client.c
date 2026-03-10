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

#define PORT 8080
#define SERVER_ADDR "34.246.184.49"

#define PATH "/api/v1/tema"
#define BUFF_MAX_NUM_LINES 10
#define MAX_NUM_COOKIES 3
#define MAX_COOKIE_LENGTH 4096

char **buffer;
char **cookies;

/*
 * function that checks that the input for adding a new book
 * is valid. (the page count is a number)
 */
int check(char title[100], char author[100], char genre[100], 
            char page_count[100], char publisher[100])
{

    /* check if the page count is a number */
    char *ptr = page_count;
    while (*ptr != '\0') {

        if (*ptr > '9' || *ptr < '0')
            return 1;
        ptr++;
    }

    return 0;
}

/*
 * function that retuns 0 if in a string there are no digits
 */
int has_digits(char *ptr)
{
    char *str = ptr;
    while (*str) {
        /* check if the current character is a digit */
        if ('0' <= *str && *str <= '9') {
            return 1;
        }
        str++;
    }
    return 0;
}

/* 
 * function that extracts the status code of a HTTP response
 */
int get_status(char *response)
{
    char number[4];
    number[0] = *(response + 9);
    number[1] = *(response + 10);
    number[2] = *(response + 11);
    number[3] = '\0';

    /* the code is always after the HTTP string */
    return atoi(number);
}


/*
 * function that creates a HTTP request based on the
 * input from the user. 
 */
void register_user(int socket)
{
    char username[101], password[101];
    char complete_path[100];
    char *message, *response;

    /* make the complete path */
    strcpy(complete_path, PATH);
    strcpy(complete_path + strlen(PATH), "/auth/register\0");

    /* read the password and the username */
    printf("username=");
    fgets(username, 100, stdin);
    username[strlen(username) - 1] = '\0';

    printf("password=");
    fgets(password, 100, stdin);
    password[strlen(password) - 1] = '\0';

    /* check if the username has spaces or numbers */
    if (strchr(username, ' ') != NULL || has_digits(username) != 0) {
        printf("ERROR: Username invalid!\n");
        return;
    }

    /* create the JSON body */
    sprintf(buffer[0], "{\n"
            "\"username\": \"%s\",\n"
            "\"password\": \"%s\"\n"
            "}\n", username, password);

    message = compute_post_request(PATH, complete_path, "application/json", buffer, 1, NULL, NULL, 0);
    send_to_server(socket, message);

    /* wait for the server response */
    response = receive_from_server(socket);

    /* check for errors */
    if (get_status(response) >= 300) {
        printf("ERROR: Utilizatorul deja exista!\n");
    } else {
        printf("SUCCESS: Utilizator inregistrat cu succes!\n");
    }
}


void login_user(int socket)
{
    char username[101], password[101];
    char complete_path[100];
    char *message, *response;

    /* make the complete path */
    strcpy(complete_path, PATH);
    strcpy(complete_path + strlen(PATH), "/auth/login\0");

    /* read the password and the username */
    printf("username=");
    fgets(username, 100, stdin);
    username[strlen(username) - 1] = '\0';

    printf("password=");
    fgets(password, 100, stdin);
    password[strlen(password) - 1] = '\0';

    /* create the JSON body */
    sprintf(buffer[0], "{\n"
            "\"username\": \"%s\",\n"
            "\"password\": \"%s\"\n"
            "}\n", username, password);

    message = compute_post_request(PATH, complete_path, "application/json", buffer, 1, NULL, NULL, 0);
    send_to_server(socket, message);

    /* wait for the server response */
    response = receive_from_server(socket);

    /* check for errors */
    if (get_status(response) >= 300) {
        printf("ERROR: Credentiale invalide!\n");
        return;
    } else {
        printf("SUCCESS: Utilizator logat cu succes!\n");
    }

    /* get the cookie */
    char *cookie_line = strstr(response, "Set-Cookie: ");

    if (cookie_line == NULL) {
        printf("ERROR: No cookie found!\n");
        return;
    }

    cookie_line += 12;
    char *cookie_end = strchr(cookie_line, ';');
    strncpy(cookies[0], cookie_line, cookie_end - cookie_line);
    cookies[0][cookie_end - cookie_line] = '\0';
}

void access_lib(int socket)
{
    char complete_path[100];
    char *message, *response;

    /* make the complete path */
    strcpy(complete_path, PATH);
    strcpy(complete_path + strlen(PATH), "/library/access\0");

    message = compute_get_request(PATH, complete_path, NULL, NULL, cookies, 1);
    send_to_server(socket, message);

    /* wait for the server response */
    response = receive_from_server(socket);

    if (get_status(response) >= 300) {
        printf("ERROR: Utilizatorul nu are access la biblioteca!\n");
        return;
    } else {
        printf("SUCCESS: Utilizatorul are acces la biblioteca!\n");
    }

    /* get the cookie from the body */
    char *cookie_line = strstr(response, "token");

    if (cookie_line == NULL) {
        printf("ERROR: No cookie found!\n");
        return;
    }

    cookie_line += 8;
    char *cookie_end = strchr(cookie_line, '}') - 1;

    strncpy(cookies[1], cookie_line, cookie_end - cookie_line);

    cookies[1][cookie_end - cookie_line] = '\0';
}

void get_books(int socket)
{
    char complete_path[100];
    char *message, *response;

    /* make the complete path */
    strcpy(complete_path, PATH);
    strcpy(complete_path + strlen(PATH), "/library/books\0");

    message = compute_get_request(PATH, complete_path, NULL, cookies[1] , NULL, 0);
    send_to_server(socket, message);

    /* wait for the server response */
    response = receive_from_server(socket);

    if (get_status(response) >= 300) {
        printf("ERROR: Utilizatorul nu are access la biblioteca!\n");
        return;
    }

    /* get and print the books */
    char *start = strchr(response, '[');
    char *end = strchr(response, ']');

    printf("%.*s\n", (int)(end - start + 1), start);
}

void get_book(int socket)
{
    char complete_path[100], str_id[10];
    char *message, *response;
    int id, aux, index = 0;

    /* make the complete path */
    strcpy(complete_path, PATH);
    strcpy(complete_path + strlen(PATH), "/library/books/\0");

    /* read the book id */
    printf("id=");
    scanf("%d", &id);

    /* get the number of digits */
    aux = id;
    while (aux > 0) {
        index++;
        aux /= 10;
    }
    str_id[index] = '\0';

    /* convert the id into a string */
    aux = id;
    while (aux > 0) {
        str_id[--index] = (aux % 10) + 48;
        aux /= 10;
    }

    /* add the book id to the path */
    strcpy(complete_path + strlen(complete_path), str_id);

    message = compute_get_request(PATH, complete_path, NULL, cookies[1] , NULL, 0);
    send_to_server(socket, message);

    /* wait for the server response */
    response = receive_from_server(socket);

    if (get_status(response) == 403 || get_status(response) == 500) {
        printf("ERROR: Userul nu este autorizat!\n");
        return;
    } else if (get_status(response) >= 300) {
        printf("ERROR: Nicio carte nu exista cu id-ul: %d!\n", id);
        return;
    }

    /* get and print the books */
    char *start = strchr(response, '{');
    char *end = strchr(response, '}');

    printf("%.*s\n", (int)(end - start + 1), start);
}

void add_book(int socket)
{
    char complete_path[100], str_page_count[100];
    char *message, *response;
    int page_count, aux, index = 0;
    char title[100], author[100], genre[100], publisher[100];

    /* make the complete path */
    strcpy(complete_path, PATH);
    strcpy(complete_path + strlen(PATH), "/library/books\0");

    printf("title=");
    fgets(title, 100, stdin);
    title[strlen(title) - 1] = '\0';

    printf("author=");
    fgets(author, 100, stdin);
    author[strlen(author) - 1] = '\0';

    printf("genre=");
    fgets(genre, 100, stdin);
    genre[strlen(genre) - 1] = '\0';

    /* read the book id */
    printf("page_count=");
    fgets(str_page_count, 100, stdin);
    str_page_count[strlen(str_page_count) - 1] = '\0';

    printf("publisher=");
    fgets(publisher, 100, stdin);
    publisher[strlen(publisher) - 1] = '\0';

    /* check that the input is correct */
    if (check(title, author, genre, str_page_count, publisher) == 1) {
        printf("ERROR: Inputul nu respecta formatul!\n");
        return;
    }

    /* create the JSON body */
    sprintf(buffer[0], "{\n"
            "\"title\": \"%s\",\n"
            "\"author\": \"%s\",\n"
            "\"genre\": \"%s\",\n"
            "\"page_count\": \"%s\",\n"
            "\"publisher\": \"%s\"\n"
            "}\n", title, author, genre, str_page_count, publisher);

    message = compute_post_request(SERVER_ADDR, complete_path, "application/json",
                                    buffer, 1, cookies[1], NULL, 0);
    send_to_server(socket, message);

    /* wait for the server response */
    response = receive_from_server(socket);

    if (get_status(response) == 403) {
        printf("ERROR: Userul nu este autorizat!\n");
        return;
    } else if (get_status(response) >= 300) {
        //printf("ERROR: Nicio carte nu exista cu id-ul: %d!\n", id);
        return;
    }

    /* get and print the books */
    char *start = strchr(response, '{');
    char *end = strchr(response, '}');

    printf("%.*s\n", (int)(end - start + 1), start);
}

void delete(int socket)
{
    char complete_path[100], str_id[10], m[1000];
    char *response, *ptr;

    /* make the complete path */
    strcpy(complete_path, PATH);
    strcpy(complete_path + strlen(PATH), "/library/books/\0");

    /* read the book id */
    printf("id=");
    scanf("%s", str_id);

    /* check that the user introduced a valid number */
    ptr = str_id;
    while (*ptr != '\0') {
        if (*ptr > '9' || *ptr < '0') {
            printf("ERROR: Nu este un id valid!\n");
            return;
        }
        ptr++;
    }

    /* add the book id to the path */
    strcpy(complete_path + strlen(complete_path), str_id);

    /* create the DELETE HTTP request */
    char line[300];
    line[0] = 0;
    m[0] = 0;

    /* make the header */
    sprintf(line, "DELETE %s HTTP/1.1", complete_path);
    compute_message(m, line);
    sprintf(line, "Host: %s", SERVER_ADDR);
    compute_message(m, line);
    sprintf(line, "Authorization: Bearer %s", cookies[1]);
    compute_message(m, line);
    compute_message(m, "");

    send_to_server(socket, m);

    /* wait for the server response */
    response = receive_from_server(socket);

    if (get_status(response) == 403) {
        printf("ERROR: Userul nu este autorizat!\n");
        return;
    } else if (get_status(response) >= 300) {
        printf("ERROR: Nicio carte nu exista cu id-ul: %s!\n", str_id);
        return;
    }

    printf("Cartea cu id %s a fost stearsa cu succes!\n", str_id);
}

void logout(int socket)
{
    char complete_path[100];
    char *message, *response;

    /* make the complete path */
    strcpy(complete_path, PATH);
    strcpy(complete_path + strlen(PATH), "/auth/logout\0");

    message = compute_get_request(PATH, complete_path, NULL, NULL , cookies, 1);
    send_to_server(socket, message);

    /* wait for the server response */
    response = receive_from_server(socket);

    if (get_status(response) >= 300) {
        printf("ERROR: Utilizatorul nu este logat!\n");
        return;
    }

    /* reset the cookies */
    cookies[0][0] = 0;
    cookies[1][0] = 0; 

    printf("Utilizatorul a fost delogat cu succes!\n");
}

/* function that gets the input from the user*/
void parse_input()
{
    char buffer[52];
    int socket;

    /* the main loop of the client */
    while(1) {
        fgets(buffer, 50, stdin);
        /* remove the \n at the end */
        buffer[strlen(buffer) - 1] = '\0';

        /* if exit close the client */
        if (strcmp(buffer, "exit") == 0) {
            break;
        }

        /* for each request open and close a connection */
        socket = open_connection(SERVER_ADDR, PORT, AF_INET, SOCK_STREAM, 0);

        /* all the possible input from the stdin */
        if (strcmp(buffer, "register") == 0)
            register_user(socket);
        else if (strcmp(buffer, "login") == 0)
            login_user(socket);
        else if (strcmp(buffer, "enter_library") == 0)
            access_lib(socket);
        else if (strcmp(buffer, "get_books") == 0)
            get_books(socket);
        else if (strcmp(buffer, "get_book") == 0)
            get_book(socket);
        else if (strcmp(buffer, "add_book") == 0)
            add_book(socket);
        else if (strcmp(buffer, "delete_book") == 0)
            delete(socket);
        else if (strcmp(buffer, "logout") == 0)
            logout(socket);

        close_connection(socket);
    }
}


int main(int argc, char *argv[])
{ 
    /* allocate memory for the buffer */
    buffer = calloc(sizeof(char *), BUFF_MAX_NUM_LINES);
    for (int i = 0; i < BUFF_MAX_NUM_LINES; i++)
        buffer[i] = calloc(sizeof(char), 1000);

    /* allocate memory for the cookies */
    cookies = calloc(sizeof(char *), MAX_NUM_COOKIES);
    for (int i = 0; i < MAX_NUM_COOKIES; i++)
        cookies[i] = calloc(sizeof(char), 1000);

    parse_input();

    /* free the allocated memory */
    for (int i = 0; i < BUFF_MAX_NUM_LINES; i++)
        free(buffer[i]);

    free(buffer);

    /* allocate memory for the cookies */
    cookies = calloc(sizeof(char *), MAX_NUM_COOKIES);
    for (int i = 0; i < MAX_NUM_COOKIES; i++)
        free(cookies[i]);
    free(cookies);

    return 0;
}
