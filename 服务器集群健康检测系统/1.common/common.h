#include <stdio.h> //stdio.h头文件定义了三个变量类型、一些宏和各种函数来执行输入和输出
#include <stdlib.h> //stdlib 头文件里包含了C、C++语言的最常用的系统函数,如malloc()、calloc()、realloc()、free()等
#include <string.h> //string .h 头文件定义了各种操作字符数组的函数
#include <unistd.h> //unistd.h 中所定义的接口通常都是大量针对系统调用的封装，如 fork、pipe 以及各种 I/O 原语（read、write、close 等等)
#include <sys/socket.h> //包含socket相关函数的头文件
#include <arpa/inet.h> //主要定义了格式转换函数
#include <errno.h>
#include <sys/file.h>
#include <time.h>
#include <sys/time.h>
#include <stdarg.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>

#define MAXBUFF 4096

int socket_create(int port);
int socket_connect(char *ip, int port);
int get_conf_value(char const *file, char *key, char *value);
int write_log(char *pathname, const char *format, ...);
int init_daemon();
