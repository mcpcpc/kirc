/*
 * dcc.c
 * Direct client-to-client (DCC) handling
 * Author: Michael Czigler
 * License: MIT
 */

#include "dcc.h"

int dcc_init(dcc_t *dcc, kirc_t *ctx)
{
    if ((dcc == NULL) || (ctx == NULL)) {
        return -1;
    }

    memset(dcc, 0, sizeof(*dcc));
    dcc->ctx = ctx;
    dcc->transfer_count = 0;

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
    if ((dcc == NULL) || (transfer_id < 0)) {
        return -1;
    }

    dcc_transfer_t *transfer = &dcc->transfer[transfer_id];

    if (transfer->type != DCC_TYPE_SEND) {
        printf("\r" DIM "error: %d is not a SEND transfer" RESET "\r\n",
            transfer_id);
        return -1;
    }

    if (transfer->state != DCC_STATE_TRANSFERRING) {
        printf("\r" DIM "error: %d is not in the transferring state" RESET "\r\n",
            transfer_id);
        return -1;
    }

    char buffer[KIRC_DCC_BUFFER_SIZE];
    ssize_t nread = read(transfer->file_fd, buffer,
        sizeof(buffer));

    if (nread < 0) {
        printf("\r" DIM "error: file read failed" RESET "\r\n");
        transfer->state = DCC_STATE_ERROR;
        return -1;
    }

    if (nread == 0) {
        printf("\r" DIM "dcc: %d transfer complete" RESET "\r\n",
            transfer_id);
        transfer->state = DCC_STATE_COMPLETE;
        return 0;
    }

    ssize_t nsent = write(dcc->sock_fd[transfer_id].fd,
        buffer, nread);

    if (nsent < 0) {
        if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
            return 0;
        }

        printf("\r" DIM "error: send failed" RESET "\r\n");
        transfer->state = DCC_STATE_ERROR;
        return -1;
    }

    transfer->bytes_transferred += nsent;

    if (transfer->bytes_transferred >= transfer->bytes_filesize) {
        printf("\r" DIM "dcc: %d transfer complete" RESET "\r\n",
            transfer_id);
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

        printf("\r" DIM "error: poll failed" RESET "\r\n");
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
                        printf("\r" DIM "dcc: %d connected" RESET "\r\n", i);
                        transfer->state = DCC_STATE_TRANSFERRING;
                        dcc->sock_fd[i].events = POLLIN;
                    } else {
                        printf("\r" DIM "error: connection failed" RESET "\r\n");
                        transfer->state = DCC_STATE_ERROR;
                    }
                }
            }
            continue;
        }

        /* handle receive transfers */
        if ((transfer->type == DCC_TYPE_RECEIVE) &&
            (transfer->state == DCC_STATE_TRANSFERRING)) {

            if (dcc->sock_fd[i].revents && POLLIN) {
                char buffer[KIRC_DCC_BUFFER_SIZE];
                ssize_t nread = read(dcc->sock_fd[i].fd, buffer,
                    sizeof(buffer));

                if (nread < 0) {
                    if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                        continue; 
                    }

                    printf("\r" DIM "error: receive failed" RESET "\r\n");
                    transfer->state = DCC_STATE_ERROR;
                    continue;
                }
                
                if (nread == 0) {
                    if (transfer->bytes_transferred >= transfer->bytes_filesize) {
                        printf("\r" DIM "dcc: %d transfer complete (%u bytes)"
                            RESET "\r\n", i, transfer->bytes_transferred);
                        transfer->state = DCC_STATE_COMPLETE;
                    } else {
                        printf("\r" DIM "error: %d transfer incomplete (%u/%u bytes)"
                            RESET "\r\n", i, transfer->bytes_transferred,
                            transfer->bytes_filesize);
                        transfer->state = DCC_STATE_COMPLETE;
                    }
                    continue;
                }

                ssize_t nwritten = write(transfer->file_fd, buffer, nread);

                if (nwritten < 0) {
                    printf("\r" DIM "error: write failed" RESET "\r\n");
                    transfer->state = DCC_STATE_ERROR;
                    continue;
                }

                transfer->bytes_transferred += nwritten;

                if (transfer->bytes_transferred >= transfer->bytes_filesize) {
                    printf("\r" DIM "dcc: %d transfer complete (%u bytes)" RESET "\r\n",
                        i, transfer->bytes_transferred);
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

        /* cleanup completed or erro transfers */
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
