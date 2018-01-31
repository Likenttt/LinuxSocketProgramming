/*
layout of my masterpiece
-------------------------------------------------
|		EthCapturer		-+*	|
|-----------------------------------------------|
|toolbar(file help about)			|
|-----------------------------------------------|
|	infobar(message about  the status)	|
|	server ip	entry			|
|	server port 	entry			|
|	buttonStart	buttonStop		|
-------------------------------------------------
2017.12.29 composed by kentlee 

*/
#include <gtk/gtk.h>
#include <regex.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <unistd.h>   
#include <fcntl.h>   
#include <pcap.h>
#include <pthread.h>

#define MATCH_SUCCESS 1
#define MATCH_FAILURE 0
#define SYS_RUNNING 1
#define SYS_EXIT 0

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
int duration(struct timeval *start, struct timeval *stop, struct timeval *delta);
char *getDeviceSniffOn();//get device name
int Open_Raw_Socket();//set tcp raw socket
int Open_Raw_Socket_Udp();//set udp raw socket
void Set_Promiscuous_Mode(char *interface, int sock);
void writeTcpHeaderIntoFile(int fd, void *buf, size_t amt);//write one line into file
void *thread1(void *arg);
void *thread2(void *arg);
void *thread3();
void *writeReport(int fd);
void sendFileWithTcp(char file_name[], char *ipaddr);
int matchRegex(const char* pattern,const char* userString);
void open_about_dialog();

static int sysStatus = SYS_RUNNING;
GtkWidget *label;
GtkWidget *infobar;
static GtkWidget *entry ;
static GtkWidget *PortEntry;
static const char *ipAddress = NULL;
static const char *port =NULL;


static void destroy(GtkWidget *widget, gpointer data)
{
    gtk_main_quit();
}

static void on_button_clicked(GtkWidget *btn, gpointer data);

