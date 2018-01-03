/**
 * Description:
 * Author:created by bob on 17-12-20.
 */
//

#ifndef TCP_SERVER_H
#define TCP_SERVER_H

enum RTS_TYPE
{
    TYPE_RTS_PCM  = 1,//pcm
    TYPE_RTS_JPEG  = 2,//JPG
    TYPE_RTS_H264  = 3,//h264
    TYPE_RTS_PIC = 4,//Picture
    TYPE_RTS_TIME = 5,//
    TYPE_MEDIA_META = 6,//media meta
    TYPE_FILE_END = 7//Flag file end
};

typedef struct
{
    int abort;
    int sockfd;
    int timeout;//network connection timeout
    int status;
    int width;
    int height;
    uint32_t v_fps;//frame rate
    uint32_t a_sr;//sample rate
    media_t *media;

    on_error_cb error_cb;
    on_update_cb frame_received_cb;
    on_state_changed_cb state_changed_cb;

    pthread_t cli_tid;
}tcp_cli_t;
typedef struct
{
    int sockfd;
    int abort;
    int port;
    media_t media;

    on_error_cb error_cb;
    on_update_cb frame_received_cb;
    on_state_changed_cb state_changed_cb;

    //tcp_cli_t *client;
}tcp_server_t;
int tcp_server_create(tcp_server_t *data);
int tcp_server_close(tcp_server_t *data);
int tcp_server_close_client(tcp_server_t*);

void tcp_server_set_error_callback(tcp_server_t *, void *cb);
void tcp_server_set_resolution(tcp_server_t *, int width, int height);
void tcp_server_set_fps(tcp_server_t *, uint32_t fps);
void tcp_server_set_sample_rate(tcp_server_t *, uint32_t sr);
void tcp_server_set_state_changed_callback(tcp_server_t *, void *cb);
void tcp_server_set_frame_received_callback(tcp_server_t *, void *cb);
int tcp_server_is_receiving();
#endif //TCP_SERVER_H
