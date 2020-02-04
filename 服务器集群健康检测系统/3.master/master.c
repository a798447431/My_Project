#include "master.h"

int main() {
    //char *config = "/opt/master/pihealthd_master.conf";
    char *config = "./pihealthd_master.conf";
    char tmp[20] = {0};
    char FromIp[20] = {0};
    char ToIp[20] = {0};
    char logpath[50] = {0};
    int master_listen;
    int Ins, HeartPort, JudgePort, DataPort, AddUserPort, MaxClient;
    long TimeOut;
   
    pthread_t HeartTid, DataTid[Ins];
    
//  init_daemon();

    get_conf_value(config, "From", tmp);
    strcpy(FromIp, tmp);
    memset(tmp, 0, sizeof(tmp));
    printf("FromIp = %s\n", FromIp);
    
    get_conf_value(config, "To", tmp);
    strcpy(ToIp, tmp);
    memset(tmp, 0, sizeof(tmp));
    printf("ToIp = %s\n", ToIp);
    
    get_conf_value(config, "LogPath", tmp);
    strcpy(logpath, tmp);
    memset(tmp, 0, sizeof(tmp));
    printf("LogPath = %s\n", logpath);
    
    get_conf_value(config, "INS", tmp);
    Ins = atoi(tmp);
    memset(tmp, 0, sizeof(tmp));
    printf("Ins = %d\n", Ins);

    get_conf_value(config, "HeartPort", tmp);
    HeartPort = atoi(tmp);
    memset(tmp, 0, sizeof(tmp));
    printf("HeartPort = %d\n", HeartPort);
    
    get_conf_value(config, "JudgePort", tmp);
    JudgePort = atoi(tmp);
    memset(tmp, 0, sizeof(tmp));
    printf("JudgePort = %d\n", JudgePort);
    
    get_conf_value(config, "DataPort", tmp);
    DataPort = atoi(tmp);
    memset(tmp, 0, sizeof(tmp));
    printf("DataPort = %d\n", DataPort);
    
    get_conf_value(config, "AddUserPort", tmp);
    AddUserPort = atoi(tmp);
    memset(tmp, 0, sizeof(tmp));
    printf("AddUserPort = %d\n", AddUserPort);

    get_conf_value(config, "TimeOut", tmp);
    TimeOut = atoi(tmp);
    memset(tmp, 0, sizeof(tmp));
    printf("TimeOut = %ld\n", TimeOut);
   
    get_conf_value(config, "MAXCLIENT", tmp);
    MaxClient = atoi(tmp);
    memset(tmp, 0, sizeof(tmp));
    printf("MaxClient = %d\n", MaxClient);
    
    unsigned int begin, end;
    begin = ntohl(inet_addr(FromIp));
    end = ntohl(inet_addr(ToIp));
    LinkedList *linkedlist = (LinkedList *)malloc(sizeof(LinkedList) * Ins);
    int *sum = (int *)malloc(sizeof(int) * Ins);
    memset(sum, 0, sizeof(int) * Ins);

    struct sockaddr_in initaddr;
    initaddr.sin_family = AF_INET;
    initaddr.sin_addr.s_addr = inet_addr("0.0.0.0");
    initaddr.sin_port = htons(HeartPort);
    for (int i = 0; i < Ins; i++) {
        Node *p = (Node *)malloc(sizeof(Node));
        p->addr = initaddr;
        p->sockfd = -1;
        p->next = NULL;
        linkedlist[i] = p;
    }
    
    for (unsigned int i = begin; i <= end; i++) {
        if ((i % 256 == 0) || ((i + 1) % 256 == 0)) {
            continue;
        }
        initaddr.sin_addr.s_addr = htonl(i);
        Node *p = (Node *)malloc(sizeof(Node));
        p->addr = initaddr;
        p->sockfd = -1;
        p->next = NULL;
        int sub = find_min(sum, Ins);
        insert(linkedlist[sub], p);
        sum[sub]++;
    }
    
    for (int i = 0; i < Ins; i++) {
        printf("Ins <%d> :\n", i);
        print_linkedlist(linkedlist[i]);
    }
    
    struct HEART heartarg;
    heartarg.head = linkedlist;
    heartarg.sum = sum;
    heartarg.ins = Ins;
    heartarg.timeout = TimeOut;
    
    pthread_create(&HeartTid, NULL, heartbeat_all, (void *)&heartarg);
    
    sleep(5);
    
    struct DATA_TRANSFER dataarg[Ins];
    
    for (int i = 0; i < Ins; i++) {
        dataarg[i].head = linkedlist[i];
        dataarg[i].judgeport = JudgePort;
        dataarg[i].dataport = DataPort;
        dataarg[i].index = Ins;
        dataarg[i].maxclient = MaxClient;
        if (pthread_create(&DataTid[i], NULL, data_transfer, (void *)&dataarg[i]) < 0) {
            write_log(logpath, "[pid = %d] [数据交互线程] [error] : %s", getpid(), strerror(errno));
            exit(1);
        }
    }

    if ((master_listen = socket_create(AddUserPort)) < 0) {
        exit(1);
    }
   
    add_user_epoll(master_listen, linkedlist, sum, Ins, HeartPort, MaxClient);

    for (int i = 0; i < Ins; i++) {
        pthread_join(DataTid[i], NULL);
    }

    pthread_join(HeartTid, NULL);

    return 0;
}

