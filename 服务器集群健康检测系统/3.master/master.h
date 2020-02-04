#include "../1.common/common.h"
#include <pthread.h>
#include <time.h>
#include <sys/ioctl.h>
#include <sys/epoll.h>
#include <sys/file.h>
#include <sys/stat.h>

typedef struct Node {
    struct sockaddr_in addr;
    int sockfd;
    struct Node *next;
}Node, *LinkedList;

struct HEART {
    LinkedList *head;
    int *sum;
    int ins;
    long timeout;
};

struct DATA_TRANSFER {
    LinkedList head;
    int judgeport;
    int dataport;
    int index; //用户ip日志
    int maxclient;
};

int insert(LinkedList head, Node *node);
int find_min(int *sum, int ins);
void *heartbeat_all(void *arg);
void *data_transfer(void *arg);
void print_linkedlist(LinkedList head);
int check_connect(struct sockaddr_in addr, long timeout);
int add_user_epoll(int listenfd, LinkedList *linkedlist, int *sum, int ins, int heartport, int maxclient);


int insert(LinkedList head, Node *node) {
    Node *p = head;
    while (p->next != NULL) {
        p = p->next;
    }
    p->next = node;
    return 0;
}

int find_min(int *sum, int ins) {
    int sub = 0;
    for (int i = 0; i < ins; i++) {
        if (*(sum + i) < *(sum + sub)) {
            sub = i;
        }
    }
    return sub;
}

void print_linkedlist(LinkedList head) {
    Node *p = head;
    int cnt = 0;
    while (p->next != NULL) {
        printf("<%d> %s\n", cnt++, inet_ntoa(p->addr.sin_addr));
        p = p->next;
    }
}

int check_connect(struct sockaddr_in addr, long timeout) {
    int sockfd;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return 0;
    }
    int error = -1, ret = 0;
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = timeout;
    socklen_t len = sizeof(int);
    unsigned long ul = 1;
    ioctl(sockfd, FIONBIO, &ul);
    fd_set set;
    FD_ZERO(&set);
    FD_SET(sockfd, &set);

    if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        if (select(sockfd + 1, NULL, &set, NULL, &tv) > 0) {
            getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len);
            if (error == 0) {
                ret = 1;
            } else {
                ret = 0;
            }
        }
    }
    close(sockfd);
    return ret;
}

void *heartbeat_all(void *arg) {
    struct HEART *heartarg = (struct HEART *)arg;
    while (1) {
        for (int i = 0; i < heartarg->ins; i++) {
            Node *p = heartarg->head[i];
            while (p->next != NULL) {
                if (!check_connect(p->next->addr, heartarg->timeout)) {
                    printf("%s:%d \033[31mdeleting\033[m ... \n", inet_ntoa(p->next->addr.sin_addr), ntohs(p->next->addr.sin_port));
                    Node *q = p->next;
                    p->next = p->next->next;
                    free(q);
                    heartarg->sum[i]--;
                } else { 
                    printf("%s:%d \033[32monline\033[m ... \n", inet_ntoa(p->next->addr.sin_addr), ntohs(p->next->addr.sin_port));
                    p = p->next;
                }
            }
        }
        sleep(5);
    }
    return NULL;
}

