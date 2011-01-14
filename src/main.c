/*
 * =====================================================================================
 *
 *       Filename:  main.c
 *
 *    Description:  Entrance point to application - command line parsing, daemonizing, etc.
 *
 *        Version:  1.0
 *        Created:  01/11/2011 12:32:17 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Nikolay Bryskin (devel.niks@gmail.com)
 *        Company:  Yandex
 *
 * =====================================================================================
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <err.h>

#include "cmdline.h"
#include "../contrib/http-parser/http_parser.h"

static int start_server(struct gengetopt_args_info* args, int* server_sock);

int main(int argc, char* argv[])
{
    printf("Hello world from %s!\n", PACKAGE_STRING);
    struct gengetopt_args_info args;
    int res = cmdline_parser(argc, argv, &args_info);
    if (res != 0)
        exit(EXIT_FAILURE);

    printf("listening on %s:%hu\n", args.listen_address_arg, args.listen_port_arg);

    int server_sock;
    if (start_server(&args, &server_sock) != 0)
        err(EXIT_FAILURE, "Could not start server");

    pthread_t workers[args.worker_count_arg];
    start_worker_threads(&args_info, workers, args.worker_count_arg);

    join_worker_threads(workers, args.worker_count_args);

    return 0;
}

int start_server(struct gengetopt_args_info* args, int* server_sock)
{
    struct addrinfo hints;

    bzero(&hints, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    struct addrinfo *result;
    int s = getaddrinfo(args->listen_address_arg, NULL, &hints, &result);
    if (s != 0)
        errx(EXIT_FAILURE, "could not get address info: %s", gai_strerror(s));
    
    struct addrinfo* rp;
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        int sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sock == -1)
            continue;

        if (bind(sock, rp->ai_addr, rp->ai_addrlen) != 0) {
            warn("could not bind socket to addr %s", rp->ai_canonname);
            close(sock);
            continue;
        }

        if (listen(sock, args->backlog_len_arg) != 0) {
            warn("could not listen on socket");
            close(sock);
        }

        close(sock);
    }

    if (rp == NULL)
        err(EXIT_FAILURE, "Could not bind");

    freeaddrinfo(result);

    return 0;
}

void start_worker_threads(gengetopt_args_info* args, int worker_count; pthread_t workers[worker_count], int worker_count)
{
    pthread_attr_t attr;
    errno = pthread_attr_init(&attr);
    if (errno != 0)
        err(EXIT_FAILURE, "pthread_attr_init");

    errno = pthread_attr_setstacksize(&attr, stack_size);
    if (errno != 0)
        err(EXIT_FAILURE, "pthread_attr_setstacksize(%d)", stack_size);

#ifdef USE_PTHREAD
    errno = pthread_attr_setschedpolicy(&attr, SCHED_RR);
    if (errno != 0)
        err(EXIT_FAILURE, "pthread_attr_setschedpolicy(SCHED_RR)");

    struct sched_param param = {2};
    errno = pthread_attr_setschedparam(&attr, &param);
    if (errno != 0)
        err(EXIT_FAILURE, "pthread_attr_setschedparam(priority=%d)", param.sched_priority);
#endif

    for (int i=0; i < worker_count; ++i) {
        pthread_create(&workers[i], &attr, accept_loop);
    }
}

void* accept_loop(void* arg)
{
    proxy_t* proxy = (proxy_t*)arg;

    while (proxy->running) {
        struct sockaddr client_addr;
        socklen_t addr_len = sizeof client_addr;
        int sock = accept(proxy->server_sock, (struct sockaddr*)&client_addr, &addr_len);
        if (sock == -1) {
            warn("could not accept connection");
            continue;
        }
        
        char host[NI_MAXHOST];
        char serv[NI_MAXSERV];
        int s = getnameinfo(&client_addr, addr_len, host, sizeof host, serv, sizeof serv, 0);
        if (s != 0)
            warnx("could not resolve client name info: %s", gai_strerror(s));
        else
            printf("accepted remote connection from %s:%s", host, serv);

        receive_loop(proxy, sock);
        close(sock);
    }
}

void receive_loop(proxy_t* proxy, http_parser* parser, int client_sock)
{
    auto on_message_begin(http_parser* parser);

    static struct http_parser_settings {
        .on_message_begin = on_message_begin,
        .on_path = on_path,
        .on_query_string = on_query_string,
        .on_url = on_url,
        .on_fragment = on_fragment,
        .on_header_field = on_header_field,
        .on_header_value = on_header_value,
        .on_headers_complete = on_header_complete,
        .on_body = on_body,
        .on_message_complete = on_message_compelete,
    } parser_settings;

    while (proxy->running) {
        http_parser parser;
        http_parser_init(&parser, HTTP_REQUEST);

        char 

        size_t len = proxy->args->buffer_size_arg;
        char buf[len];
        size_t received;
        do {
            recved = recv(client_sock, buf, len, 0);
            if (recved < 0) {
                warn("could not receive data from client");
                break;
            }

            size_t nparsed = http_parser_execute(parser, &parser_settings, buf, recved);
            if (nparsed != recved) {
                warn("could not parse client request");
                return;
            }
        } while (received > 0);

        struct sockaddr server_addr;
        int res = resolve_destination(url, path - url, &server_addr);
        if (res != 0)
            continue;

        int server_sock;
        res = connect_to_server(&server_addr, &server_sock);
        if (res != 0)
            continue;

        res = send_request_to_server(server_sock, 
    }
}
