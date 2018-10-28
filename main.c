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
    fd_set master_set, working_set;  //�ļ�����������
    struct timeval timeout;          //select �����еĳ�ʱ�ṹ��
    int proxy_cmd_socket    = 0;     //proxy listen��������
    int accept_cmd_socket   = 0;     //proxy accept�ͻ�������Ŀ�������
    int connect_cmd_socket  = 0;     //proxy connect������������������
    int proxy_data_socket   = 0;     //proxy listen��������
    int accept_data_socket  = 0;     //proxy accept�õ�������������ӣ�����ģʽʱaccept�õ��������������ӵ����󣬱���ģʽʱaccept�õ��ͻ����������ӵ�����
    int connect_data_socket = 0;     //proxy connect������������ ������ģʽʱconnect�ͻ��˽����������ӣ�����ģʽʱconnect�������˽����������ӣ�
    int selectResult = 0;     //select��������ֵ
    int select_sd = 10,Port,Port_data,canwriteA,canwriteC,j,i,x,y;    //select ��������������ļ�������
    
	char *a,*b;
    FD_ZERO(&master_set);   //���master_set����
    bzero(&timeout, sizeof(timeout));//ȫ�ṹ����

	proxy_cmd_socket = socket(PF_INET, SOCK_STREAM, 0);
    accept_cmd_socket = socket(PF_INET, SOCK_STREAM, 0);
    connect_cmd_socket = socket(PF_INET, SOCK_STREAM, 0);
    proxy_data_socket = socket(PF_INET, SOCK_STREAM, 0);
    accept_data_socket = socket(PF_INET, SOCK_STREAM, 0);
    connect_data_socket = socket(PF_INET, SOCK_STREAM, 0);
	
    proxy_cmd_socket = bindAndListenSocket(21);  //����proxy_cmd_socket��bind������listen����
    FD_SET(proxy_cmd_socket, &master_set);  //��proxy_cmd_socket����master_set����
    
    timeout.tv_sec = 60;    //Select�ĳ�ʱ����ʱ��
    timeout.tv_usec = 0;    //ms
    
    while (1) {
        FD_ZERO(&working_set); //���working_set�ļ�����������
        memcpy(&working_set, &master_set, sizeof(master_set)); //��master_set����copy��working_set����
        
        //selectѭ������ ����ֻ�Զ������ı仯���м�����working_setΪ���Ӷ������������������ļ��ϣ�,�����͵��ĸ�������NULL������д����������������м���
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
        
        // selectResult > 0 ʱ ����ѭ���ж��б仯���ļ�������Ϊ�ĸ�socket
        for (i = 0; i < select_sd; i++) {
            //�жϱ仯���ļ��������Ƿ������working_set����
            if (FD_ISSET(i, &working_set)) {
                if (i == proxy_cmd_socket) {
                    accept_cmd_socket = acceptCmdSocket(proxy_cmd_socket);  //ִ��accept����,����proxy�Ϳͻ���֮��Ŀ�������
                    connect_cmd_socket = connectToServer(); //ִ��connect����,����proxy�ͷ�������֮��Ŀ�������
                    printf("%d",accept_cmd_socket);
					printf("%d",connect_cmd_socket);
                    //���µõ���socket���뵽master_set�����
                    FD_SET(accept_cmd_socket, &master_set);
                    FD_SET(connect_cmd_socket, &master_set);
                }
                
                if (i == accept_cmd_socket) 
				{
                    char buff[BUFFSIZE] = {0};
					char buff1[BUFFSIZE] = {0};
                   
                    if (read(i, buff, BUFFSIZE) == 0) 
						{
                        close(i); //������ղ�������,��ر�Socket
                        close(connect_cmd_socket);
                        
                        //socket�رպ�ʹ��FD_CLR���رյ�socket��master_set��������ȥ,ʹ��select�������ټ����رյ�socket
                        FD_CLR(i, &master_set);
                        FD_CLR(connect_cmd_socket, &master_set);

                        } 
					else 
						{
                        //������յ�����,������ݽ��б�Ҫ�Ĵ���֮���͸��������ˣ�д��connect_cmd_socket��

                        //����ͻ��˷���proxy��request������������Ҫ���д�����PORT��RETR��STOR                        
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
								Port = p1*256+p2;//client�˿�
								Port_data=p1*256+p2;//proxy�˿�
								sprintf(buff," PORT 192,168,56,101,%d,%d/r/n",x,y);	
                        	}
						
                        //д��proxy��server������cmd����,����PORT֮�⣬ֱ��ת��buff����
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
						  close(i); //������ղ�������,��ر�Socket
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
								Port= p1*256+p2;//server�˿�
								 Port_data=p1*256+p2;//proxy�˿�
							}
					     write(accept_cmd_socket,buff1, strlen(buff1));
					 
						proxy_data_socket = bindAndListenSocket(Port_data); //����proxy_data_socket��bind������listen���� 
					   
						FD_SET(proxy_data_socket, &master_set);//��proxy_data_socket����master_set����
    
					}
					
                  //����������˷���proxy��reply��д��accept_cmd_socket
                    
                  //PASV�յ��Ķ˿� 227 ��port��
                  //////////////
                }
                
                if (i == proxy_data_socket) {
				 accept_data_socket = acceptCmdSocket(proxy_data_socket);
				 connect_data_socket = connectDataSocket(Port);
				 FD_SET(accept_data_socket, &master_set);
				 FD_SET(connect_data_socket, &master_set);
                }
                
                if (i == accept_data_socket) {
					if((action==1&&mode==0)||(action==2&&mode==1))/*���أ�����ģʽ/�ϴ�������ģʽ*/
						{
						if(read(accept_data_socket,buffer,strlen(buffer))==0)
							{
							close(i); //������ղ�������,��ر�Socket
                       	    close(accept_data_socket);
                        	FD_CLR(i, &master_set);
                        	FD_CLR(accept_data_socket, &master_set);
							canwriteA=1;
							}
						}
					if(((action==1&&mode==1)||(action==2&&mode==0))&&canwriteC==1)/*����,����ģʽ*/
						{
						  write(accept_data_socket,buffer,strlen(buffer));
						  memset(buffer,0,BUFFSIZE);
						}
					
                    //�ж��������ʹ��䷽ʽ���ϴ������أ�������δ�������
                }
                
                if (i == connect_data_socket) {
					if(((action==1&&mode==0)||(action==2&&mode==1))&&canwriteA==1)/*����*/
						{
						write(connect_data_socket,buffer,sizeof(buffer));
						 memset(buffer,0,BUFFSIZE);
						}
				    if((action==1&&mode==1)||(action==2&&mode==0))
						{
						if(read(connect_data_socket,buffer,strlen(buffer))==0)
							{
							close(i); //������ղ�������,��ر�Socket
                       	    close(connect_data_socket);
                        	FD_CLR(i, &master_set);
                        	FD_CLR(connect_data_socket, &master_set);
							canwriteC=1;
							}
						}
                    //�ж��������ʹ��䷽ʽ���ϴ������أ�������δ�������
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
    memset(&ClntAddr, 0, sizeof(ClntAddr));//�����յ�����ʵ��ĵ�ַ����
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
		DestAddr.sin_addr.s_addr = inet_addr("192.168.56.1");//client��server��ַ��ͬ
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





