#include "../1.common/common.h"
#include <pthread.h>
#include <sys/wait.h>
#include <sys/shm.h>

double dyanmic_parity = 0;
char script_info[6][MAXBUFF * 5];

struct Share_Mem {
    int share_cnt;
    pthread_mutex_t smutex;
    pthread_cond_t sready;
};

void client_heartbeat(char *ip, int port, struct Share_Mem *msg, char *logpath);
void recv_heartbeat(int port, struct Share_Mem *msg, char *logpath);
void send_data(int judgeport, int dataport,  char *ip, struct Share_Mem *msg, char *logpath);
int get_script_info(char *bsname, char *destfile, int cnt, int ind);
void Autoselftest(struct Share_Mem *msg, int cnt, char *logpath);

void client_heartbeat(char *ip, int port, struct Share_Mem *msg, char *logpath) {
    int delaytime = 10;
    printf("\n \033[32m 心跳进程已开启!\033[0m");
    while (1) {
        pthread_mutex_lock(&msg->smutex); 
        if (msg->share_cnt < 5) {
            pthread_mutex_unlock(&msg->smutex);
            printf("\033[31m 心跳进程中止!\033[0m");
            break;
        }
        pthread_mutex_unlock(&msg->smutex);
        int sockfd = socket_connect(ip, port);
        if (sockfd < 0) {
            close(sockfd);
            printf("\ntry once\n");
            write_log(logpath, "[pid = %d] [主动心跳进程] [error] : %s", getpid(), strerror(errno));
            sleep(delaytime);
            if (delaytime < 100) {
                delaytime += 10;
            }
            continue;
        }
        printf("\n \033[32m 心跳成功！\033[0m");
        write_log(logpath, "[pid = %d] [主动心跳进程] [success] !");
        close(sockfd);
        break;
    }
}

void recv_heartbeat(int port, struct Share_Mem *msg, char *logpath) {
    int listenfd = socket_create(port);
    if (listenfd < 0) {
        write_log(logpath, "[pid = %d] [接受心跳进程] [error] : %s", getpid(), strerror(errno));
        return ;
    }
    while (1) {
        int newfd = accept(listenfd, NULL, NULL);
        pthread_mutex_lock(&msg->smutex);
        msg->share_cnt = 0;
        pthread_mutex_unlock(&msg->smutex);
        if (newfd < 0) {
            write_log(logpath, "[pid = %d] [接受心跳进程] [waring] : %s", getpid(), strerror(errno));
            continue;
        }
        printf("\033[31m ❤ \033[0m ");
        fflush(stdout);
        close(newfd);
    }
    close(listenfd);
}

void send_data(int judgeport, int dataport, char *ip, struct Share_Mem *msg,  char *logpath) {
    int listenfd = socket_create(judgeport);
    if (listenfd < 0) {
        write_log(logpath, "[pid = %d] [数据交互进程] [error] : %s", getpid(), strerror(errno));
        return ;
    }

    while (1) {
        int newfd;
        if ((newfd = accept(listenfd, NULL, NULL)) < 0) {
            write_log(logpath, "[pid = %d] [数据交互进程] [waring] : %s", getpid(), strerror(errno));
            continue;
        }
        for (int i = 0; i < 6; i++) {
            int ack = 0, sign = 0;
            int recvnum = recv(newfd, &sign, sizeof(int), 0);
            if (recvnum <= 0) {
                write_log(logpath, "[pid = %d] [数据交互进程] [waring] : %s", getpid(), strerror(errno));
                close(newfd);
                continue;
            }
            char pathname[50] = {0};
            switch(sign) {
                case 101: {
                    //sprintf(pathname, "/opt/client/log/cpu.log");
                    sprintf(pathname, "./log/cpu.log");
                } break;
                case 102: {
                    //sprintf(pathname, "/opt/client/log/disk.log");
                    sprintf(pathname, "./log/disk.log");
                } break;
                case 103: {
                    //sprintf(pathname, "/opt/client/log/mem.log");
                    sprintf(pathname, "./log/mem.log");
                } break;
                case 104: {
                    //sprintf(pathname, "/opt/client/log/user.log");
                    sprintf(pathname, "./log/user.log");
                } break;
                case 105: {
                    //sprintf(pathname, "/opt/client/log/os.log");
                    sprintf(pathname, "./log/os.log");
                } break;
                case 106: {
                    //sprintf(pathname, "/opt/client/log/malic_process.log");
                    sprintf(pathname, "./log/malic_process.log");
                } break;
            }
            if (access(pathname, F_OK) < 0) {
                ack = 0;
                send(newfd, &ack, sizeof(int), 0);
                continue;
            }
            ack = 1;
            send(newfd, &ack, sizeof(int), 0);
            ack = 0;
            printf("ack = %d\n", ack);
            recvnum = recv(newfd, &ack, sizeof(int), 0);
            printf("recv : ack = %d\n", ack);
            if (recvnum < 0 || ack != 1) {
                write_log(logpath, "[pid = %d] [数据交互进程] [error] : %s", getpid(), strerror(errno));
                close(newfd);
                continue;
            }

            int sendfd = socket_connect(ip, dataport);
            FILE *rdfp = NULL;
            rdfp = fopen(pathname, "r");
            flock(rdfp->_fileno, LOCK_EX);
            char buff[MAXBUFF] = {0};
            while (1) {
                int num_read = fread(buff, 1, MAXBUFF, rdfp);
                if (send(sendfd, buff, num_read, 0) < 0) {
                    write_log(logpath, "[pid = %d] [数据交互进程] [error] : %s", getpid(), strerror(errno));
                    fclose(rdfp);
                    return ;
                }
                if (num_read == 0) break;
            }
            memset(buff, 0, MAXBUFF);
            memset(pathname, 0, sizeof(pathname));
            fclose(rdfp);
            close(sendfd);
        }
        pthread_mutex_lock(&msg->smutex);
        msg->share_cnt = 0;
        pthread_mutex_unlock(&msg->smutex);
        close(newfd);
    }
    close(listenfd);
    return ;
}

