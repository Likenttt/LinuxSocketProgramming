# 
#Makefile starts
#


#使用的C语言编译器
CC = gcc


#object文件
OFILES = client.o \
	server.o 

#客户端源程序
CFILECLT = client.c

#服务器源程序
CFILESRV = server.c

FILE_SRV = ClientPacket* ClientReport* Packet.txt Report.txt
		

#可执行的服务器程序
EXESRV = server

#可执行的客户端程序
EXECLT = client


but : ${EXESRV} ${EXECLT} 


${EXESRV} :
	$(CC) $(CFLAGS) -o ${EXESRV} ${CFILESRV}

#使用的多线程,线程机制不是linux的原有机制,需要链接pthread线程库
${EXECLT} :
	$(CC) $(CFLAGS) -o ${EXECLT} ${CFILECLT}  -lpthread

#清理废弃文件
clean :
	-rm -f ${OFILES} ${EXESRV} ${EXECLT} ${FILE_SRV} *~
                 
mrproper : clean but

#伪目标,没有将任何的项目作为依赖
.PHONY : but clean mrproper

#Makefile ends



