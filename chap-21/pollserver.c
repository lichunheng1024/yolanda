
#include "lib/common.h"

#define INIT_SIZE 128

int main(int argc, char **argv) {
    int listen_fd, connected_fd;
    int ready_number;
    ssize_t n;
    char buf[MAXLINE];
    struct sockaddr_in client_addr;

    //创建一个监听套接字，并绑定在本地的地址和端口上
    listen_fd = tcp_server_listen(SERV_PORT);

    //lch c语言基础知识补充：结构数组就是具有相同结构类型的变量集合。

    //初始化pollfd数组，这个数组的第一个元素是listen_fd，其余的用来记录将要连接的connect_fd
    struct pollfd event_set[INIT_SIZE];

    /**
     * 将listen_fd 和 POLLRDNORM事件加入到event_set里，
     * 表示我们期望系统内核检测监听套接字上的连接建立完成事件
    */
    event_set[0].fd = listen_fd;
    //POLLRDNORM : Normal data may be read without blocking.
    event_set[0].events = POLLRDNORM;



    // 用-1表示这个数组位置还没有被占用
    //设置为 -1 表示 poll函数将会忽略这个pollfd，也表示当前pollfd没被使用的意思
    int i;
    for (i = 1; i < INIT_SIZE; i++) {
        event_set[i].fd = -1;
    }

    for (;;) {
        /**
         *  int poll(struct pollfd fds[], int nfds, int timeout)
         *  fds[]    pollfd结构体类型
         *  nfds     fds[]数组大小
         *  timeout  若为-1 如果没有就绪事件，就无限期等下去
         *           若为0，表示不阻塞进程，立即返回
         *           若大于0，表示等待指定的毫秒值
         * 返回值
         *      若为-1， 表示出错
         *      若为0，  表示超时
         *      若大于0，表示就绪fd的数目
         *
         */
        if ((ready_number = poll(event_set, INIT_SIZE, -1)) < 0) {
            error(1, errno, "poll failed ");
        }

        if (event_set[0].revents & POLLRDNORM) {
            socklen_t client_len = sizeof(client_addr);
            connected_fd = accept(listen_fd, (struct sockaddr *) &client_addr, &client_len);

            //找到一个可以记录该连接套接字的位置
            for (i = 1; i < INIT_SIZE; i++) {
                if (event_set[i].fd < 0) {
                    event_set[i].fd = connected_fd;
                    event_set[i].events = POLLRDNORM;
                    break;
                }
            }
            //如果连接占满了event_set 则打印个错误出来
            if (i == INIT_SIZE) {
                error(1, errno, "can not hold so many clients");
            }
            //如果是第一次建立连接事件， 则跳过后续执行
            if (--ready_number <= 0)
                continue;
        }

        for (i = 1; i < INIT_SIZE; i++) {
            int socket_fd;
            // 值为 -1 的跳过执行
            if ((socket_fd = event_set[i].fd) < 0)
                continue;
            if (event_set[i].revents & (POLLRDNORM | POLLERR)) {
                // 处理读事件
                if ((n = read(socket_fd, buf, MAXLINE)) > 0) {
                    if (write(socket_fd, buf, n) < 0) {
                        error(1, errno, "write error");
                    }
                } else if (n == 0 || errno == ECONNRESET) {
                    close(socket_fd);
                    // 重置  -1 代表 poll 函数 不关心
                    event_set[i].fd = -1;
                } else {
                    error(1, errno, "read error");
                }
                //性能优化
                if (--ready_number <= 0)
                    break;
            }
        }
    }
}
