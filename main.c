#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>                                                    
#include <arpa/inet.h>
#include <string.h>
#define BUFFSIZE 65535 /* Longest string */

int mode,action;
char buffer[BUFFSIZE] = {0};

int main(int argc, const char *argv[])
{
    fd_set master_set, working_set;  //文件描述符集合
    struct timeval timeout;          //select 参数中的超时结构体
    int proxy_cmd_socket    = 0;     //proxy listen控制连接
    int accept_cmd_socket   = 0;     //proxy accept客户端请求的控制连接
    int connect_cmd_socket  = 0;     //proxy connect服务器建立控制连接
    int proxy_data_socket   = 0;     //proxy listen数据连接
    int accept_data_socket  = 0;     //proxy accept得到请求的数据连接（主动模式时accept得到服务器数据连接的请求，被动模式时accept得到客户端数据连接的请求）
    int connect_data_socket = 0;     //proxy connect建立数据连接 （主动模式时connect客户端建立数据连接，被动模式时connect服务器端建立数据连接）
    int selectResult = 0;     //select函数返回值
    int select_sd = 10,Port,Port_data,canwriteA,canwriteC,j,i,x,y;    //select 函数监听的最大文件描述符
    
	char *a,*b;
    FD_ZERO(&master_set);   //清空master_set集合
    bzero(&timeout, sizeof(timeout));//全结构置零

	proxy_cmd_socket = socket(PF_INET, SOCK_STREAM, 0);
    accept_cmd_socket = socket(PF_INET, SOCK_STREAM, 0);
    connect_cmd_socket = socket(PF_INET, SOCK_STREAM, 0);
    proxy_data_socket = socket(PF_INET, SOCK_STREAM, 0);
    accept_data_socket = socket(PF_INET, SOCK_STREAM, 0);
    connect_data_socket = socket(PF_INET, SOCK_STREAM, 0);
	
    proxy_cmd_socket = bindAndListenSocket(21);  //开启proxy_cmd_socket、bind（）、listen操作
    FD_SET(proxy_cmd_socket, &master_set);  //将proxy_cmd_socket加入master_set集合
    
    timeout.tv_sec = 60;    //Select的超时结束时间
    timeout.tv_usec = 0;    //ms
    
    while (1) {
        FD_ZERO(&working_set); //清空working_set文件描述符集合
        memcpy(&working_set, &master_set, sizeof(master_set)); //将master_set集合copy到working_set集合
        
        //select循环监听 这里只对读操作的变化进行监听（working_set为监视读操作描述符所建立的集合）,第三和第四个参数的NULL代表不对写操作、和误操作进行监听
        selectResult = select(select_sd, &working_set, NULL, NULL, &timeout);
        
        // fail
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
        
        // selectResult > 0 时 开启循环判断有变化的文件描述符为哪个socket
        for (i = 0; i < select_sd; i++) {
            //判断变化的文件描述符是否存在于working_set集合
            if (FD_ISSET(i, &working_set)) {
                if (i == proxy_cmd_socket) {
                    accept_cmd_socket = acceptCmdSocket(proxy_cmd_socket);  //执行accept操作,建立proxy和客户端之间的控制连接
                    connect_cmd_socket = connectToServer(); //执行connect操作,建立proxy和服务器端之间的控制连接
                    printf("%d",accept_cmd_socket);
					printf("%d",connect_cmd_socket);
                    //将新得到的socket加入到master_set结合中
                    FD_SET(accept_cmd_socket, &master_set);
                    FD_SET(connect_cmd_socket, &master_set);
                }
                
                if (i == accept_cmd_socket) 
				{
                    char buff[BUFFSIZE] = {0};
					char buff1[BUFFSIZE] = {0};
                   
                    if (read(i, buff, BUFFSIZE) == 0) 
						{
                        close(i); //如果接收不到内容,则关闭Socket
                        close(connect_cmd_socket);
                        
                        //socket关闭后，使用FD_CLR将关闭的socket从master_set集合中移去,使得select函数不再监听关闭的socket
                        FD_CLR(i, &master_set);
                        FD_CLR(connect_cmd_socket, &master_set);

                        } 
					else 
						{
                        //如果接收到内容,则对内容进行必要的处理，之后发送给服务器端（写入connect_cmd_socket）

                        //处理客户端发给proxy的request，部分命令需要进行处理，如PORT、RETR、STOR                        
                        if(strstr(buff,"PORT")!=NULL)
                        	{
                               strtok(buff," ");
                                for(j=0;j<4;j++)
                                {
                                     strtok(NULL,",");
                                }
								a=strtok(NULL,",");
								b=strtok(NULL,",");
								int p1=atoi(a);
                                int p2=atoi(b);
								Port = p1*256+p2;//client端口
								Port_data=p1*256+p2;//proxy端口
								sprintf(buff," PORT 192,168,56,101,%d,%d/r/n",x,y);	
                        	}
						
                        //写入proxy与server建立的cmd连接,除了PORT之外，直接转发buff内容
                        write(connect_cmd_socket, buff, strlen(buff));
						if(strstr(buff,"PASV")!=NULL)
							mode=1;
                        else if(strstr(buff,"RETR")!=NULL)
                            action=1;
                        else if(strstr(buff,"STOR")!=NULL)
                            action=2;
                              
                    }
                }
                
                if (i == connect_cmd_socket) 
				{
					 char buff[BUFFSIZE] = {0};
					 char buff1[BUFFSIZE] = {0};
					if (read(i, buff, strlen(buff)) == 0)
						{
						  close(i); //如果接收不到内容,则关闭Socket
						  close(accept_cmd_socket);
						  FD_CLR(i, &master_set);
						  FD_CLR(accept_cmd_socket, &master_set);
					     }
					else 
						{
						strcpy(buff1,buff);
						printf("f*ck");
						printf("%s",buff);
						if(mode==1&&strstr(buff,"227"))
							{
							 strtok(buff," ");
                                for(j=0;j<4;j++)
                                {
                                  strtok(NULL,",");
                                }
								a=strtok(NULL,",");
								b=strtok(NULL,",");
								int p1=atoi(a);
                                int p2=atoi(b);
								Port= p1*256+p2;//server端口
								 Port_data=p1*256+p2;//proxy端口
							}
					     write(accept_cmd_socket,buff1, strlen(buff1));
					 
						proxy_data_socket = bindAndListenSocket(Port_data); //开启proxy_data_socket、bind（）、listen操作 
					   
						FD_SET(proxy_data_socket, &master_set);//将proxy_data_socket加入master_set集合
    
					}
					
                  //处理服务器端发给proxy的reply，写入accept_cmd_socket
                    
                  //PASV收到的端口 227 （port）
                  //////////////
                }
                
                if (i == proxy_data_socket) {
				 accept_data_socket = acceptCmdSocket(proxy_data_socket);
				 connect_data_socket = connectDataSocket(Port);
				 FD_SET(accept_data_socket, &master_set);
				 FD_SET(connect_data_socket, &master_set);
                }
                
                if (i == accept_data_socket) {
					if((action==1&&mode==0)||(action==2&&mode==1))/*下载，主动模式/上传，被动模式*/
						{
						if(read(accept_data_socket,buffer,strlen(buffer))==0)
							{
							close(i); //如果接收不到内容,则关闭Socket
                       	    close(accept_data_socket);
                        	FD_CLR(i, &master_set);
                        	FD_CLR(accept_data_socket, &master_set);
							canwriteA=1;
							}
						}
					if(((action==1&&mode==1)||(action==2&&mode==0))&&canwriteC==1)/*下载,被动模式*/
						{
						  write(accept_data_socket,buffer,strlen(buffer));
						  memset(buffer,0,BUFFSIZE);
						}
					
                    //判断主被动和传输方式（上传、下载）决定如何传输数据
                }
                
                if (i == connect_data_socket) {
					if(((action==1&&mode==0)||(action==2&&mode==1))&&canwriteA==1)/*下载*/
						{
						write(connect_data_socket,buffer,sizeof(buffer));
						 memset(buffer,0,BUFFSIZE);
						}
				    if((action==1&&mode==1)||(action==2&&mode==0))
						{
						if(read(connect_data_socket,buffer,strlen(buffer))==0)
							{
							close(i); //如果接收不到内容,则关闭Socket
                       	    close(connect_data_socket);
                        	FD_CLR(i, &master_set);
                        	FD_CLR(connect_data_socket, &master_set);
							canwriteC=1;
							}
						}
                    //判断主被动和传输方式（上传、下载）决定如何传输数据
                }
            }
        }
    }
    
    return 0;
}

