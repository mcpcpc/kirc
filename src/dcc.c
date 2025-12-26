/*
 * dcc.c
 * Direct client-to-client (DCC) handling
 * Author: Michael Czigler
 * License: MIT
 */

#include "dcc.h"

static int sanitize_filename(char *filename)
{
    if (filename == NULL) {
        return -1;
    }

    if ((filename[0] == '\0') || (filename[0] == '.')) {
        return -1;
    }

    if (strstr(filename, "..") != NULL) {
        return -1;
    }

    if (strchr(filename, '/') != NULL) {
        return -1;
    }

    if (strchr(filename, '\\') != NULL) {
        return -1;
    }

    if (strlen(filename) >= NAME_MAX) {
        return -1;
    }

    return 0;
}

int dcc_init(dcc_t *dcc, kirc_t *ctx)
{
    if ((dcc == NULL) || (ctx == NULL)) {
        return -1;
    }

    memset(dcc, 0, sizeof(*dcc));
    dcc->ctx = ctx;

    int limit = KIRC_DCC_TRANSFERS_MAX;

    for (int i = 0; i < limit; ++i) {
        dcc->sock_fd[i].fd = -1;
        dcc->sock_fd[i].events = POLLIN;
        dcc->transfer[i].state = DCC_STATE_IDLE;
        dcc->transfer[i].file_fd = -1;
    }

    return 0;
}

int dcc_free(dcc_t *dcc)
{
    if (dcc == NULL) {
        return -1;
    }

    int limit = KIRC_DCC_TRANSFERS_MAX;

    for (int i = 0; i < limit; ++i) {
        if (dcc->sock_fd[i].fd >= 0) {
            close(dcc->sock_fd[i].fd);
            dcc->sock_fd[i].fd = -1;
        }

        if (dcc->transfer[i].file_fd >= 0) {
            close(dcc->transfer[i].file_fd);
            dcc->transfer[i].file_fd = -1;
        }
    }

    return 0;
}

int dcc_send(dcc_t *dcc, int transfer_id)
{
    if ((dcc == NULL) || (transfer_id < 0) ||
        (transfer_id >= KIRC_DCC_TRANSFERS_MAX)) {
        return -1;
    }

    dcc_transfer_t *transfer = &dcc->transfer[transfer_id];

    if (transfer->type != DCC_TYPE_SEND) {
        printf("\r" CLEAR_LINE DIM "error: %d is not a SEND transfer"
            RESET "\r\n", transfer_id);
        return -1;
    }

    if (transfer->state != DCC_STATE_TRANSFERRING) {
        printf("\r" CLEAR_LINE DIM "error: %d is not in the transferring state"
            RESET "\r\n", transfer_id);
        return -1;
    }

    char buffer[KIRC_DCC_BUFFER_SIZE];
    ssize_t nread = read(transfer->file_fd, buffer,
        sizeof(buffer));

    if (nread < 0) {
        printf("\r" CLEAR_LINE DIM "error: file read failed"
            RESET "\r\n");
        transfer->state = DCC_STATE_ERROR;
        return -1;
    }

    if (nread == 0) {
        printf("\r" CLEAR_LINE DIM "dcc: %d transfer complete"
            RESET "\r\n", transfer_id);
        transfer->state = DCC_STATE_COMPLETE;
        return 0;
    }

    ssize_t nsent = write(dcc->sock_fd[transfer_id].fd,
        buffer, nread);

    if (nsent < 0) {
        if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
            return 0;
        }

        printf("\r" CLEAR_LINE DIM "error: send failed"
            RESET "\r\n");
        transfer->state = DCC_STATE_ERROR;
        return -1;
    }

    transfer->sent += nsent;

    if (transfer->sent >= transfer->filesize) {
        printf("\r" CLEAR_LINE DIM "dcc: %d transfer complete"
            RESET "\r\n", transfer_id);
        transfer->state = DCC_STATE_COMPLETE;
    }
    
    return nsent;
}

