#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <strings.h>
#define BUFFERT 512//用于接收文件中字符串的缓冲区
/*Size of the customer queue */
#define BACKLOG 1    

int duration (struct timeval *start,struct timeval *stop, struct timeval *delta);//计算文件传输用时的函数
int create_server_socket (int port); /// 创建服务器端的套接字

struct sockaddr_in sock_serv,sock_clt;  //客户端和服务端的套接字


int main(int argc,char** argv){
    socklen_t len;   //长度
    int sfd,fd,listenFd;
    long int n, m,count=0;
    unsigned int nsid;
    ushort clt_port; 
    char buffer[BUFFERT],filename[256];
    char dst[INET_ADDRSTRLEN];

    //Variable for the date
    time_t intps;
    struct tm* tmi;
    
//    struct tm {
//               int tm_sec;    /* Seconds (0-60) */
//               int tm_min;    /* Minutes (0-59) */
//               int tm_hour;   /* Hours (0-23) */
//               int tm_mday;   /* Day of the month (1-31) */
//               int tm_mon;    /* Month (0-11) */
//               int tm_year;   /* Year - 1900 */
//               int tm_wday;   /* Day of the week (0-6, Sunday = 0) */
//               int tm_yday;   /* Day in the year (0-365, 1 Jan = 0) */
//               int tm_isdst;  /* Daylight saving time */
//           };

    if(argc!=2) {//如果参数的个数不为2
        perror("Usage ./a.out <port>\n");
        exit(1);
    }

    sfd = create_server_socket(atoi(argv[1]));//服务器套接字的创建,传入的参数为端口

    bzero(buffer,BUFFERT);//清空缓冲区等待接收字符串
    listenFd = listen(sfd,BACKLOG);//监听连接请求
		if ( listenFd == -1){//监听失败
		    perror("Listen failed");
		    exit(1);
		}

		//connect client
		len = sizeof(struct sockaddr_in);
		nsid=  accept(sfd, (struct sockaddr*)&sock_clt, &len);//建立连接,返回连接成功的描述符
		//int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);参数类型为*的应使用指针类型
		if(nsid == -1){
		    perror("accept fail");
		    exit(1);
		}
		else {
		    if(inet_ntop(AF_INET,&sock_clt.sin_addr,dst,INET_ADDRSTRLEN)==NULL){
		    //inet_ntop - convert IPv4 and IPv6 addresses from binary to text form
		    //把地址从二进制转化成文本格式
		    //原型为const char *inet_ntop(int af, const void *src, char *dst, socklen_t size);

		        perror("error socket");
		        exit (1);
		    }
		    clt_port=ntohs(sock_clt.sin_port);
		    //The ntohs() function converts the unsigned short integer netshort from network byte order to host byte order.
		    printf("Start of the connection for : %s:%d\n",dst,clt_port);
			//打印建立的连接信息
		    //Processing the file name with the date
		    bzero(filename,256);
		    intps = time(NULL);//* Get the current time 
		    tmi = localtime(&intps);// 格式化时间 Format and print the time, "ddd yyyy-mm-dd hh:mm:ss zzz" 
		    bzero(filename,256);
		    sprintf(filename,"ClientPacket.%d.%d.%d-%d:%d:%d",1900+tmi->tm_year,tmi->tm_mon+1,tmi->tm_mday,tmi->tm_hour,tmi->tm_min,tmi->tm_sec);//字符串格式化拼接函数,用于连接文本
		    printf("Creating the Packet file : %s\n",filename);

		    if ((fd=open(filename,O_CREAT|O_WRONLY,0644))==-1)//创建并以只写方式打开文件
		    {
		        perror("open fail");
		        exit (1);
		    }
		    bzero(buffer,BUFFERT);

		    n=recv(nsid,buffer,BUFFERT,0);//接收定长文本,n为接收字节数,如果为零表示到达文件末尾
		    /*
		     The only difference between recv() and read(2) is the presence of
       flags.  With a zero flags argument, recv() is generally equivalent to
       read(2) (but see NOTES).  Also, the following call

           recv(sockfd, buf, len, flags);

       is equivalent to

           recvfrom(sockfd, buf, len, flags, NULL, NULL);
		    */
		    while(n) {
		        if(n==-1){//读取失败
		            perror("recv failed");
		            exit(1);
		        }
		        if((m=write(fd, buffer, (size_t) n)) == -1){
		            perror("write failed");
		            exit (1);
		        }
		        count=count+m;
		        bzero(buffer,BUFFERT);
		        n=recv(nsid,buffer,BUFFERT,0);
		        
		    }
		    close(sfd);
		    close(fd);
		}
		close(nsid);

		printf("End of transmission with %s:%d\n",dst,clt_port);
		printf("Number of bytes received : %ld \n",count);
		
		
		sfd = create_server_socket(atoi(argv[1]));
		bzero(buffer,BUFFERT);
		//printf("new fd = %d",sfd);
    listenFd = listen(sfd,BACKLOG);
		if ( listenFd == -1){
		    perror("Listen failed");
		    exit(1);
		}

		//下面为接收report文件.为了保证正常的时序,在客户端发送完packet文件之后,睡眠五秒等待服务器进入监听模式
		
		//接收reoort文件和packet文件基本相似
		//connect client
		len = sizeof(struct sockaddr_in);
		nsid=  accept(sfd, (struct sockaddr*)&sock_clt, &len);//
		if(nsid == -1){
		    perror("accept fail");
		    exit(1);
		}
		else {
		    if(inet_ntop(AF_INET,&sock_clt.sin_addr,dst,INET_ADDRSTRLEN)==NULL){
		        perror("error socket");
		        exit (1);
		    }
		    clt_port=ntohs(sock_clt.sin_port);
		    printf("Start of the connection for : %s:%d\n",dst,clt_port);

		    //Processing the file name with the date
		    bzero(filename,256);
		    intps = time(NULL);
		    tmi = localtime(&intps);
		    bzero(filename,256);
		    sprintf(filename,"ClientReport.%d.%d.%d-%d:%d:%d",1900+tmi->tm_year,tmi->tm_mon+1,tmi->tm_mday,tmi->tm_hour,tmi->tm_min,tmi->tm_sec);
		    printf("Creating file  %s \n ...",filename);

		    if ((fd=open(filename,O_CREAT|O_WRONLY,0644))==-1)
		    {
		        perror("open fail");
		        exit (3);
		    }
		    bzero(buffer,BUFFERT);

		    n=recv(nsid,buffer,BUFFERT,0);
		    while(n) {
		        if(n==-1){
		            perror("recv failed");
		            exit(5);
		        }
		        if((m=write(fd, buffer, (size_t) n)) == -1){
		            perror("write failed");
		            exit (6);
		        }
		        count=count+m;
		        bzero(buffer,BUFFERT);
		        n=recv(nsid,buffer,BUFFERT,0);
		    }
		    close(sfd);
		    close(fd);
		}
		close(nsid);

		printf("End of transmission with %s:%d\n",dst,clt_port);
		printf("Number of bytes received : %ld \n",count);
		
    return 0;
}


