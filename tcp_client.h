/**
 * Description:
 * Author:created by bob on 17-12-20.
 */
//

#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H

#include <sys/types.h>
#include "tcp_server.h"

int tcp_client_create(tcp_cli_t *tcp_cli);
int tcp_client_close(tcp_cli_t *tcp_cli);
void tcp_client_set_error_callback(tcp_cli_t *, void *cb);
void tcp_client_set_resolution(tcp_cli_t *, int width, int height);
void tcp_client_set_fps(tcp_cli_t *, uint32_t fps);
void tcp_client_set_sample_rate(tcp_cli_t *, uint32_t sr);
void tcp_client_set_state_changed_callback(tcp_cli_t *, void *cb);
void tcp_client_set_frame_received_callback(tcp_cli_t *, void *cb);
int tcp_client_is_receiving(tcp_cli_t *);
#endif //TCP_CLIENT_H