int dcc_process(dcc_t *dcc)
{
    if (dcc == NULL) {
        return -1;
    }

    if (dcc->transfer_count == 0) {
        return 0;
    }

    int limit = KIRC_DCC_TRANSFERS_MAX;
    int rc = poll(dcc->sock_fd, limit, 0);

    if (rc < 0) {
        if (errno == EINTR) {
            return 0;
        }

        printf("\r" CLEAR_LINE DIM "error: poll failed"
            RESET "\r\n");
        return -1;
    }

    if (rc == 0) {
        return 0;
    }

    /* process each transfer */
    for (int i = 0; i < limit; ++i) {
        if (dcc->sock_fd[i].fd < 0) {
            continue;
        }

        dcc_transfer_t *transfer = &dcc->transfer[i];

        if (transfer->state == DCC_STATE_CONNECTING) {
            if (dcc->sock_fd[i].revents & POLLOUT) {
                int error = 0;
                socklen_t len = sizeof(error);
                if (getsockopt(dcc->sock_fd[i].fd, SOL_SOCKET, SO_ERROR, &error, &len) == 0) {
                    if (error == 0) {
                        printf("\r" CLEAR_LINE DIM "dcc: %d connected"
                            RESET "\r\n", i);
                        transfer->state = DCC_STATE_TRANSFERRING;
                        dcc->sock_fd[i].events = POLLIN;
                    } else {
                        printf("\r" CLEAR_LINE DIM "error: connection failed"
                            RESET "\r\n");
                        transfer->state = DCC_STATE_ERROR;
                    }
                }
            }
            continue;
        }

        /* handle receive transfers */
        if ((transfer->type == DCC_TYPE_RECEIVE) &&
            (transfer->state == DCC_STATE_TRANSFERRING)) {

            if (dcc->sock_fd[i].revents & POLLIN) {
                char buffer[KIRC_DCC_BUFFER_SIZE];
                ssize_t nread = read(dcc->sock_fd[i].fd, buffer,
                    sizeof(buffer));

                if (nread < 0) {
                    if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                        continue; 
                    }

                    printf("\r" CLEAR_LINE DIM "error: receive failed"
                        RESET "\r\n");
                    transfer->state = DCC_STATE_ERROR;
                    continue;
                }
                
                if (nread == 0) {
                    if (transfer->sent >= transfer->filesize) {
                        printf("\r" CLEAR_LINE DIM "dcc: %d transfer complete (%llu bytes)"
                            RESET "\r\n", i, transfer->sent);
                        transfer->state = DCC_STATE_COMPLETE;
                    } else {
                        printf("\r" CLEAR_LINE DIM "error: %d transfer incomplete (%llu/%llu bytes)"
                            RESET "\r\n", i, transfer->sent, transfer->filesize);
                        transfer->state = DCC_STATE_COMPLETE;
                    }
                    continue;
                }

                ssize_t nwritten = write(transfer->file_fd, buffer, nread);

                if (nwritten < 0) {
                    printf("\r" CLEAR_LINE DIM "error: write failed"
                        RESET "\r\n");
                    transfer->state = DCC_STATE_ERROR;
                    continue;
                }

                transfer->sent += nwritten;

                if (transfer->sent >= transfer->filesize) {
                    printf("\r" CLEAR_LINE DIM "dcc: %d transfer complete (%llu bytes)"
                        RESET "\r\n", i, transfer->sent);
                    transfer->state = DCC_STATE_COMPLETE;
                }
            }
        }
        
        /* handle send transfer */
        if ((transfer->type == DCC_TYPE_SEND) &&
            (transfer->state == DCC_STATE_TRANSFERRING)) {
            if (dcc->sock_fd[i].revents & POLLOUT) {
                dcc_send(dcc, i);
            }
        }

        /* cleanup completed or error transfers */
        if ((transfer->state == DCC_STATE_COMPLETE) ||
            (transfer->state == DCC_STATE_ERROR)) {
            if (dcc->sock_fd[i].fd >= 0) {
                close(dcc->sock_fd[i].fd);
                dcc->sock_fd[i].fd = -1;
            }

            if (dcc->transfer[i].file_fd >= 0) {
                close(dcc->transfer[i].file_fd);
                dcc->transfer[i].file_fd = -1;
            }

            transfer->state = DCC_STATE_IDLE;
            dcc->transfer_count--;
        }
    }

    return 0;
}