int main(int argc, char *argv[])
{    
	
    gtk_init(&argc, &argv);
    
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(window), 335, 308);//set size
    gtk_window_set_title(GTK_WINDOW(window),"EthCapturer");
    g_signal_connect(window, "destroy", G_CALLBACK(destroy), NULL);
    
   GtkWidget *image = gtk_image_new_from_file("../res/spongeBob.png");
   // gtk_container_add(GTK_CONTAINER(window), image);
    
    GtkWidget *menu_bar = gtk_menu_bar_new();
    GtkWidget *file_menu = gtk_menu_new();
    GtkWidget *help_menu = gtk_menu_new();
    GtkWidget *about_menu = gtk_menu_new();
    GtkWidget *separator = gtk_separator_menu_item_new();
    
    GtkWidget *menu_item = gtk_menu_item_new_with_label("File");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item),file_menu);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar),menu_item);
    
    
    menu_item = gtk_menu_item_new_with_label("Open");//submenu of File
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu),menu_item);
    
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), separator);/////!!!!!!!!!!!!!!!must add separator, or the layout shown is unexpected
    //GtkWidget *open_menu_item = gtk_menu_item_new_with_label("Packet");
   // gtk_menu_item_set_submenu(GTK_MENU_ITEM(open_menu_item),menu_item);
    //gtk_menu_shell_append(GTK_MENU_SHELL(menu_item),open_menu_item);
    
    
    
    
    menu_item = gtk_menu_item_new_with_label("Exit");//submenu of File
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu),menu_item);
    
    menu_item = gtk_menu_item_new_with_label("Help");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar),menu_item);
    
    
    GtkWidget *about_item = gtk_menu_item_new_with_label("About");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar),about_item);
    g_signal_connect(GTK_MENU_SHELL(about_item),"clicked",G_CALLBACK(open_about_dialog),NULL);
    
            
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 1);
    gtk_container_add(GTK_CONTAINER(window), vbox);    
    
    gtk_box_pack_start(GTK_BOX(vbox), menu_bar, FALSE, FALSE, 0);//put menu_bar in vbox
    
    infobar = gtk_info_bar_new();
    label = gtk_label_new("Welcome to EthCapturer!");//text on infobar
    gtk_info_bar_add_button(GTK_INFO_BAR(infobar), "Close", GTK_RESPONSE_OK);
    g_signal_connect(infobar, "response", G_CALLBACK(gtk_widget_hide), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), infobar, FALSE,FALSE, 0);
    GtkWidget *content_area = gtk_info_bar_get_content_area(GTK_INFO_BAR(infobar));//set content area within infobar 
    gtk_box_pack_start(GTK_BOX(content_area), label, TRUE, TRUE, 0);
    
    GtkWidget *fixed = gtk_fixed_new();
    gtk_container_add(GTK_CONTAINER(vbox), fixed);////fixed is a container
    gtk_container_add(GTK_CONTAINER(fixed), image);///add image into the fix container 
    
   GtkWidget *ServerIpLabel = gtk_label_new("Server IP:");
   gtk_fixed_put(GTK_FIXED(fixed), ServerIpLabel, 35, 145);

   entry = gtk_entry_new();//entry box for server ip
   gtk_entry_set_text(GTK_ENTRY(entry), NULL);
   gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "ip");
   gtk_fixed_put(GTK_FIXED(fixed), entry, 105, 140);
   
  
   GtkWidget *ServerPortLabel = gtk_label_new("Server Port:");
   gtk_fixed_put(GTK_FIXED(fixed), ServerPortLabel, 35, 185);
    
   PortEntry = gtk_entry_new();//entry box
   gtk_entry_set_placeholder_text(GTK_ENTRY(PortEntry), "port");
   gtk_fixed_put(GTK_FIXED(fixed), PortEntry, 105, 180);
   
   
   GtkWidget *btnStart = gtk_button_new_with_label("Capture");
   gtk_fixed_put(GTK_FIXED(fixed), btnStart, 105, 225);
   gtk_widget_set_size_request(btnStart, 60, 30);
   g_signal_connect(btnStart, "clicked", G_CALLBACK(on_button_clicked), "start");
   
   GtkWidget *btnStop = gtk_button_new_with_label("Stop");
   gtk_fixed_put(GTK_FIXED(fixed), btnStop, 205, 225);
   gtk_widget_set_size_request(btnStop, 60, 30);
   g_signal_connect(btnStop, "clicked", G_CALLBACK(on_button_clicked), "stop");
            
   gtk_widget_show_all(window);
   
   char packet_file_name[15] = "Packet.txt";
	char report_file_name[15] = "Report.txt";
	pthread_t pid1, pid2,pid3;
	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	int thread1Error, thread2Error,thread3Error;
	int i = 0;
	struct argument argument1, argument2;
	int fdPacketOpen = open(packet_file_name, O_CREAT | O_RDWR | O_TRUNC, 00644);
	if (fdPacketOpen == -1) {
		printf("Error occurred when Opening the file Packet: %s\n", strerror(errno));
		exit(-1);
	}
	int fdReportOpen = open(report_file_name, O_CREAT | O_RDWR | O_TRUNC, 00644);
	if (fdReportOpen == -1) {
		perror("Error occurred when opening the file Report");
		exit(1);
	}

	argument1.a_mutex = mutex;
	argument1.fd1 = fdPacketOpen;
	argument1.fd2 = fdReportOpen;

	argument2.a_mutex = mutex;
	argument2.fd1 = fdPacketOpen;
	argument2.fd2 = fdReportOpen;

	thread1Error = pthread_create(&pid1, NULL, (void *(*)(void *)) thread1, (void *) &argument1);
	thread2Error = pthread_create(&pid2, NULL, (void *(*)(void *)) thread2, (void *) &argument2);
	thread3Error = pthread_create(&pid3, NULL,(void *(*)(void *)) thread3,NULL);
	if (thread1Error != 0 || thread2Error != 0|| thread3Error != 0) {
		printf("Error occurred  when creating thread1 ,thread2 or thread3");
		exit(1);
	}


	pthread_join(pid1, NULL);
	pthread_join(pid2, NULL);

	
	close(fdPacketOpen);
	close(fdReportOpen);

   
   
    
   return 0;
}
static void on_button_clicked(GtkWidget *btn, gpointer data){
	gchar *text;
	const char *tmpIp,*tmpPort;
	int statusIp,statusPort;
	const char* ipPattern = "^(25[0-5]|2[0-4][0-9]|[0-1]{1}[0-9]{2}|[1-9]{1}[0-9]{1}|[1-9])\.(25[0-5]|2[0-4][0-9]|[0-1]{1}[0-9]{2}|[1-9]{1}[0-9]{1}|[1-9]|0)\.(25[0-5]|2[0-4][0-9]|[0-1]{1}[0-9]{2}|[1-9]{1}[0-9]{1}|[1-9]|0)\.(25[0-5]|2[0-4][0-9]|[0-1]{1}[0-9]{2}|[1-9]{1}[0-9]{1}|[0-9])$";//regex to match ipaddress
	const char* portPattern="^([0-9]|[1-9]\d{1,3}|[1-5]\d{4}|6[0-5]{2}[0-3][0-5])$";

	if(data == "start"){//startBtn
    		tmpIp = gtk_entry_get_text(GTK_ENTRY(entry));
    		tmpPort = gtk_entry_get_text(GTK_ENTRY(PortEntry));
    		//printf("%s  %li\n",tmpIp,strlen(tmpIp));
    		//printf("%s  %li\n",tmpPort,strlen(tmpPort));
    		//if(!strcmp(tmpIp,"127.0.0.1")){
    		//	printf("ip合适成功\n");
    		//}
    		//if(!strcmp(tmpPort,"80")){
    		//	printf("Port合适\n");
    		//}
    		if((statusIp=matchRegex(ipPattern,tmpIp)) == MATCH_FAILURE ){//why the port is always mismatched	
    			//printf("%d\n",statusIp);
    			//   printf("%d\n",statusPort);
    			//statusPort=matchRegex(portPattern,tmpPort);
    			//printf("statusPort= %d\n",statusPort);
    			text = g_strdup_printf("Invalid ipaddress or Port!");
    			gtk_label_set_text(GTK_LABEL(label), text);
    			gtk_info_bar_set_message_type(GTK_INFO_BAR(infobar), GTK_MESSAGE_ERROR);
    		}else{
    			ipAddress = tmpIp;
    			port = tmpPort;//match success 
    			text = g_strdup_printf("Capturing started");
    			gtk_label_set_text(GTK_LABEL(label), text);
    			gtk_info_bar_set_message_type(GTK_INFO_BAR(infobar), GTK_MESSAGE_QUESTION);
    			//printf("%s %s",port,ipAddress);
    		}
    	}else if(data == "stop"){//stopbtn
    		text = g_strdup_printf("Capturing stopped,sending Files ...");
    		sysStatus = SYS_EXIT;
    		sendFileWithTcp("Packet.txt",ipAddress);
    		sleep(3);
    		sendFileWithTcp("Report.txt",ipAddress);
    		gtk_label_set_text(GTK_LABEL(label), text);
    		gtk_info_bar_set_message_type(GTK_INFO_BAR(infobar), GTK_MESSAGE_WARNING);
    		
    	}
    	gtk_widget_show(GTK_WIDGET(infobar));
}


