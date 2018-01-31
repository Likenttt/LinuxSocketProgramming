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
#define PORT 80
#define BUFFERT 512
/*Size of the customer queue */
#define BACKLOG 1

int duration (struct timeval *start,struct timeval *stop, struct timeval *delta);
int create_server_socket (int port);

struct sockaddr_in sock_serv,sock_clt;


int main(int argc,char** argv){
    socklen_t len;
    int sfd,fd;
    long int n, m,count=0;
    unsigned int nsid;
    ushort clt_port;
    char buffer[BUFFERT],filename[256];
    char dst[INET_ADDRSTRLEN];

    //Variable for the date
    time_t intps;
    struct tm* tmi;

    if(argc!=2) {
        perror("usage ./a.out <port>\n");
        exit(1);
    }

    sfd = create_server_socket(atoi(argv[1]));

    bzero(buffer,BUFFERT);

    if ( listen(sfd,BACKLOG) == -1){
        perror("Listen failed");
        exit(1);
    }

    //connect client
    len = sizeof(struct sockaddr_in);
    nsid=  accept(sfd, (struct sockaddr*)&sock_clt, &len);
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
        sprintf(filename,"ClientPacket.%d.%d.%d-%d:%d:%d",1900+tmi->tm_year,tmi->tm_mon+1,tmi->tm_mday,tmi->tm_hour,tmi->tm_min,tmi->tm_sec);
        printf("Creating the copied output file : %s\n",filename);

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

    printf("End of transmission with %s.%d\n",dst,clt_port);
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

    if(setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR,&yes,sizeof(int)) == -1 ) {
        perror("set sockopt error");
        exit(1);
    }

    bzero(&sock_serv,sizeof(struct sockaddr_in));
    sock_serv.sin_family=AF_INET;
    sock_serv.sin_port=htons((uint16_t) port);
    sock_serv.sin_addr.s_addr=htonl(INADDR_ANY);

    if(bind(sfd,(struct sockaddr*)&sock_serv,sizeof(struct sockaddr_in))==-1){
        perror("bind fail");
        exit(1);
    }


    return sfd;
}