int bindAndListenSocket(int portNum)
{
    struct sockaddr_in ProxyAddr;
	int sock;
    memset(&ProxyAddr, 0, sizeof(ProxyAddr));
    ProxyAddr.sin_family = AF_INET; /* Internet addr family */
    ProxyAddr.sin_addr.s_addr = inet_addr("192.168.56.102");
    ProxyAddr.sin_port = htons(portNum);
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
    if((listen(sock,BUFFSIZE))<0)
    {
        printf("listen() failed.\n");
        exit(1);
    }
    return sock;
}

int acceptCmdSocket(int fd)
{
    struct sockaddr_in ClntAddr;
	int sock;
    memset(&ClntAddr, 0, sizeof(ClntAddr));//将接收的连接实体的地址清零
    unsigned int ClntSize;
    ClntSize = sizeof(ClntAddr);
    if ((socket(PF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("socket() failed.\n");
		exit(1);
	}
    sock = accept(fd,(struct sockaddr *) &ClntAddr,&ClntSize);
    if(sock<0)
    {
        printf("The cmd connection is failed.\n");
        exit(1);
    }
return sock;
}

int connectToServer()
{
	int sock;
    struct sockaddr_in ServAddr;
    memset(&ServAddr, 0, sizeof(ServAddr));
    ServAddr.sin_family = AF_INET; /* Internet addr family */
    ServAddr.sin_addr.s_addr = inet_addr("192.168.56.1");
    ServAddr.sin_port = htons(21);
    if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("socket() failed.\n");
		exit(1);
	}
    if((connect(sock, (struct sockaddr *) &ServAddr, sizeof(ServAddr)))<0)
	{
		perror("The commend connection between proxy and server is failed.\n");
		exit(1);
	}
	return sock;
}

int connectDataSocket(int portNum)
{
		int sock;
		struct sockaddr_in DestAddr;
		memset(&DestAddr, 0, sizeof(DestAddr));
		DestAddr.sin_family = AF_INET; /* Internet addr family */
		DestAddr.sin_addr.s_addr = inet_addr("192.168.56.1");//client和server地址相同
		DestAddr.sin_port = htons(portNum);
		if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
		{
			printf("socket() failed.\n");
			exit(1);
		}
		if((connect(sock, (struct sockaddr *) &DestAddr, sizeof(DestAddr)))<0)
		{
			perror("The data connection is failed.\n");
			exit(1);
		}
		return sock;
}





