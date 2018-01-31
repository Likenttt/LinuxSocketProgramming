
OFILES = client.o \
	server.o 

CFILECLT = client.c

CFILESRV = server.c

EXESRV = server

EXECLT = client

${EXESRV} :
	gcc  -o ${EXESRV} ${CFILESRV}

${EXECLT} :
	gcc  -o ${EXECLT} ${CFILECLT} -lpthread -lpcap
	#线程和lpcap库
clean :
	-rm -f ${OFILES} ${EXESRV} ${EXECLT} *~


                
