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
#define BUFFERT 512//用于接收文件中字符串的缓冲区大小
#define BACKLOG 1    

int duration (struct timeval *start,struct timeval *stop, struct timeval *delta);//计算文件传输用时的函数
int create_server_socket (int port); /// 创建服务器端的套接字
struct sockaddr_in sock_serv,sock_clt;  //客户端和服务端的套接字
int main(int argc,char** argv){
    socklen_t len;   //套接字长度
    int sfd,fd,listenFd;
    long int n, m,count=0;
    unsigned int nsid;
    ushort clt_port; 
    char buffer[BUFFERT],filename[256];//暂存接受到的字符串    暂存生成的接收文件名
    char dst[INET_ADDRSTRLEN];//

    //时间变量
    time_t intps;
    struct tm* tmi;//time结构体
    
    if(argc!=2) {//如果参数的个数不为2
        perror("使用方法 ./server <监听端口号>\n");
        exit(1);
    }

    sfd = create_server_socket(atoi(argv[1]));//服务器套接字的创建,传入的参数为端口

    bzero(buffer,BUFFERT);//清空缓冲区等待接收字符串
    listenFd = listen(sfd,BACKLOG);//监听连接请求
	if ( listenFd == -1){//监听失败
	    perror("监听端口失败");
	    exit(1);
	}

	//与客户端建立
	len = sizeof(struct sockaddr_in);
	nsid=  accept(sfd, (struct sockaddr*)&sock_clt, &len);//建立连接,返回连接成功的描述符
	if(nsid == -1){
	    perror("与客户端建立连接失败");
	    exit(1);
	}
	else {
	    if(inet_ntop(AF_INET,&sock_clt.sin_addr,dst,INET_ADDRSTRLEN)==NULL){
	    //把地址从二进制转化成点分十进制
	    

	        perror("套接字错误");
	        exit (1);
	    }
	    clt_port=ntohs(sock_clt.sin_port);
	    //The ntohs() function converts the unsigned short integer netshort from network byte order to host byte order.
	    printf("与客户端成功建立连接 %s:%d\n",dst,clt_port);
		//打印建立的连接信息

//加上文件的时间戳
	    bzero(filename,256);//清空文件名缓冲区
	    intps = time(NULL);//获取当前时间 
	    tmi = localtime(&intps);// 格式化时间
	    sprintf(filename,"ClientPacket.%d.%d.%d-%d:%d:%d",1900+tmi->tm_year,tmi->tm_mon+1,tmi->tm_mday,tmi->tm_hour,tmi->tm_min,tmi->tm_sec);//字符串格式化拼接函数,用于连接文本
	    printf("生成文件: %s中...\n",filename);

	    if ((fd=open(filename,O_CREAT|O_WRONLY,0644))==-1)//创建并以只写方式打开文件
	    {
	        perror("打开服务器文件失败");
	        exit (1);
	    }
	    bzero(buffer,BUFFERT);//清空缓冲区等待接收字符串 

	    n=recv(nsid,buffer,BUFFERT,0);//接收定长文本,n为接收字节数,如果返回值为零表示到达文件末尾
	    while(n) {
	        if(n==-1){//读取失败
	            perror("接收失败");
	            exit(1);
	        }
	        if((m=write(fd, buffer, (size_t) n)) == -1){
	            perror("写入失败");
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

	printf("结束传输 %s:%d\n",dst,clt_port);
	printf("收到文件字节数 : %ld \n",count);
	
	
	sfd = create_server_socket(atoi(argv[1]));
	bzero(buffer,BUFFERT);
	listenFd = listen(sfd,BACKLOG);
	if ( listenFd == -1){
	    perror("监听失败");
	    exit(1);
	}

	//下面为接收report文件.为了保证正常的时序,在客户端发送完packet文件之后,睡眠五秒等待服务器进入监听模式
	
	//接收reoort文件和packet文件基本相似
	//建立与客户端的连接
	len = sizeof(struct sockaddr_in);
	nsid=  accept(sfd, (struct sockaddr*)&sock_clt, &len);//
	if(nsid == -1){
	    perror("建立连接套接字失败");
	    exit(1);
	}
	else {
	    if(inet_ntop(AF_INET,&sock_clt.sin_addr,dst,INET_ADDRSTRLEN)==NULL){
	        perror("连接套接字错误");
	        exit (1);
	    }
	    clt_port=ntohs(sock_clt.sin_port);
	    printf("与客户端连接开始 : %s:%d\n",dst,clt_port);

	    //将文件名加上时间戳
	    bzero(filename,256);
	    intps = time(NULL);//现在的时间
	    tmi = localtime(&intps);//时间格式化
	    bzero(filename,256);//清除缓冲区
	    sprintf(filename,"ClientReport.%d.%d.%d-%d:%d:%d",1900+tmi->tm_year,tmi->tm_mon+1,tmi->tm_mday,tmi->tm_hour,tmi->tm_min,tmi->tm_sec);
	    printf("创建文件中  %s ...\n",filename);

	    if ((fd=open(filename,O_CREAT|O_WRONLY,0644))==-1)
	    {
	        perror("open fail");
	        exit (3);
	    }
	    bzero(buffer,BUFFERT);

	    n=recv(nsid,buffer,BUFFERT,0);
	    while(n) {
	        if(n==-1){
	            perror("服务器文件接受失败");
	            exit(5);
	        }
	        if((m=write(fd, buffer, (size_t) n)) == -1){
	            perror("服务器文件写入失败");
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

	printf("结束与客户端文件传输: %s:%d\n",dst,clt_port);
	printf("收到文件字节数: %ld \n",count);
	
    return 0;
}


int create_server_socket (int port){
    int sfd;
    int yes=1;
    sfd = socket(PF_INET,SOCK_STREAM,0);
    if (sfd == -1){
        perror("创建服务器套接字失败");
        exit(1);
    }
	/*设置套接字选项,套接字级别,允许用于bind重用本地地址,*/
    if(setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR,&yes,sizeof(int)) == -1 ) {
        perror("设置套接字选项失败");
        exit(1);
    }

    bzero(&sock_serv,sizeof(struct sockaddr_in));
    sock_serv.sin_family=AF_INET;//协议簇
    sock_serv.sin_port=htons((uint16_t) port);//端口,主机字节序(short int)转化为网络字节序
   
    sock_serv.sin_addr.s_addr=htonl(INADDR_ANY);
    //主机字节序(long int)转化为网络字节序

    if(bind(sfd,(struct sockaddr*)&sock_serv,sizeof(struct sockaddr_in))==-1){
        perror("服务器地址绑定成败");
        exit(1);
    }
    return sfd;
}
