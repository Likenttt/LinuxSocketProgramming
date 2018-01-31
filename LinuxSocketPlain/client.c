#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <unistd.h>   
#include <fcntl.h>   
#include <sys/wait.h>
#include <pthread.h>
#define BUFFERT 512
#define ETH_NAME   "ens33"//"ens33"//此处为网卡的名称,一般的ubuntu虚拟机为ens33,可以使用ifconfig查看.我的网卡为enp2s0f1
static int N_Line_Packet = 1;//packet的行号
static int N_wrt_P2 = 0;//udp写入行数
static int N_rev_P2 = 0;//udp接收行数
static int N_wrt_P1 = 0;//tcp写入行数
static int N_rev_P1 = 0;//tcp接受行数
char packet_file_name[15] = "Packet.txt";
char report_file_name[15] = "Report.txt";
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;//线程互斥信号量
char *ipAddress = NULL;//服务器ip地址
int serverPort;//服务器端口
static int i= 1;//用于控制两个线程的循环
int fd1,fd2;//分别是packet和report文件描述符

//设置网卡混杂模式
void Set_Promiscuous_Mode(char *interface, int sock) {
	struct ifreq ifr;//可参考linux教材socket编程例程
	strncpy(ifr.ifr_name, interface, strnlen(interface, sizeof(interface)) + 1);
	if (ioctl(sock, SIOCGIFFLAGS, &ifr) == -1) {
		perror("设置混杂模式失败\n");
		exit(1);
	}
	ifr.ifr_flags |= IFF_PROMISC;
	if (ioctl(sock, SIOCGIFFLAGS, &ifr) == -1) {
		perror("无法设置混杂模式标志");
		exit(1);
	}
}
//写入一行文件
void writeOneLineIntoFile(int fd, void *buffer, size_t size) {
	ssize_t bytes_writeTcp = write(fd, buffer, size);
	if (bytes_writeTcp != size) {
		perror("一行数据写入失败");
		exit(1);
	}
}
//分析报表写入report文件
void writeReport(int fd) {
	char bufferReport[300];
	sprintf(bufferReport,
			"Packet文件行数:%d\nTcp统计:\n接收到的Tcp头部数目:%d\n写入的tcp头部数目:%d\n百分比:%lf\nUDP统计:\n接收到的Udp头部数目:%d\n写入的udp头部数目:%d\n百分比:%lf\n",
			N_Line_Packet - 1, N_rev_P1, N_wrt_P1, ((double) N_rev_P1 / N_wrt_P1), N_rev_P2, N_wrt_P2,
			((double) N_rev_P2 / N_wrt_P2));
	writeOneLineIntoFile(fd, bufferReport, strlen(bufferReport));
}
void *TcpThreadExe() {
	int sock;
	int rc;
	char buffer[200];//ip数据包的缓冲区
	char bufferForOneLineTcpPrint[40];//一行文本的缓冲区
	int bytes_received;//接受字节数
	size_t sourceLen;//地址长度
	struct sockaddr_in source; //本地地址
	struct tcphdr *tcp;//tcp头部结构体
	if ((sock = socket(AF_INET, SOCK_RAW, IPPROTO_TCP)) == -1) {
		perror("创建tcp原始套接字失败");
		exit(1);
	}
	Set_Promiscuous_Mode(ETH_NAME, sock);//混杂模式,抓取一切数据
	printf("Tcp抓包开始... \n");
	while (i) {
		sourceLen = sizeof(source);
		bytes_received = (int) recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr *) &source,
										(socklen_t *) &sourceLen);
		if (bytes_received < 0) {
			perror("接收数据失败.");
			exit(-1);
		}
		N_rev_P1++;//tcp read 
		tcp = (struct tcphdr *) (buffer + 20);
		printf("Tcp读取:%d \n", N_rev_P1);
		rc = pthread_mutex_lock(&mutex);
		if (rc) {
			printf("临界资源加锁失败.");//临界资源即packet文件以及行号
			exit(1);
		}
		//字符串拼接函数,将多个字段拼接成一行字符串
		sprintf(bufferForOneLineTcpPrint,
				"%d.[TCP] 源地址端口:%d,目的地址端口:%d",
				N_Line_Packet, ntohs((uint16_t) tcp->th_sport), ntohs((uint16_t) tcp->th_dport));
		writeOneLineIntoFile(fd1, bufferForOneLineTcpPrint, strlen(bufferForOneLineTcpPrint) - 1);
		writeOneLineIntoFile(fd1, "\n", strlen("\n"));//写入换行符
		N_Line_Packet++;//行号+1
		N_wrt_P1++;//写入数据+1
		printf("Tcp写入:%d \n", N_wrt_P1);
		rc = pthread_mutex_unlock(&mutex);
		if (rc) {
			printf("解锁临界资源失败");//临界资源即packet文件以及行号
			exit(1);
		}
	}
	
	close(sock);
}
void *UdpThreadExe() {
	int sock;
	int rc;
	char buffer[500];
	char bufferForOneLineUdpPrint[40];
	int bytes_received;
	size_t sourceLen;

	struct sockaddr_in source; //本地地址结构体
	struct udphdr *udp;
	
	if ((sock = socket(AF_INET, SOCK_RAW, IPPROTO_UDP)) == -1) {
		perror("创建原始套接字失败.");
		exit(1);
	}
	Set_Promiscuous_Mode(ETH_NAME, sock);
	printf("Udp抓包开始\n");
	while (i) {

		sourceLen = sizeof(source);
		bytes_received = (int) recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr *) &source,
										(socklen_t *) &sourceLen);
		if (bytes_received < 0) {//return bytes received if succeeded
			perror("udp接收数据失败.");
			exit(-1);
		}
		N_rev_P2++;//udp account read

		udp = (struct udphdr *) (buffer + 20);//20 bytes from ip header to udphdr

		printf("Udp读取:%d \n", N_rev_P2);

		rc = pthread_mutex_lock(&mutex);
		if (rc) {
			printf("临界资源加锁失败");
			exit(1);
		}
		sprintf(bufferForOneLineUdpPrint,
				"%d.[UDP]源地址端口:%d,目的端口:%d",
				N_Line_Packet, ntohs((uint16_t) udp->uh_sport), ntohs((uint16_t) udp->uh_sport));

		writeOneLineIntoFile(fd1, bufferForOneLineUdpPrint, strlen(bufferForOneLineUdpPrint));
		writeOneLineIntoFile(fd1, "\n", strlen("\n"));
		N_Line_Packet++;
		N_wrt_P2++;
		printf("Udp写入:%d \n", N_wrt_P2);
		rc = pthread_mutex_unlock(&mutex);
		if (rc) {
			perror("临界资源解锁失败");
			exit(1);
		}
	}
	close(sock);
}