int matchRegex(const char* pattern, const char* userString)
{
    int result = MATCH_FAILURE;

    regex_t regex;
    int regexInit = regcomp( &regex, pattern, REG_EXTENDED );
    if( regexInit )
    {
        //Error print : Compile regex failed
    }
    else
    {
        int reti = regexec( &regex, userString, 0, NULL, 0 );
        if( REG_NOERROR != reti )
        {
            //Error print: match failed! 
        }
        else
        {
            result = MATCH_SUCCESS;
        }
    }
    regfree( &regex );
    return result;
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

int Open_Raw_Socket() {
	int sock;
	if ((sock = socket(AF_INET, SOCK_RAW, IPPROTO_TCP)) == -1) {
		perror("Error occurred when creating RAW socket for TCP.");
		exit(1);
	}
	return sock;
}
int Open_Raw_Socket_Udp() {
	int sock;
	if ((sock = socket(AF_INET, SOCK_RAW, IPPROTO_UDP)) == -1) {
		perror("Error occurred when creating RAW socket for UDP.");
		exit(1);
	}
	return sock;
}

void Set_Promiscuous_Mode(char *interface, int sock) {
	struct ifreq ifr;// Interface request structure
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

void writeTcpHeaderIntoFile(int fd, void *buf, size_t amt) {
	ssize_t bytes_writeTcp = write(fd, buf, amt);
	if (bytes_writeTcp != amt) {
		perror("Error occurred when writing Tcp header");
		exit(1);
	}

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
	while(ipAddress == NULL || port == NULL);
	int sock;
	int rc;
	struct argument *a = (struct argument *) arg;
	int fdP = a->fd1;
	int fdR = a->fd2;
	pthread_mutex_t mutex = a->a_mutex;
	char buffer[200];
	char bufferForOneLineTcpPrint[40];
	int bytes_received;
	size_t sourceLen;
	struct sockaddr_in source; 
	struct tcphdr *tcp;
	sock = Open_Raw_Socket();
	Set_Promiscuous_Mode(ETH_NAME, sock);
	printf("TCP Capture starts... \n");
	while (sysStatus) {

		sourceLen = sizeof(source);
		bytes_received = (int) recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr *) &source,
										(socklen_t *) &sourceLen);
		if (bytes_received < 0) {
			perror("Error occurred when receiving from source.");
			exit(-1);
		}
		N_rev_P1++;//tcp read 
		tcp = (struct tcphdr *) (buffer + 20);
		printf("Tcp read:%d \n", N_rev_P1);
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
		N_wrt_P1++;//written in
		printf("Tcp written:%d \n", N_wrt_P1);
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
	while(ipAddress == NULL || port == NULL);
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

	struct sockaddr_in source; //local address struct 
	struct udphdr *udp;
	sock = Open_Raw_Socket_Udp();
	Set_Promiscuous_Mode(ETH_NAME, sock);
	printf("Udp capture starts...\n");
	//printf("Raw socket for Udp is %d ,system is grasping data packets from network interface... \n", sock);
	while (sysStatus) {

		sourceLen = sizeof(source);
		bytes_received = (int) recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr *) &source,
										(socklen_t *) &sourceLen);
		if (bytes_received < 0) {//return bytes received if succeeded
			perror("Error occurred when receiving UDP from source.");
			exit(-1);
		}
		N_rev_P2++;//udp account read

		udp = (struct udphdr *) (buffer + 20);//20 bytes from ip header to udphdr

		printf("Udp read:%d \n", N_rev_P2);

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
		N_wrt_P2++;//user datagram written in 
		printf("Udp written:%d \n", N_wrt_P2);
		rc = pthread_mutex_unlock(&mutex);
		if (rc) {
			perror("Error occurred when unlocking N_line_Packet");
			exit(1);
		}
	}
	close(sock);
}
void *thread3(){
	gtk_main();
}
void sendFileWithTcp(char file_name[],char *ipaddr) {
	struct sockaddr_in server_address;//struct for server address
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
	int client_sock = socket(AF_INET, SOCK_STREAM, 0);//used to connect with server socket
	if (client_sock < 0) {
		perror("Error occurred when creating client_socket!\n");
		exit(1);
	}

	bzero(&server_address, sizeof(struct sockaddr_in));//clear every bit in address  !!!!
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons((uint16_t)(atoi(port)));//先把字符串转化整形在转化为网络字节序
		//ip is from parameter
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

	//prepare to send file,clean the buf
	bzero(&buffer, BUFFERT);    //!!!!!
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

void open_about_dialog()
{
	GtkWidget *aboutdialog = gtk_about_dialog_new();
    gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(aboutdialog), "EthCapturer");
    gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(aboutdialog), "1.0");
    gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(aboutdialog), "An excellent Tool used to capture flow on networking interface!");
    //gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(aboutdialog), "likent.cn");
    gtk_dialog_run(GTK_DIALOG(aboutdialog));
    gtk_widget_destroy(aboutdialog);
}

