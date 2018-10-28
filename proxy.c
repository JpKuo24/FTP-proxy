#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include<fcntl.h>
#define MAX 255 /* Longest string */
#define CLOSE "CLOSE\r\n"
int main(int argc,  const char *argv[])
{
    fd_set master_set, working_set;
    struct timeval timeout;
    int proxy_cmd_socket    = 0;
    int accept_cmd_socket   = 0;
    int connect_cmd_socket  = 0;
    int proxy_data_socket   = 0;
    int accept_data_socket  = 0;
    int connect_data_socket = 0;
    int selectResult = 0;
    int select_sd = 20;
    static int FEXIST = 0;
    static int PASV;
    unsigned int ServSize;
    unsigned int ProxySize;
    int dataPort;
    int i;
    int j;
    int fd;
    char *a;
    char *b;
    char buff[MAX] = {0};
    FD_ZERO(&master_set);   //清空master_set集合
    bzero(&timeout, sizeof(timeout));

    proxy_cmd_socket = socket(PF_INET, SOCK_STREAM, 0);
    accept_cmd_socket = socket(PF_INET, SOCK_STREAM, 0);
    connect_cmd_socket = socket(PF_INET, SOCK_STREAM, 0);
    proxy_data_socket = socket(PF_INET, SOCK_STREAM, 0);
    accept_data_socket = socket(PF_INET, SOCK_STREAM, 0);
    connect_data_socket = socket(PF_INET, SOCK_STREAM, 0);


    proxy_cmd_socket = bindAndListenSocket(21);
    proxy_data_socket = bindAndListenSocket(5001);
    FD_SET(proxy_cmd_socket, &master_set);  //将proxy_cmd_socket加入master_set集合
    FD_SET(proxy_data_socket, &master_set);//将proxy_data_socket加入master_set集合

     timeout.tv_sec = 60;    //Select的超时结束时间
    timeout.tv_usec = 0;    //ms

    while(1)
    {
        FD_ZERO(&working_set); //清空working_set文件描述符集合
        memcpy(&working_set, &master_set, sizeof(master_set)); //将master_set集合copy到working_set集合
        selectResult = select(select_sd, &working_set, NULL, NULL, &timeout);

        if (selectResult < 0) {
            perror("select() failed\n");
            exit(1);
        }

        // timeout
        if (selectResult == 0) {
            printf("select() timed out.\n");
            timeout.tv_sec = 60;
            continue;
        }
        for (i = 0; i < select_sd; i++) {

            if (FD_ISSET(i, &working_set))
            {
               if (i == proxy_cmd_socket)
               {
                accept_cmd_socket = CmdfromClnt(proxy_cmd_socket);
                connect_cmd_socket = CmdtoServ();
                FD_SET(accept_cmd_socket, &master_set);
                FD_SET(connect_cmd_socket, &master_set);
               }
                if (i == accept_cmd_socket)
                {
                    if (read(i, buff, MAX) == 0)
                        {
                        //write(i,CLOSE,strlen(CLOSE));
                        //write(connect_cmd_socket,CLOSE,strlen(CLOSE));
                        close(i); //如果接收不到内容,则关闭Socket
                        close(connect_cmd_socket);
                        FD_CLR(i, &master_set);
                        FD_CLR(connect_cmd_socket, &master_set);
                        }
                        else
                        {
                            printf("client: %s",buff);
                            if(strncmp(buff,"PORTxx",4)==0)
                            {
                                a = strtok(buff," ");
                                for(j=0;j<3;j++)
                                {
                                    a = strtok(NULL,",");
                                    b = strtok(NULL,",");
                                }
                                int x = atoi(a);
                                int y = atoi(b);
                                dataPort = x*256+y;
                               PASV = 1;
                               strcpy(buff,"PORT 192,168,56,101,19,137\r\n");
                            }
                           if(strncmp(buff,"RETRxx",4)==0)
                            {
                                char name[255] = {0};
                                strcpy(name,buff);
                            a = strtok(name," ");
                            b = strtok(NULL,"\r");
                            printf("%s",b);
                            if((fd=open(b,O_RDWR,0))==-1)
                            {
                               if((fd = open(b,O_RDWR|O_CREAT,0))== -1)
                               {
                                   perror("open error");
                               }
                               FEXIST = 0;
                               printf("what the hell?");
                            }
                            }
                        write(connect_cmd_socket, buff, strlen(buff));
                        memset(buff,0,MAX);
                        }

                }
                /*if (i == accept_cmd_socket)
                {

                            read(i, buff, MAX);
                            printf("client: %s",buff);
                            if(strncmp(buff,"PORTxx",4)==0)
                            {
                                a = strtok(buff," ");
                                for(j=0;j<3;j++)
                                {
                                    a = strtok(NULL,",");
                                    b = strtok(NULL,",");
                                }
                                int x = atoi(a);
                                int y = atoi(b);
                                dataPort = x*256+y;


                               strcpy(buff,"PORT 192,168,56,101,19,137\r\n");
                            }
                        write(connect_cmd_socket, buff, MAX);

                        memset(buff,0,MAX);
                }*/
                if (i == connect_cmd_socket)
                {
                        if (read(i, buff, MAX) == 0)
                        {
                       //write(i,CLOSE,strlen(CLOSE));
                        //write(accept_cmd_socket,CLOSE,strlen(CLOSE));
                        close(i); //如果接收不到内容,则关闭Socket
                        close(accept_cmd_socket);
                        FD_CLR(i, &master_set);
                        FD_CLR(accept_cmd_socket, &master_set);
                        }

                        else
                        {
                            //printf("server: %s",buff);
                            if(strncmp(buff,"227xx",3)==0)
                            {
                                a = strtok(buff," ");
                                for(j=0;j<3;j++)
                                {
                                    a = strtok(NULL,",");
                                    b = strtok(NULL,",");
                                }
                                int x = atoi(a);
                                int y = atoi(b);
                                dataPort = x*256+y;
                               PASV = 0;
                               strcpy(buff,"227 Entering Passive Mode (192,168,56,101,19,137)\r\n");
                            }
                        write(accept_cmd_socket, ,buff, strlen(buff));
                        memset(buff,0,MAX);
                        }
                }
                /*if (i == connect_cmd_socket)
                {

                            read(i, buff, MAX);
                            printf("server: %s",buff);
                            if(strncmp(buff,"227xx",3)==0)
                            {
                                a = strtok(buff," ");
                                for(j=0;j<3;j++)
                                {
                                    a = strtok(NULL,",");
                                    b = strtok(NULL,",");
                                }
                                int x = atoi(a);
                                int y = atoi(b);
                                dataPort = x*256+y;

                               strcpy(buff,"227 Entering Passive Mode (192,168,56,101,19,137)\r\n");
                            }
                        write(accept_cmd_socket, buff, MAX);
                        memset(buff,0,MAX);
                }*/
                if (i == proxy_data_socket)
                {
                    //printf("%d",dataPort);

                    accept_data_socket = DatafromSrc(proxy_data_socket);
                    FD_SET(accept_data_socket, &master_set);
                    connect_data_socket = DatatoDest(dataPort);
                    FD_SET(connect_data_socket, &master_set);

                }

                if (i == accept_data_socket)
                {

                        if (read(i, buff, MAX) == 0)
                        {
                        close(fd);
                        //write(i,CLOSE,strlen(CLOSE));
                        //write(connect_data_socket,CLOSE,strlen(CLOSE));
                        close(i); //如果接收不到内容,则关闭Socket
                        close(connect_data_socket);
                        printf("herec ");
                        FD_CLR(i, &master_set);
                        FD_CLR(connect_data_socket, &master_set);
                        }
                        else
                        {
                        printf("client data: %s",buff);
                        write(connect_data_socket,buff,strlen(buff));
                        write(fd,buff,strlen(buff));
                        memset(buff,0,MAX);
                        }
                        //判断主被动和传输方式（上传、下载）决定如何传输数据
                }

                if (i == connect_data_socket)
                {
                        if (read(i, buff, MAX) == 0)
                        {
                        close(fd);
                        //write(i,CLOSE,strlen(CLOSE));
                        //write(accept_data_socket,CLOSE,strlen(CLOSE));
                        close(i); //如果接收不到内容,则关闭Socket
                        close(accept_data_socket);
                        printf("heres ");
                        FD_CLR(i, &master_set);
                        FD_CLR(accept_data_socket, &master_set);
                        }
                        else
                        {
                        printf("server data: %s",buff);
                        write(accept_data_socket,buff,strlen(buff));
                        write(fd,buff,strlen(buff));
                        memset(buff,0,MAX);
                        }
                    //判断主被动和传输方式（上传、下载）决定如何传输数据
                }

            }



        }

    }
}