int get_script_info(char *bsname, char *destfile, int cnt, int ind) {
    FILE *pfile = NULL, *fp;
    char filename[100] = {0};
    sprintf(filename, "bash %s", bsname);
    pfile = popen(filename, "r");
    fp = fopen(destfile, "a+");
    if (!pfile) {
        fprintf(fp, "Error : Script run failed\n");
        return 0;
    }
    char buff[MAXBUFF] = {0};
    if (ind == 2) {
        if (fgets(buff, MAXBUFF, pfile) != NULL) {
            strcat(script_info[ind], buff);
        }
        if (fgets(buff, MAXBUFF, pfile) != NULL) {
            dyanmic_parity = atof(buff);
        }
    } else {
        while (fgets(buff, MAXBUFF, pfile) != NULL) {
            strcat(script_info[ind], buff);
        }    
    }
    if (cnt == 5) {
        fprintf(fp, "%s", script_info[ind]);
        memset(script_info[ind], 0, sizeof(script_info[ind]));
    }
    fclose(fp);
    pclose(pfile);
    return 1;
}

void Autoselftest(struct Share_Mem *msg, int cnt, char *logpath) {
    int flag;
    /*
    flag = get_script_info("/opt/client/script/cpuinfo.sh", "/opt/client/log/cpu.log", cnt, 0);
    flag &= get_script_info("/opt/client/script/diskinfo.sh", "/opt/client/log/disk.log", cnt, 1);
    char buff[50] = {0};
    sprintf(buff, "/opt/client/script/meminfo.sh %.2lf", dyanmic_parity);
    flag &= get_script_info(buff, "/opt/client/log/mem.log", cnt , 2);
    flag &= get_script_info("/opt/client/script/userinfo.sh", "/opt/client/log/user.log", cnt, 3);
    flag &= get_script_info("/opt/client/script/osinfo.sh", "/opt/client/log/os.log", cnt, 4);
    flag &= get_script_info("/opt/client/script/malic_process.sh", "/opt/client/log/malic_process.log", cnt, 5);
    */
    flag = get_script_info("./script/cpuinfo.sh", "./log/cpu.log", cnt, 0);
    flag &= get_script_info("./script/diskinfo.sh", "./log/disk.log", cnt, 1);
    char buff[50] = {0};
    sprintf(buff, "./script/meminfo.sh %.2lf", dyanmic_parity);
    flag &= get_script_info(buff, "./log/mem.log", cnt , 2);
    flag &= get_script_info("./script/userinfo.sh", "./log/user.log", cnt, 3);
    flag &= get_script_info("./script/osinfo.sh", "./log/os.log", cnt, 4);
    flag &= get_script_info("./script/malic_process.sh", "./log/malic_process.log", cnt, 5);

    if (flag == 0) {
        write_log(logpath, "[pid = %d] [用户自检进程] [error] : %s", getpid(), strerror(errno));
        return ;
    } else {
        printf("\033[32m OK \033[0m");
        fflush(stdout);
        write_log(logpath, "[pid = %d] [用户自检进程] [success] !");
    }
    sleep(2);
    if (msg->share_cnt >= 5) return ;
    pthread_mutex_lock(&msg->smutex);
    msg->share_cnt += 1;
    if (msg->share_cnt >= 5) {
        pthread_cond_signal(&msg->sready);   
    }
    pthread_mutex_unlock(&msg->smutex);
}

