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
#include <unistd.h>     //fork函数声明的头文件,文件读取写入操作定义的头文件
#include <fcntl.h>          //文件创建以及打开关闭操作函数定义的的头文件
#include <sys/wait.h>
#include <pcap.h>
#include <pthread.h>
#include <sys/stat.h>

#define SERVER_PORT 80
#define BUFFER_UPLOAD_MAX_SIZE 65535
#define BUFFERT 512
#define ETH_NAME getDeviceSniffOn()
static int N_Line_Packet = 1;
static int N_wrt_P2 = 0;
static int N_rev_P2 = 0;
static int N_wrt_P1 = 0;
static int N_rev_P1 = 0;

struct argument {
	int fd1;
	int fd2;
	pthread_mutex_t a_mutex;
};

static int i= 1;
int duration(struct timeval *start, struct timeval *stop, struct timeval *delta);

char *getDeviceSniffOn();//获取网卡设备名称
int Open_Raw_Socket();//设置tcp原始套接字
int Open_Raw_Socket_Udp();//设置udp原始套接字
void Set_Promiscuous_Mode(char *interface, int sock);//设置网卡为混杂模式
void writeTcpHeaderIntoFile(int fd, void *buf, size_t amt);//将tcp头部写入文件并检测异常
void *ctrlCHandlerTcp(int fd, int N_rev, int N_wrt, pthread_mutex_t a_mutex);//处理子进程1中的ctrl-c信号
void *ctrlCHandlerUdp(int fd, int N_rev, int N_wrt, pthread_mutex_t a_mutex);

void *thread1(void *arg);

void *thread2(void *arg);

void *signalHandler(int signum);

void *writeReport(int fd);

void sendFileWithTcp(char file_name[], char *ipaddr);

int main(int argc, char **argv) {

	if (argc != 2) {
		printf("Usage: ./%s ServerIPAddress\n", argv[0]);
		exit(1);
	}
	char packet_file_name[15] = "Packet.txt";
	char report_file_name[15] = "Report.txt";
	pthread_t pid1, pid2;
	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	int thread1Error, thread2Error;
	int i = 0;
	struct argument argument1, argument2;

	//创建packet文件用于写入两个子进程读取到的报文头
	int fdPacketOpen = open(packet_file_name, O_CREAT | O_RDWR | O_TRUNC, 00644);
	if (fdPacketOpen == -1) {
		//todo: 文件打开失败
		printf("Error occurred when Opening the file Packet: %s\n", strerror(errno));
		exit(-1);
	}
	int fdReportOpen = open(report_file_name, O_CREAT | O_RDWR | O_TRUNC, 00644);
	if (fdReportOpen == -1) {
		perror("Error occurred when opening the file Report");
		exit(1);
	}
	signal(SIGINT,(__sighandler_t )signalHandler);

	argument1.a_mutex = mutex;
	argument1.fd1 = fdPacketOpen;
	argument1.fd2 = fdReportOpen;

	argument2.a_mutex = mutex;
	argument2.fd1 = fdPacketOpen;
	argument2.fd2 = fdReportOpen;

	thread1Error = pthread_create(&pid1, NULL, (void *(*)(void *)) thread1, (void *) &argument1);
	thread2Error = pthread_create(&pid2, NULL, (void *(*)(void *)) thread2, (void *) &argument2);
	if (thread1Error != 0 || thread2Error != 0) {
		printf("Error occurred  when creating thread1 or thread2");
		exit(1);
	}


	pthread_join(pid1, NULL);
	pthread_join(pid2, NULL);

	
	close(fdPacketOpen);
	close(fdReportOpen);

	return 0;
}

/* Function allowing the calculation of the duration of the sending */
int duration(struct timeval *start, struct timeval *stop, struct timeval *delta) {
	suseconds_t microstart, microstop, microdelta;

	microstart = (suseconds_t) (100000 * (start->tv_sec)) + start->tv_usec;
	microstop = (suseconds_t) (100000 * (stop->tv_sec)) + stop->tv_usec;
	microdelta = microstop - microstart;

	delta->tv_usec = microdelta % 100000;
	delta->tv_sec = (time_t) (microdelta / 100000);

	if ((*delta).tv_sec < 0 || (*delta).tv_usec < 0)
		return -1;
	else
		return 0;
}


