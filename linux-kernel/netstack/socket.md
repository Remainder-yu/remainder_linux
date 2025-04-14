# 流socket
流socket与电话系统类似：
![Alt text](../images/socket.png)
socket通信实现步骤：
Step 1：创建ServerSocket和Socket
Step 2：打开连接到的Socket的输入/输出流
Step 3：按照协议对Socket进行读/写操作
Step 4：关闭输入输出流，以及Socket
# socket的基本操作
参考：
[socket详解-博客](https://blog.csdn.net/gswwldp/article/details/73557998)
socket是“open—write/read—close”模式的一种实现

## socket函数

```c
int socket(int domain, int type, int protocol);
```
参数说明：
* domain：即协议域，又称协议族（family）。常见：AF_INET,AF_INET6，AF_LOCAL等。
* type：指定socket类型。常见的socket类型有，SOCK_STREAM、SOCK_DGRAM、SOCK_RAW、SOCK_PACKET、SOCK_SEQPACKET等等。
* protocol：指定协议。常用的协议有，IPPROTO_TCP、IPPTOTO_UDP、IPPROTO_SCTP、IPPROTO_TIPC等，它们分别对应TCP传输协议、UDP传输协议、STCP传输协议、TIPC传输协议。

当我们调用socket创建一个socket时，返回的socket描述字它存在于协议族（address family，AF_XXX）空间中，但没有一个具体的地址。如果想要给它赋值一个地址，就必须调用bind()函数，否则就当调用connect()、listen()时系统会自动随机分配一个端口。


## bind函数
```c
int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
```
参数说明：
* sockfd：即socket描述字，它是通过socket()函数创建了，唯一标识一个socket。bind()函数就是将给这个描述字绑定一个名字。
* addr：一个const struct sockaddr *指针，指向要绑定给sockfd的协议地址。详细参考struct sockaddr_in结构体
* addrlen：对应的是地址的长度。

通常服务器在启动的时候都会绑定一个众所周知的地址（如ip地址+端口号），用于提供服务，客户就可以通过它来接连服务器；而客户端就不用指定，有系统自动分配一个端口号和自身的ip地址组合。这就是为什么通常服务器端在listen之前会调用bind()，而客户端就不会调用，而是在connect()时由系统随机生成一个。

```c
struct sockaddr_in {
    sa_family_t    sin_family; /* address family: AF_INET */
    in_port_t      sin_port;   /* port in network byte order */
    struct in_addr sin_addr;   /* internet address */
};
```
## listen、connect函数
如果作为一个服务器，在调用socket()、bind()之后就会调用listen()来监听这个socket，如果客户端这时调用connect()发出连接请求，服务器端就会接收到这个请求。
```c
int listen(int sockfd, int backlog);
int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
```
listen函数的第一个参数即为要监听的socket描述字，第二个参数为相应socket可以排队的最大连接个数。socket()函数创建的socket默认是一个主动类型的，listen函数将socket变为被动类型的，等待客户的连接请求。
connect函数的第一个参数即为客户端的socket描述字，第二参数为服务器的socket地址，第三个参数为socket地址的长度。客户端通过调用connect函数来建立与TCP服务器的连接。

listen系统调用将文件描述符sockfd引用的流socket标记为被动。
connect系统调用将文件描述符sockfd引用的主动socket连接到地址通过addr和addrlen指定的监听socket上。
## accept函数
TCP服务器端依次调用 socket，bind，listen之后，就会监听指定的socket地址。Tcp客户端依次调用socket，connect之后就想TCP服务器发送一个连接请求。TCP服务器监听到这个请求之后，就会调用accept函数取接收请求，这样就建立好连接。之后就可以开始网络I/O操作，即类同于普通文件的读写I/O操作。

accept系统调用在文件描述符sockfd引用的监听流socket上接受一个接入连接。
如果在调用accept时不存在未决的连接，那么调用就会阻塞直到有连接请求为止。

理解accept()的关键点是它会创建一个新的socket，并且正式这个socket会与执行connect对等的socket进行连接。

```c
int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
```
参数说明：
* sockfd：服务器的socket文件描述符
* addr：用于返回客户端的协议地址
* addrlen：协议地址的长度

如果accept成功，那么返回值是由内核生成全新的sockfd，代表与返回客户的TCP连接。

>注意：accept第一个参数为服务器创建的socket描述字，是服务器开始调用socket函数生成的， 称为**监听socket描述字**。而accpet函数返回的则是已连接socket描述字。


![Alt text](../images/未决的socket连接.png)

## read,write函数

read函数是负责从fd中读取内容。
```c
read()/write()
recv()/send()
readv()/writev()
recvmsg()/sendmsg()
recvfrom()/sendto()
```

## close函数
在服务器与客户端建立连接之后，会进行一些读写操作，完成了读写操作就要关闭相应的socket描述字，好比操作完打开的文件要调用fclose关闭打开的文件。
```c
#include <unistd.h>
int close(int fd);
```
注意：close操作只是使相应socket描述字的引用计数-1，只有当引用计数为0的时候，才会触发TCP客户端向服务器发送终止连接请求。

# TCP三次握手
核心流程：
* 客户端向服务器发送一个SYN J
* 服务器向客户端响应一个SYN K，并对SYN J进行确认ACK+1
* 客户端再向服务器发一个确认ACK K+1

联系函数调用：
当客户端调用connect时，触发了连接请求，向服务器发送了SYN J包，这时connect进入阻塞状态；服务器监听到连接请求，即收到SYN J包，调用accept函数接收请求向客户端发送SYN K ，ACK J+1，这时accept进入阻塞状态；客户端收到服务器的SYN K ，ACK J+1之后，这时connect返回，并对SYN K进行确认；服务器收到ACK K+1时，accept返回，至此三次握手完毕，连接建立。
**总结：客户端的connect在三次握手的第二次返回，而服务器端的accept在三次握手的第三次返回。**

## socket-TCP四次挥手

* 某个应用进程首先调用 close主动关闭连接，这时TCP发送一个FIN M；
* 另一端接收到FIN M之后，执行被动关闭，对这个FIN进行确认。它的接收也作为文件结束符传递给应用进程，因为FIN的接收意味着应用进程在相应的连接上再也接收不到额外数据；
* 一段时间之后，接收到文件结束符的应用进程调用 close关闭它的socket。这导致它的TCP也发送一个FIN N；
* 接收到这个FIN的源发送端TCP对它进行确认。
这样每个方向上都有一个FIN和ACK。