void sendFileWithTcp(char file_name[],char *ipaddr) {
	struct sockaddr_in server_address;//服务器地址
	struct timeval start, stop, delta;
	struct stat bufferForFileAttributes;
	char buffer[BUFFERT];
	off_t count = 0, m, size;
	int l = sizeof(struct sockaddr_in);
	long int n;
	int fd = open(file_name,O_RDWR,0644);//打开文件,读写方式
	if(fd < 0 ){
		perror("打开文件失败");
		exit(1);
	}
	int client_sock = socket(AF_INET, SOCK_STREAM, 0);//客户端套接字,用于建立与服务器的连接
	if (client_sock < 0) {
		perror("创建客户端套接字失败\n");
		exit(1);
	}

	bzero(&server_address, sizeof(struct sockaddr_in));//清空地址
	server_address.sin_family = AF_INET;//协议族
	server_address.sin_port = htons((uint16_t) serverPort);//主机地址转网络字节序
	
	if (inet_pton(AF_INET, ipaddr, &server_address.sin_addr) == 0) {//点分十进制转网络字节序,校验ip是否合法
		printf("非法Ip地址\n");
		exit(1);
	}
	
	//获取文件属性
	if (stat(file_name, &bufferForFileAttributes) == -1) {
		perror("获取文件属性失败");
		exit(1);
	} else
		size = bufferForFileAttributes.st_size;//文件字节数

	//清空缓冲区等待发送文件
	bzero(buffer, BUFFERT);    
	if (connect(client_sock, (struct sockaddr *) &server_address, (socklen_t) l) == -1) {
		perror("请求服务器失败\n");
		exit(1);
	}

	
	n = read(fd, buffer, BUFFERT);//读取定长文本
	
	while (n) {
		if (n == -1) {
			perror("读取失败");
			exit(1);
		}
		m = sendto(client_sock, buffer, (size_t) n, 0, (struct sockaddr *) &server_address, (socklen_t) l);//发送定长文本
		if (m == -1) {
			perror("发送失败");
			exit(1);
		}
		count += m;
		bzero(buffer, BUFFERT);
		n = read(fd, buffer, BUFFERT);
	}
	//0 for EOF
	sendto(client_sock, buffer, 0, 0, (struct sockaddr *) &server_address, (socklen_t) l);//发送最后一段
	
	printf("传输字节数 : %li\n", count);
	printf("文件实际大小 : %li \n", size);
	close(client_sock);
}

//信号处理函数
void *signalHandler(int signum) {
	i = 0;//退出while循环,停止tcp和udp的读写
	printf("\n信号处理函数响应中...\n");
	writeReport(fd2);//写入report文件
	printf("发送文件Packet中...");
	sendFileWithTcp(packet_file_name,ipAddress);
	sleep(5);//睡眠五秒处理时序问题
	printf("发送文件Report中...");
	sendFileWithTcp(report_file_name,ipAddress);
	exit(1);

}
int main(int argc, char **argv) {
	if (argc != 3) {
		printf("使用方法: %s <服务器ip地址> <服务器端口号>\n", argv[0]);
		exit(1);
	}
	pthread_t pid1, pid2;
	int thread1Return, thread2Return;//创建线程返回值
	ipAddress = argv[1];
	serverPort = atoi(argv[2]);//字符串转整型
	fd1 = open(packet_file_name, O_CREAT | O_RDWR | O_TRUNC, 00644);
	//打开文件,选项为没有则创建,可读写,可截断,权限设置为root用户读写,同组用户以及其他用户只读
	if (fd1 == -1) {
		printf("打开Packet文件失败: %s\n", strerror(errno));
		exit(-1);
	}
	fd2 = open(report_file_name, O_CREAT | O_RDWR | O_TRUNC, 00644);
	if (fd2 == -1) {
		perror("打开Report文件失败");
		exit(1);
	}
	signal(SIGINT,(__sighandler_t )signalHandler);

	thread1Return = pthread_create(&pid1, NULL, (void *(*)(void *)) TcpThreadExe,NULL);
	thread2Return = pthread_create(&pid2, NULL, (void *(*)(void *)) UdpThreadExe,NULL);
	if (thread1Return != 0 || thread2Return != 0) {
		perror("创建线程1或创建线程2失败");
		exit(1);
	}


	pthread_join(pid1, NULL);//线程挂起释放资源
	pthread_join(pid2, NULL);

	
	close(fd1);//关闭文件描述符
	close(fd2);
	return 0;
}