char *getDeviceSniffOn() {
	char errbuf[PCAP_ERRBUF_SIZE];
	return pcap_lookupdev(errbuf);
}

int Open_Raw_Socket() {//用于创建接收tcp类型的报文的原始套接字
	int sock;
	if ((sock = socket(AF_INET, SOCK_RAW, IPPROTO_TCP)) == -1) {
		perror("Error occurred when creating RAW socket for TCP.");
		exit(1);
	}
	return sock;
}

int Open_Raw_Socket_Udp() {//用于创建接收udp类型的报文的原始套接字
	int sock;
	if ((sock = socket(AF_INET, SOCK_RAW, IPPROTO_UDP)) == -1) {
		perror("Error occurred when creating RAW socket for UDP.");
		exit(1);
	}
	return sock;
}

void Set_Promiscuous_Mode(char *interface, int sock) {//设置网卡为混杂模式抓取一切数据
	struct ifreq ifr;// Interface request structure接口请求结构体
	/* Copy no more than N characters of SRC to DEST.  */
	strncpy(ifr.ifr_name, interface, strnlen(interface, sizeof(interface)) + 1);
	if (ioctl(sock, SIOCGIFFLAGS, &ifr) == -1) {
		perror("Set promiscuous error\n");
		exit(1);
	}
	ifr.ifr_flags |= IFF_PROMISC;
	if (ioctl(sock, SIOCGIFFLAGS, &ifr) == -1) {
		perror("Cannot set promiscuous flag");
		exit(1);
	}
}

void writeTcpHeaderIntoFile(int fd, void *buf, size_t amt) {//用于将tcp报文段的头部写入文件
	ssize_t bytes_writeTcp = write(fd, buf, amt);
	if (bytes_writeTcp != amt) {
		perror("Error occurred when writing Tcp header");
		exit(1);
	}

}

void *ctrlCHandlerTcp(int fd, int N_rev, int N_wrt, pthread_mutex_t a_mutex) {
	//todo:用于响应用户按下的ctrl+C,退出之间进行的数据同步以及report操作
	int rc;
	char bufferTcpReport[200];
	printf("Tcp CtrlC is responding");
	rc = pthread_mutex_lock(&a_mutex);
	if (rc) {
		printf("Error occurred when locking Tcp Report writing .");
		exit(1);
	}
	sprintf(bufferTcpReport,
			"Line in Packet:%d\nReport for TCP:\nTcp Header received:%d\n,Tcp header writed:%d\nPercentage:%lf\n",
			N_Line_Packet, N_rev, N_wrt, ((double) N_rev / N_wrt));
	writeTcpHeaderIntoFile(fd, bufferTcpReport, strlen(bufferTcpReport));

	rc = pthread_mutex_unlock(&a_mutex);
	if (rc) {
		printf("Error occurred when unlocking Tcp Report writing");
		exit(1);
	}

//	pthread_exit(NULL);//主线程
//	exit(1);
}

void *ctrlCHandlerUdp(int fd, int N_rev, int N_wrt, pthread_mutex_t a_mutex) {
	int rc;
	char bufferUdpReport[200];
	printf("Udp CtrlC is responding");
	rc = pthread_mutex_lock(&a_mutex);
	if (rc) {
		printf("Error occurred when locking Udp Report writing .");
		exit(1);
	}
	sprintf(bufferUdpReport, "Report for UDP:\nUdp Header received:%d\n,Udp header written:%d\nPercentage:%lf\n", N_rev,
			N_wrt, ((double) N_rev / N_wrt));
	writeTcpHeaderIntoFile(fd, bufferUdpReport, strlen(bufferUdpReport));
	rc = pthread_mutex_unlock(&a_mutex);
	if (rc) {
		printf("Error occurred when unlocking Udp Report writing");
		exit(1);
	}
//	if (close(sock))
//	exit(1);
	pthread_exit(NULL);
}

void *signalHandler(int signum) {
	i = 0;
	printf("SIGINT is responding\n");	
	// sendFileWithTcp("Report.txt","127.0.0.1");
	sendFileWithTcp("Packet.txt","127.0.0.1");
	exit(1);

}