int dcc_request(dcc_t *dcc, const char *sender, const char *params)
{
    if ((dcc == NULL) || (sender == NULL) || (params == NULL)) {
        return -1;
    }

    /* find free transfer slot */
    int transfer_id = -1;
    int limit = KIRC_DCC_TRANSFERS_MAX;

    for (int i = 0; i < limit; ++i) {
        if (dcc->transfer[i].state == DCC_STATE_IDLE) {
            transfer_id = i;
            break;
        }
    }

    if (transfer_id < 0) {
        printf("\r" CLEAR_LINE DIM "error: no free DCC transfer slots"
            RESET "\r\n");
        return -1;
    }

    /* parse DCC SEND parameters: SEND filename ip port filesize */
    char params_copy[MESSAGE_MAX_LEN];
    size_t siz = sizeof(params_copy);
    safecpy(params_copy, params, siz);

    char *command = strtok(params_copy, " ");
    if ((command == NULL) || (strcmp(command, "SEND") != 0)) {
        printf("\r" CLEAR_LINE DIM "error: unsupported DCC command"
            RESET "\r\n");
        return -1;
    }

    char *filename_tok = strtok(NULL, " ");
    if (filename_tok == NULL) {
        printf("\r" CLEAR_LINE DIM "error: invalid DCC SEND format"
            RESET "\r\n");
        return -1;
    }

    /* handle quoted filenames */
    char filename[NAME_MAX];
    if (filename_tok[0] == '"') {
        /* find closing quote */
        char *end_quote = strchr(filename_tok + 1, '"');
        if (end_quote != NULL) {
            size_t len = end_quote - filename_tok - 1;
            if (len >= NAME_MAX) {
                len = NAME_MAX;
            } else {
                len = len + 1;
            }
            safecpy(filename, filename_tok + 1, len);
        } else {
            /* filename spans multiple tokens */
            size_t len = strlen(filename_tok + 1);

            if (len >= NAME_MAX) {
                len = NAME_MAX;
            } else {
                len = len + 1;
            }

            safecpy(filename, filename_tok + 1, len);

            /* continue reading until closing quote */
            char *next = strtok(NULL, "\"");

            if (next != NULL) {
                size_t flen = strlen(filename);
                size_t nlen = strlen(next);

                if (flen + nlen + 1 < NAME_MAX) {
                    filename[flen] = ' ';
                    strncpy(filename + flen + 1, next, NAME_MAX - flen - 2);
                    filename[NAME_MAX - 1] = '\0';
                }
            }
        }
    } else {
        siz = sizeof(filename);
        safecpy(filename, filename_tok, siz);
    }

    if (sanitize_filename(filename) < 0) {
        printf("\r" CLEAR_LINE DIM "error: invalid or unsafe filename"
            RESET "\r\n");
        return -1;
    }

    char *server = strtok(NULL, " ");
    char *port = strtok(NULL, " ");
    char *filesize = strtok(NULL, " ");

    if ((server == NULL) || (port == NULL) || (filesize == NULL)) {
        printf("\r" CLEAR_LINE DIM "error: invalid DCC SEND format"
            RESET "\r\n");
        return -1;
    }

    /* initialize transfer */
    dcc_transfer_t *transfer = &dcc->transfer[transfer_id];
    transfer->type = DCC_TYPE_RECEIVE;
    transfer->state = DCC_STATE_CONNECTING;
    transfer->filesize = strtoull(filesize, NULL, 10);
    transfer->sent = 0;
    
    siz = sizeof(transfer->filename);
    safecpy(transfer->filename, filename, siz);
    
    siz = sizeof(transfer->sender);
    safecpy(transfer->sender, sender, siz);

    /* open file for writing */
    transfer->file_fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if (transfer->file_fd < 0) {
        printf("\r" CLEAR_LINE DIM "error: cannot create file %s"
            RESET "\r\n", filename);
        transfer->state = DCC_STATE_IDLE;
        return -1;
    }

    /* create socket */
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int rc = getaddrinfo(server, port, &hints, &res);
    if (rc != 0) {
        printf("\r" CLEAR_LINE DIM "error: getaddrinfo failed: %s"
            RESET "\r\n", gai_strerror(rc));
        close(transfer->file_fd);
        transfer->file_fd = -1;
        transfer->state = DCC_STATE_IDLE;
        return -1;
    }

    int sock_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock_fd < 0) {
        printf("\r" CLEAR_LINE DIM "error: socket creation failed"
            RESET "\r\n");
        freeaddrinfo(res);
        close(transfer->file_fd);
        transfer->file_fd = -1;
        transfer->state = DCC_STATE_IDLE;
        return -1;
    }

    /* set non-blocking */
    int flags = fcntl(sock_fd, F_GETFL, 0);
    fcntl(sock_fd, F_SETFL, flags | O_NONBLOCK);

    /* connect */
    rc = connect(sock_fd, res->ai_addr, res->ai_addrlen);
    if ((rc < 0) && (errno != EINPROGRESS)) {
        printf("\r" CLEAR_LINE DIM "error: connection failed"
            RESET "\r\n");
        close(sock_fd);
        freeaddrinfo(res);
        close(transfer->file_fd);
        transfer->file_fd = -1;
        transfer->state = DCC_STATE_IDLE;
        return -1;
    }

    freeaddrinfo(res);

    dcc->sock_fd[transfer_id].fd = sock_fd;
    dcc->sock_fd[transfer_id].events = POLLOUT;
    dcc->transfer_count++;

    printf("\r" CLEAR_LINE DIM "dcc: receiving %s from %s (%llu bytes)"
        RESET "\r\n", transfer->filename, transfer->sender,
        transfer->filesize);

    return transfer_id;
}

int dcc_cancel(dcc_t *dcc, int transfer_id)
{
    if ((dcc == NULL) || (transfer_id < 0) ||
        (transfer_id >= KIRC_DCC_TRANSFERS_MAX)) {
        return -1;
    }

    dcc_transfer_t *transfer = &dcc->transfer[transfer_id];

    if (transfer->state == DCC_STATE_IDLE) {
        return 0;
    }

    printf("\r" CLEAR_LINE DIM "dcc: cancelling transfer %d"
        RESET "\r\n", transfer_id);

    if (dcc->sock_fd[transfer_id].fd >= 0) {
        close(dcc->sock_fd[transfer_id].fd);
        dcc->sock_fd[transfer_id].fd = -1;
    }

    if (dcc->transfer[transfer_id].file_fd >= 0) {
        close(dcc->transfer[transfer_id].file_fd);
        dcc->transfer[transfer_id].file_fd = -1;
    }

    transfer->state = DCC_STATE_IDLE;
    dcc->transfer_count--;

    return 0;
}
