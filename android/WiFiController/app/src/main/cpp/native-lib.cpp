#include <jni.h>
#include <string>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE                1024

int sock;

void error_handler(char *msg);

#include "com_example_kwrg_wificontroller_WiFiController.h"

JNIEXPORT jstring JNICALL Java_com_example_kwrg_wificontroller_WiFiController_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "C/C++ This is Wi-Fi Controller";
    return env->NewStringUTF(hello.c_str());
}

JNIEXPORT void JNICALL Java_com_example_kwrg_wificontroller_WiFiController_WiFiConnect(JNIEnv *env, jobject){
    struct sockaddr_in serv_addr;

    char *ip = "192.168.25.8";

    /* Target IP: 192.168.25.8, Port: 7777
     * This have to be AM5728 DSP IP Address */
#if 0
    if(argc != 3)
    {
        printf("Usage: %s IP port\n", argv[0]);
        exit(1);
    }
#endif

    sock = socket(PF_INET, SOCK_STREAM, 0);
    if(sock == -1)
        error_handler("socket() error");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    //serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_addr.s_addr = inet_addr(ip);
    //serv_addr.sin_port = htons(atoi(argv[2]));
    serv_addr.sin_port = htons(7777);

    if(connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
        error_handler("connect() error!");
    else
        puts("Connected ......");
}

#if 1
JNIEXPORT jint JNICALL Java_com_example_kwrg_wificontroller_WiFiController_WiFiSendData(JNIEnv *env, jobject, jint no) {
    char msg[BUF_SIZE];
    int str_len, recv_len, recv_cnt;

    switch(no)
    {
        case 1:
            str_len = write(sock, no, strlen(no));
    }
    for (;;) {
        fputs("Input msg(q to quit): ", stdout);
        fgets(msg, BUF_SIZE, stdin);
        if (!strcmp(msg, "q\n") || !strcmp(msg, "Q\n"))
            break;

        str_len = write(sock, msg, strlen(msg));

        recv_len = 0;
        while (recv_len < str_len) {
            recv_cnt = read(sock, &msg[recv_len], BUF_SIZE - 1);
            if (recv_cnt == -1)
                error_handler("read() error!");
            recv_len += recv_cnt;
        }
        msg[recv_len] = 0;
        printf("msg from server: %s\n", msg);
        
    }
#if 0
    close(sock);

    return 0;
#endif
}
#endif

void error_handler(char *msg)
{
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}