void *writeReport(int fd) {
	char bufferReport[200];
	sprintf(bufferReport,
			"Line in Packet:%d\nReport for TCP:\nTcp Header received:%d\n,Tcp header written:%d\nPercentage:%lf\nReport for UDP:\nUdp Header received:%d\n,Udp header written:%d\nPercentage:%lf\n",
			N_Line_Packet - 1, N_rev_P1, N_wrt_P1, ((double) N_rev_P1 / N_wrt_P1), N_rev_P2, N_wrt_P2,
			((double) N_rev_P2 / N_wrt_P2));
	writeTcpHeaderIntoFile(fd, bufferReport, strlen(bufferReport));
}


void *thread1(void *arg) {//pthread_t  pid,int fd1, int fd2, pthread_mutex_t
	printf("TCP抓包开始\n");
	int sock;
	int rc;
	struct argument *a = (struct argument *) arg;
	int fdP = a->fd1;
	int fdR = a->fd2;
	pthread_mutex_t mutex = a->a_mutex;
	char buffer[200];//拼接tcp头部为字符串的
	char bufferForOneLineTcpPrint[40];
	int bytes_received;
	size_t sourceLen;
	
	struct sockaddr_in source; //需要绑定的ip地址结构体,这里指的是本地的
	struct tcphdr *tcp;
	sock = Open_Raw_Socket();
	Set_Promiscuous_Mode(ETH_NAME, sock);
	printf("Raw socket for Tcp is %d ,system is grasping data packets from network interface... \n", sock);


	while (i) {

		sourceLen = sizeof(source);
		bytes_received = (int) recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr *) &source,
										(socklen_t *) &sourceLen);
		if (bytes_received < 0) {//返回值大于零为实际的接收字节数,小于0接收失败
			perror("Error occurred when receiving from source.");
			exit(-1);
		}
		N_rev_P1++;//读取到的ip数据包个数

		tcp = (struct tcphdr *) (buffer + 20);//ip数据包和tcp数据包有20个字节的距离

		printf("读取的tcp数据包数目:%d \n", N_rev_P1);

		rc = pthread_mutex_lock(&mutex);
		if (rc) {
			printf("Error occurred when locking Tcp Header writing.");
			exit(1);
		}
		sprintf(bufferForOneLineTcpPrint,
				"%d.[TCP] SourcePort:%d,DestinationPort:%d",
				N_Line_Packet, ntohs((uint16_t) tcp->th_sport), ntohs((uint16_t) tcp->th_dport));
		writeTcpHeaderIntoFile(fdP, bufferForOneLineTcpPrint, strlen(bufferForOneLineTcpPrint) - 1);
		writeTcpHeaderIntoFile(fdP, "\n", strlen("\n"));
		N_Line_Packet++;
		N_wrt_P1++;//写入的tcp报文段的数目
		printf("写入tcp包的数目:%d \n", N_wrt_P1);
		rc = pthread_mutex_unlock(&mutex);
		if (rc) {
			printf("Error occurred when unlocking Tcp Header writing");
			exit(1);
		}
	}
	writeReport(fdR);
	close(sock);
}

void *thread2(void *arg) {
	printf("UDP抓包开始\n");
	int sock;
	int rc;
	struct argument *a = (struct argument *) arg;
	int fdP = a->fd1;
	int fdR = a->fd2;
	pthread_mutex_t mutex = a->a_mutex;
	char buffer[500];
	char bufferForOneLineUdpPrint[40];
	int bytes_received;
	size_t sourceLen;

	struct sockaddr_in source; //需要绑定的ip地址结构体,这里指的是本地的
	struct udphdr *udp;
	sock = Open_Raw_Socket_Udp();//创建udp套接字,记得以后需要关闭套接字
	Set_Promiscuous_Mode(ETH_NAME, sock);
	printf("Raw socket for Udp is %d ,system is grasping data packets from network interface... \n", sock);
	while (i) {

		sourceLen = sizeof(source);
		bytes_received = (int) recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr *) &source,
										(socklen_t *) &sourceLen);
		if (bytes_received < 0) {//返回值大于零为实际的接收字节数,小于0接收失败
			perror("Error occurred when receiving UDP from source.");
			exit(-1);
		}
		N_rev_P2++;//读取到的ip数据包个数

		udp = (struct udphdr *) (buffer + 20);//ip数据包和tcp数据包有20个字节的距离

		printf("读取的udp数据包数目:%d \n", N_rev_P2);

		rc = pthread_mutex_lock(&mutex);
		if (rc) {
			printf("Error occurred when locking N_line_Packet in UDP writing");
			exit(1);
		}
		sprintf(bufferForOneLineUdpPrint,
				"%d.[UDP] SourcePort:%d,DestinationPort:%d",
				N_Line_Packet, ntohs((uint16_t) udp->uh_sport), ntohs((uint16_t) udp->uh_sport));

		writeTcpHeaderIntoFile(fdP, bufferForOneLineUdpPrint, strlen(bufferForOneLineUdpPrint));
		writeTcpHeaderIntoFile(fdP, "\n", strlen("\n"));
		N_Line_Packet++;
		N_wrt_P2++;//写入的udp报文段的数目
		printf("写入udp包的数目:%d \n", N_wrt_P2);
		rc = pthread_mutex_unlock(&mutex);
		if (rc) {
			perror("Error occurred when unlocking N_line_Packet");
			exit(1);
		}
	}
	close(sock);
}