int bindAndListenSocket(int portNUM)
{
    struct sockaddr_in ProxyAddr;
    memset(&ProxyAddr, 0, sizeof(ProxyAddr));
    ProxyAddr.sin_family = AF_INET; /* Internet addr family */
    ProxyAddr.sin_addr.s_addr = inet_addr("192.168.56.101");
    ProxyAddr.sin_port = htons(portNUM);
    int sock;
    if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("socket() failed.\n");
		exit(1);
	}
    if((bind(sock,(struct sockaddr *) &ProxyAddr, sizeof(ProxyAddr)))<0)
    {
       printf("bind() failed.\n");
       exit(1);
    }
    if((listen(sock,MAX))<0)
    {
        printf("listen() failed.\n");
        exit(1);
    }
    return sock;
}
int CmdfromClnt(int sockfd)
{
    struct sockaddr_in ClntAddr;
    memset(&ClntAddr, 0, sizeof(ClntAddr));
    unsigned int ClntSize;
    ClntSize = sizeof(ClntAddr);
    int sock;
    if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("socket() failed.\n");
		exit(1);
	}
    sock = accept(sockfd,(struct sockaddr *) &ClntAddr,&ClntSize);
    if(sock<0)
    {
        printf("The cmd connection between client and proxy is failed.\n");
        exit(1);
    }
