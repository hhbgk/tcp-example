/**
 * Description:
 * Author:created by bob on 17-12-20.
 */
//

#include <malloc.h>
#include <pthread.h>
#include <assert.h>
#include <unistd.h>
#include <asm/poll.h>
#include <poll.h>
#include <sys/socket.h>
#include "tcp_client.h"
#include "tcp_server.h"

#define tag "TCP_CLIENT"

static int udp_send_frame(int type, uint8_t *buf, uint32_t size, tcp_cli_t *client)
{
    switch (type)
    {
        case TYPE_RTS_PCM:
            rtp_pcm_send_frame(buf, size, client->media->sample);
            break;
        case TYPE_RTS_JPEG:
            rtp_jpeg_send_frame(buf, size, client->media->width, client->media->height);
            break;
        case TYPE_RTS_H264:
            rtp_h264_send_frame(buf, size, client->media->fps);
            break;
        default:
            return -1;
    }
    return 0;
}
static void status_update_callback(tcp_cli_t *tcp_cli, int status)
{
    if (tcp_cli)
    {
        tcp_cli->status = status;
        logi(tag, "status=%s", get_play_state_str(status));
        if (tcp_cli->state_changed_cb) tcp_cli->state_changed_cb(status);
    }
}
static void *tcp_client_runnable(void *arg)
{
    tcp_cli_t *tcp_cli = arg;
    assert(tcp_cli != NULL);
    struct pollfd pollfds[1];
    int timeout = 1000;//milliseconds
    int timeout_count = 0;
    int max_timeout_count = (tcp_cli->timeout==0?NETWORK_TIMEOUT: tcp_cli->timeout)/ timeout;
    int start = 1, type = -1;
    int receive_len;
    uint8_t hdr_buf[PAYLOAD_HEADER_SIZE];
    int is_playing = 0;
    packet_hdr_t packet_hdr;
    packet_hdr_t *packetHdr = &packet_hdr;
    static uint8_t buffer[MAX_FRAME_SIZE];
    static uint8_t frame_buffer[MAX_FRAME_SIZE];
    uint8_t *pBuffer = buffer;
    uint8_t *pFrameBuffer = frame_buffer;

    memset(frame_buffer, 0, MAX_FRAME_SIZE);
    memset(buffer, 0, MAX_FRAME_SIZE);
    pollfds[0].fd = tcp_cli->sockfd;                //设置监控sockfd
    pollfds[0].events = POLLIN | POLLPRI;            //设置监控的事件

    status_update_callback(tcp_cli, RTS_STATE_PREPARE);
    tcp_cli->abort = 0;
    while (!tcp_cli->abort)
    {
        switch (poll(pollfds, 1, timeout))
        {
            case -1:  //函数调用出错
                loge(tag, "poll error:%s ", strerror(errno));
                if (tcp_cli->error_cb) tcp_cli->error_cb(RTS_ERR_EXCEPTION, strerror(errno));
                goto err_output;
            case 0://poll timeout
                if (timeout_count >= max_timeout_count)
                {
                    if (tcp_cli->error_cb) tcp_cli->error_cb(RTS_ERR_TIMEOUT, "Receive data timeout");
                    goto err_output;
                }
                timeout_count++;
                break;
            default: //得到数据返回
                if (!is_playing)
                {
                    is_playing = 1;
                    status_update_callback(tcp_cli, RTS_STATE_PLAYING);
                }
                timeout_count = 0;//re-count timeout
                if (start)
                {
                    receive_len = (int) recv(tcp_cli->sockfd, hdr_buf, PAYLOAD_HEADER_SIZE, MSG_WAITALL);
                    switch (receive_len)
                    {
                        case 0://server has disconnected
                            logw(tag, "1 socket has closed: %s", strerror(errno));
                            goto err_output;
                        case -1:
                            loge(tag, "error:%s", strerror(errno));
                            if (tcp_cli->error_cb) tcp_cli->error_cb(RTS_ERR_EXCEPTION, strerror(errno));
                            goto err_output;
                        default://OK
                            break;
                    }

                    memset(packetHdr, 0, sizeof(packet_hdr_t));
                    memcpy(packetHdr, hdr_buf, (size_t) receive_len);
                    type = packetHdr->frame_info.info.type;
                    start = 0;
                    //loge(tag, "type=%d, frame size=%d, receive_len=%d", type, packetHdr->frame_size, receive_len);
                }
                else
                {
                    receive_len = (int) recv(tcp_cli->sockfd, pBuffer, packetHdr->frame_size, MSG_WAITALL);
                    //logi(tag, "receive len=%d, packetHdr.frame_size=%d", receive_len, packetHdr->frame_size);
                    if (receive_len == 0)
                    {
                        logw(tag, "2 client socket has closed");
                        goto err_output;
                    }
                    else if (receive_len < 0)
                    {
                        if (tcp_cli->error_cb) tcp_cli->error_cb(RTS_ERR_EXCEPTION, strerror(errno));
                        goto err_output;
                    }
                    else if (receive_len == packetHdr->frame_size)
                    {
                        switch (type)
                        {
                            case TYPE_RTS_PCM:
                            case TYPE_RTS_JPEG:
                            case TYPE_RTS_H264:
                            case TYPE_RTS_TIME:
                                memcpy(pFrameBuffer, pBuffer, packetHdr->frame_size);
                                if (tcp_cli->frame_received_cb)
                                    tcp_cli->frame_received_cb(type, pFrameBuffer, packetHdr->frame_size, packetHdr->sequence, packetHdr->timestamp);

                                udp_send_frame(type, pFrameBuffer, packetHdr->frame_size, tcp_cli);
                                break;
                            default:
                                loge(tag, "Unknown type:%d", type);
                                break;
                        }
                        memset(pBuffer, 0, MAX_FRAME_SIZE);
                        start = 1;
                    }
                }
                break;
        }
    }

err_output:
    if (tcp_cli->sockfd > 0) close(tcp_cli->sockfd);
    tcp_cli->sockfd = -1;
    status_update_callback(tcp_cli, RTS_STATE_STOP);
    logi(tag, "TCP client exit");
    pthread_exit(NULL);
    return NULL;
}
int tcp_client_create(tcp_cli_t *tcp_cli)
{
    logi(tag, "%s", __func__);
    assert(tcp_cli != NULL);
    if (rtp_create_socket() < 0)
    {
        loge(tag, "%s: create rtp socket failed", __func__);
        return JNI_FALSE;
    }
    if (pthread_create(&tcp_cli->cli_tid, NULL, tcp_client_runnable, tcp_cli) != 0)
    {
        logw(tag, "Create tcp client runnable failed");
        goto err_output;
    }
    return 0;
err_output:
    logw(tag, "Create tcp client failed");
    return -1;
}
int tcp_client_close(tcp_cli_t *tcp_cli)
{
    logi(tag, "%s", __func__);
    rtp_close_socket();
    if (tcp_cli)
    {
        tcp_cli->abort = 1;
        if (tcp_cli->sockfd > 0) close(tcp_cli->sockfd);
        tcp_cli->sockfd = -1;
        if (tcp_cli->cli_tid) pthread_join(tcp_cli->cli_tid, NULL);
    }
    return 0;
}

void tcp_client_set_frame_received_callback(tcp_cli_t *client, void *cb)
{
    if (client ) client->frame_received_cb = (on_update_cb) cb;
}
void tcp_client_set_error_callback(tcp_cli_t *client, void *cb)
{
    if (client) client->error_cb = cb;
}
void tcp_client_set_state_changed_callback(tcp_cli_t *server, void *cb)
{
    if (server) server->state_changed_cb = cb;
}
void tcp_client_set_resolution(tcp_cli_t *client, int width, int height)
{
    if (client)
    {
        client->width = width;
        client->height = height;
    }
}
void tcp_client_set_fps(tcp_cli_t *client, uint32_t fps)
{
    if (client) client->v_fps = fps;
}
void tcp_client_set_sample_rate(tcp_cli_t *client, uint32_t sr)
{
    if (client) client->a_sr = sr;
}
int tcp_client_is_receiving(tcp_cli_t * cli)
{
    if (cli) return cli->status == RTS_STATE_PLAYING ? 0 : -1;
    return -2;
}