void sendFileWithTcp(char file_name[],char *ipaddr) {
//sendFileWithTcp(packet_file_name,fdPacketOpen,client_socket,server_address, buffer);
	struct sockaddr_in server_address;//服务器地址
	struct timeval start, stop, delta;
	struct stat bufferForFileAttributes;
	char buffer[BUFFERT];
	off_t count = 0, m, size;//long
	int l = sizeof(struct sockaddr_in);
	long int n;
	int fd = open(file_name,O_RDWR,0644);
	if(fd < 0 ){
		perror("fail to open file");
		exit(1);
	}
	int client_sock = socket(AF_INET, SOCK_STREAM, 0);//创建客户端套接字
	if (client_sock < 0) {
		perror("Error occurred when creating client_socket!\n");
		exit(1);
	}

	printf("Client_socket fd is:%d", client_sock);
	bzero(&server_address, sizeof(struct sockaddr_in));//清零每一位
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons((uint16_t) SERVER_PORT);
	// 服务器的IP地址来自程序的参数

	if (inet_pton(AF_INET, ipaddr, &server_address.sin_addr) == 0) {
		printf("Invalid IP address\n");
		exit(1);
	}
	//file size
	/* Get file attributes for FILE and put them in BUF.  */
	if (stat(file_name, &bufferForFileAttributes) == -1) {
		perror("Getting file attributes failed");
		exit(1);
	} else
		size = bufferForFileAttributes.st_size;

	printf("client socket is:%d\n", client_sock);
	//prepare to send file,clean the buf
	bzero(&buffer, BUFFERT);    //!!!!!!!!!!!!!!!!这一句为什么会清除client_socket的数值
	if (connect(client_sock, (struct sockaddr *) &server_address, (socklen_t) l) == -1) {
		perror("Connection Failed\n");
		exit(1);
	}
	gettimeofday(&start, NULL);
	/* Read NBYTES into BUF from FD.  Return the
   number read, -1 for errors or 0 for EOF.
	 here is 512 bytes in every read action*/
	n = read(fd, buffer, BUFFERT);
	printf("the bytes read is :%li \n", n);

	while (n) {
		if (n == -1) {
			perror("Read failed");
			exit(1);
		}
		/* Send m bytes of BUF on socket FD to peer at address ADDR (which is
			ADDR_LEN bytes long).  Returns the number sent, or -1 for errors.*/
		m = sendto(client_sock, buffer, (size_t) n, 0, (struct sockaddr *) &server_address, (socklen_t) l);
		if (m == -1) {
			perror("Send Error");
			exit(1);
		}
		count += m;
		bzero(buffer, BUFFERT);
		n = read(fd, buffer, BUFFERT);
	}
	//0 for EOF
	sendto(client_sock, buffer, 0, 0, (struct sockaddr *) &server_address, (socklen_t) l);
	gettimeofday(&stop, NULL);
	duration(&start, &stop, &delta);
	printf("Bytes transferred : %li\n", count);
	printf("Total size of file : %li \n", size);
	printf("Time used : %ld.%li \n", delta.tv_sec, delta.tv_usec);
	close(client_sock);
}