void *data_transfer(void *arg) {
    struct DATA_TRANSFER *inarg = (struct DATA_TRANSFER *)arg;
    char log[50] = {0};
    sprintf(log, "./%d.log", inarg->index);
    char script_name[6][50] = {"cpu.log", "disk.log", "mem.log", "userinfo.log", "osinfo.log", "malic_process.log"};
    
    while (1) {
        sleep(20);
        FILE *file = fopen(log, "w");
        Node *p = inarg->head;
        while (p->next != NULL) {
            fprintf(file, "%s:%d\n", inet_ntoa(p->next->addr.sin_addr), ntohs(p->next->addr.sin_port));
            int sockfd;
            if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                perror("socket()");
                return NULL;
            }    
            struct sockaddr_in naddr;
            naddr.sin_family = AF_INET;
            naddr.sin_addr = p->next->addr.sin_addr;
            naddr.sin_port = htons(inarg->judgeport);
            
            if (connect(sockfd, (struct sockaddr *)&naddr, sizeof(naddr)) < 0) {
                perror("send connect()");
                close(sockfd);
                continue;
            }

            for (int i = 101; i <= 106; i++) {
                send(sockfd, &i, sizeof(int), 0);
                int ack = 0;
                int recvnum = recv(sockfd, &ack, sizeof(int), 0);
                printf("ack = %d\n", ack);
                if (recvnum <= 0) {
                    perror("ack recv()");
                    continue;
                }
                
                if (ack == 0) continue;
                
                int recvfd = socket_create(inarg->dataport);
                if (recvfd < 0) {
                    perror("recv socket_create()");
                    break;
                }
                send(sockfd, &ack, sizeof(int), 0);
                
                struct sockaddr_in rnaddr;
                socklen_t len = sizeof(int);
                int newfd = accept(recvfd, (struct sockaddr *)&rnaddr, &len);
                if (newfd < 0) {
                    perror("accept");
                    close(recvfd);
                    continue;
                }
                char dirname[50] = {0};
                char filename[50] = {0};
                sprintf(dirname, "%s", inet_ntoa(p->next->addr.sin_addr));
                printf("%s\n", inet_ntoa(p->next->addr.sin_addr));
                if (access(dirname, F_OK) < 0) {
                    mkdir(dirname, 0755); 
                } 
                char buff[MAXBUFF] = {0};
                sprintf(filename, "./%s/%s", dirname, script_name[i - 101]);
                FILE *rcfp = fopen(filename, "a+");
                while (1) {
                    int recvnum2 =  recv(newfd, buff, MAXBUFF, 0);
                    if (recvnum2 <= 0) break;
                    fprintf(rcfp, "%s", buff);
                    memset(buff, 0, MAXBUFF);
                }
                fclose(rcfp);
                close(newfd); 
                close(recvfd);
            }
            close(sockfd);
            p = p->next;        
        }
        fclose(file);
        sleep(5);       
    }
    return NULL;
}

int add_user_epoll(int listenfd, LinkedList *linkedlist, int *sum, int ins, int heartport, int maxclient) {
    unsigned int ul = 1;
    ioctl(listenfd, FIONBIO, &ul);
    struct epoll_event ev, events[maxclient];
    int epollfd = epoll_create(maxclient);
    if (epollfd < 0) {
        perror("epoll_create()");
        return -1;
    }
    ev.data.fd = listenfd;
    ev.events = EPOLLIN;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &ev) < 0) {
        perror("epoll_ctl()");
        return -1;
    }

    while (1) {
        int nfds = epoll_wait(epollfd, events, maxclient, -1);
        if (nfds < 0) {
            perror("epoll_wait()");
            return -1;
        } else if(nfds == 0) {
            continue;
        }

        for (int i = 0; i < nfds; i++) {
            if (events[i].data.fd == listenfd && events[i].events & EPOLLIN) {
                struct sockaddr_in newaddr;
                socklen_t len = sizeof(newaddr);
                int newsock = accept(listenfd, (struct sockaddr *)&newaddr, &len);
                if (newsock < 0) {
                    perror("accept()");
                    return -1;
                }
                int sub = find_min(sum, ins);
                Node *p = (Node *)malloc(sizeof(Node));
                newaddr.sin_port = htons(heartport);
                p->addr = newaddr;
                p->sockfd = newsock;
                p->next = NULL;
                insert(linkedlist[sub], p);
                printf("insert %s:%d is success!\n", inet_ntoa(newaddr.sin_addr), htons(newaddr.sin_port));
                sum[sub]++;
            }
        }
    }
}
