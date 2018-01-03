/**
 * Description:
 * Author:created by bob on 17-12-20.
 */

#include <assert.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <poll.h>
#include <endian.h>
#include <strings.h>
#include <arpa/inet.h>
#include "tcp_server.h"
#include "tcp_client.h"

#define tag "TCP_SERVER"

static pthread_t  g_tid = 0;
static int gServerfd;
static tcp_cli_t g_tcp_cli;
static int initialized = 0;

static int handle_client(tcp_server_t *server, int fd)
{
    assert(server != NULL);
    /*
    tcp_cli_t *tcp_cli = calloc(1, sizeof(tcp_cli_t));
    if (!tcp_cli) goto err_output;

    tcp_cli->sockfd = fd;
    server->client = tcp_cli;
*/

    tcp_cli_t *tcp_cli = &g_tcp_cli;
    memset(tcp_cli, 0, sizeof(tcp_cli_t));
    tcp_cli->sockfd = fd;
    tcp_cli->media = &server->media;
    tcp_cli->frame_received_cb = server->frame_received_cb;
    tcp_cli->state_changed_cb = server->state_changed_cb;
    tcp_cli->error_cb = server->error_cb;
    return tcp_client_create(tcp_cli);
//err_output:
//    return -1;
}
static void *server_runnable(void *arg)
{
    pthread_detach(pthread_self());
    tcp_server_t *data = arg;
    assert(data != NULL);
    int opt = 1;
    int clientfd;
    struct pollfd fds[1];
    struct sockaddr_in server_addr, client_addr;

    if ((gServerfd = socket(PF_INET,SOCK_STREAM, 0)) < 0)
    {
        loge(tag, "Create server socket failed");
        goto err_output;
    }

    fds[0].fd = gServerfd;                //设置监控sockfd
    fds[0].events = POLLIN | POLLPRI;            //设置监控的事件

    /* build the server's Internet address */
    bzero((char *) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(data->port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    //SO_REUSEADDR是让端口释放后立即就可以被再次使用
    if (setsockopt(gServerfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        loge(tag, "Set sockopt Failed:%s", strerror(errno));
        goto err_output;
    }
    //Bind the server socket
    if (bind(gServerfd, (struct sockaddr *)&server_addr, sizeof(server_addr))< 0)
    {
        loge(tag, "Bind sock failed:%s", strerror(errno));
        //goto err_output;
    }
    //Listen on the server socket
    if (listen(gServerfd, 10) < 0)
    {
        loge(tag, "listen sock Failed:%s", strerror(errno));
        goto err_output;
    }

    data->abort = 0;
    while (!data->abort)
    {
        logi(tag, "Wait for client connection");
        unsigned int client_len = sizeof(client_addr);
        if ((clientfd = accept(gServerfd, (struct sockaddr *)&client_addr, &client_len)) < 0)
        {
            loge(tag, "Failed to accept client connection");
            goto err_output;
        }
        if (handle_client(data, clientfd) < 0)
        {
            loge(tag, "Create client failed");
        }
        else
        {
            logi(tag, "Client connected: %s", inet_ntoa(client_addr.sin_addr));
        }
        //break;
    }

err_output:
    if (gServerfd > 0) close(gServerfd);
    gServerfd = -10;
    logw(tag, "Server exit");
    pthread_exit(NULL);
    return NULL;
}
int tcp_server_create(tcp_server_t *data)
{
    logi(tag, "%s: initialized %d", __func__, initialized);
    if (initialized)
    {
        loge(tag, "%s has called. %d", __func__, initialized);
        return -1;
    }
    initialized = 1;
    if (pthread_create(&g_tid, NULL, server_runnable, data) != 0)
    {
        loge(tag, "Failed to start server thread");
        goto err_output;
    }
    return 0;
err_output:
    initialized = 0;
    return -1;
}
int tcp_server_close(tcp_server_t *data)
{
    logi(tag, "%s:initialized %d", __func__, initialized);
    if (!initialized)
    {
        logw(tag, "%s() called without prior call to create_udp_client() ignored.", __func__);
        return -1;
    }
    initialized = 0;
    //loge(tag, "%s:gServer fd=%d, data =%p", __func__, gServerfd, data);
    if (data)
    {
        //logw(tag, "try to close tcp client %d", gServerfd);
//        tcp_cli_t *client = &g_tcp_cli;
//        if (tcp_client_close(client) != 0)
//            loge(tag, "Close tcp client failed");
        //if (data->client) free(data->client);
        //data->client = NULL;
        data->abort = 1;
    }
    if (gServerfd > 0) close(gServerfd);
    gServerfd = -10;
    //logi(tag, "%s: 11", __func__);
    //if (g_tid) pthread_join(g_tid, NULL);
    logi(tag, "%s: over!!!", __func__);
    return 0;
}
void tcp_server_set_frame_received_callback(tcp_server_t *client, void *cb)
{
    if (client ) client->frame_received_cb = (on_update_cb) cb;
}
void tcp_server_set_error_callback(tcp_server_t *client, void *cb)
{
    if (client) client->error_cb = cb;
}
void tcp_server_set_state_changed_callback(tcp_server_t *server, void *cb)
{
    if (server) server->state_changed_cb = cb;
}
void tcp_server_set_resolution(tcp_server_t *client, int width, int height)
{
    if (client)
    {
        client->media.width = (uint16_t) width;
        client->media.height = (uint16_t) height;
    }
}
void tcp_server_set_fps(tcp_server_t *client, uint32_t fps)
{
    if (client) client->media.fps = fps;
}
void tcp_server_set_sample_rate(tcp_server_t *client, uint32_t sr)
{
    if (client) client->media.sample = sr;
}
int tcp_server_is_receiving()
{
    tcp_cli_t *cli = &g_tcp_cli;
    if (cli) return tcp_client_is_receiving(cli);
    return -2;
}
int tcp_server_close_client(tcp_server_t *server)
{
    logi(tag, "%s", __func__);
    tcp_cli_t *client = &g_tcp_cli;
    if (tcp_client_close(client) != 0)
    {
        loge(tag, "Close tcp client failed");
        return -1;
    }
    return 0;
}