int create_server_socket (int port){
    int sfd;
    int yes=1;
    sfd = socket(PF_INET,SOCK_STREAM,0);
    if (sfd == -1){
        perror("create socket failed");
        exit(1);
    }
	/*setsockopt - set the socket options
	
	int setsockopt(int socket, int level, int option_name,const void *option_value, socklen_t option_len);
	
	To set options at the socket level, specify the level argument as SOL_SOCKET.
	SO_REUSEADDR
Specifies that the rules used in validating addresses supplied to bind() should allow reuse of local addresses, if this is supported by the protocol. This option takes an int value. This is a Boolean option.
	The setsockopt() function shall set the option specified by the option_name argument, at the protocol level specified by 		the level argument, to the value pointed to by the option_value argument for the socket associated with the file 		descriptor 	specified by the socket argument.
	Upon successful completion, setsockopt() shall return 0. Otherwise, -1 shall be returned and errno set to indicate the error.

	设置套接字选项,套接字级别,允许用于bind重用本地地址,*/
    if(setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR,&yes,sizeof(int)) == -1 ) {
        perror("set sockopt error");
        exit(1);
    }

    bzero(&sock_serv,sizeof(struct sockaddr_in));
    sock_serv.sin_family=AF_INET;//协议簇
    sock_serv.sin_port=htons((uint16_t) port);//端口,主机字节序(short int)转化为网络字节序
   // The ntohs() function converts the unsigned short integer netshort from network byte order to host byte order.
    sock_serv.sin_addr.s_addr=htonl(INADDR_ANY);
    //主机字节序(long int)转化为网络字节序The htonl() function converts the unsigned integer hostlong from host byte order to network byte order.
    //INADDR_ANY is used when you don't need to bind a socket to a specific IP. When you use this value as the address when calling bind() , the socket accepts connections to all the IPs of the machine.

    if(bind(sfd,(struct sockaddr*)&sock_serv,sizeof(struct sockaddr_in))==-1){
        perror("bind fail");
        exit(1);
    }
    return sfd;
}
