#include "common.h"

int socket_create(int port) {
    int listenfd; //listen套接字
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(1);
    }
    
    int flag = 1, len = sizeof(int);
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &flag, len) < 0) {
        close(listenfd);
        perror("setsockopt");
        return -1;
    }
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(struct sockaddr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);
    if (bind(listenfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        exit(1);
    }
    if (listen(listenfd, 20) < 0) {
        perror("listen");
        exit(1);
    }
    return listenfd;
}

int socket_connect(char *ip, int port) {
    int sockfd;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket()");
        exit(1);
    }
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip);
    addr.sin_port = htons(port);
    socklen_t addr_len = sizeof(addr);
    if (connect(sockfd, (struct sockaddr *)&addr, addr_len) < 0) {
        perror("connect()");
        exit(1);
    }
    return sockfd;
}

int get_conf_value(char const *file, char *key, char *value) {
    FILE *fp = NULL;
    char *line = NULL, *substr = NULL;
    size_t n = 0, len = 0;
    ssize_t read;
    if (key == NULL || value == NULL) {
        printf("error\n");
        return -1;
    }
    fp = fopen(file, "r");
    if (fp == NULL) {
        printf("Open config file error!\n");
        return -1;
    }
    while ((read = getline(&line, &n, fp)) != -1) {
        substr = strstr(line, key);
        if (substr == NULL) continue;
        else {
            len = strlen(key);
            if (line[len] == '=') {
                strncpy(value, &line[len + 1], (int)read - len - 2);
                break;
            } else continue;          
        }
    }
    if (substr == NULL) {
        printf("%s Not found in %s!\n", key, file);
        fclose(fp);
        free(line);
        return -1;
    }
    fclose(fp);
    free(line);
    return 0;
}

int write_log(char *pathname, const char *format, ...) {
    FILE *fp = fopen(pathname, "a+");
    flock(fp->_fileno, F_OK);
    va_list arg;
    int done;
    va_start(arg, format);
    time_t time_now  = time(NULL);
    struct tm *curr_time = localtime(&time_now);
    fprintf(fp, "%d.%02d.%02d %02d:%02d:%02d ", (1900 + curr_time->tm_year), (1 + curr_time->tm_mon), curr_time->tm_mday, curr_time->tm_hour, curr_time->tm_min, curr_time->tm_sec);
    done = vfprintf(fp, format, arg);
    va_end(arg);
    fprintf(fp, "\n");
    fflush(fp);
    fclose(fp);
    return done;
}

int init_daemon() {
    int pid;
    
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGHUP, SIG_IGN);
    
    pid = fork();
    if (pid > 0) {
        exit(0);
    } else if (pid < 0) {
        return -1;
    }
    
    setsid();
    pid = fork();
    if (pid > 0) {
        exit(0);
    } else if (pid < 0) {
        return -1;
    }
    
    for (int i = 0; i < NOFILE; close(i++));

    chdir("/opt/");
    umask(0);
    signal(SIGCHLD, SIG_IGN);
    return 0;


}   