return sock;
}
int CmdtoServ()
{
    struct sockaddr_in ServAddr;
    memset(&ServAddr, 0, sizeof(ServAddr));
    ServAddr.sin_family = AF_INET; /* Internet addr family */
    ServAddr.sin_addr.s_addr = inet_addr("192.168.56.1");
    ServAddr.sin_port = htons(21);
    int sock;
    if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("socket() failed.\n");
		exit(1);
	}
    if((connect(sock, (struct sockaddr *) &ServAddr, sizeof(ServAddr)))<0)
	{
		perror("The cmd connection between proxy and server is failed.\n");
		exit(1);
	}
	return sock;
}
int DatafromSrc(int sockfd)
{
    struct sockaddr_in SrcAddr;
    memset(&SrcAddr, 0, sizeof(SrcAddr));
    unsigned int SrcSize;
    SrcSize = sizeof(SrcAddr);
    int sock;
    if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("socket() failed.\n");
		exit(1);
	}
    sock = accept(sockfd,(struct sockaddr *) &SrcAddr,&SrcSize);
    if(sock<0)
    {
        printf("The data connection between source and proxy is failed.\n");
        exit(1);
    }
    return sock;
}
int DatatoDest(int portNum)
{_addr("192.168.56.1");
    DestAddr.sin_port = htons(portNum);
    int sock;
    if ((sock = socket(PF_INET, SO
    struct sockaddr_in DestAddr;
    memset(&DestAddr, 0, sizeof(DestAddr));
    DestAddr.sin_family = AF_INET; /* Internet addr family */
    DestAddr.sin_addr.s_addr = inetCK_STREAM, 0)) < 0)
	{
		printf("socket() failed.\n");
		exit(1);
	}
    if((connect(sock, (struct sockaddr *) &DestAddr, sizeof(DestAddr)))<0)
	{
		printf("The data connection between proxy and destination is failed.\n");
		exit(1);
	}
	return sock;
}
