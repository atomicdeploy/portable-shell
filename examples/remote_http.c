/* Loopback-only demonstration transport, not a public HTTP server.
   One request per connection; bounded headers/body; no chunked requests.
   Firmware can use remote_device.h with its existing authenticated HTTP stack. */
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
typedef SOCKET socket_t;
#define socket_close closesocket
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
typedef int socket_t;
#define INVALID_SOCKET (-1)
#define socket_close close
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "remote_device.h"
typedef struct {unsigned char bytes[4096];unsigned size;int overflow;} Reply;
static void append(void *opaque,const unsigned char *b,unsigned n){Reply *r=opaque;if(n>sizeof r->bytes-r->size){r->overflow=1;return;}memcpy(r->bytes+r->size,b,n);r->size+=n;}
static int send_all(socket_t fd,const char *b,unsigned n){while(n){int k=send(fd,b,(int)n,0);if(k<=0)return 0;b+=k;n-=(unsigned)k;}return 1;}
static void serve(socket_t fd,ps_remote_provider *provider){
    char request[8192],method[12],path[1024],version[16],header[160];
    unsigned used=0,length=0,offset=0;int status=400,have_length=0;Reply reply={{0},0,0};
    while(used+1<sizeof request){
        char *end,*line;int n=recv(fd,request+used,(int)(sizeof request-used-1),0);
        if(n<=0)goto respond;used+=(unsigned)n;request[used]=0;
        end=strstr(request,"\r\n\r\n");if(!end)continue;offset=(unsigned)(end-request)+4;
        if(sscanf(request,"%11s %1023s %15s",method,path,version)!=3||strcmp(version,"HTTP/1.1"))goto respond;
        line=strstr(request,"\r\n");if(!line)goto respond;line+=2;
        while(line<end){char *next=strstr(line,"\r\n"),*colon;unsigned i,key;char name[64];
            if(!next||next>end)goto respond;colon=memchr(line,':',(size_t)(next-line));if(!colon)goto respond;
            key=(unsigned)(colon-line);if(!key||key>=sizeof name)goto respond;
            for(i=0;i<key;++i)name[i]=(char)tolower((unsigned char)line[i]);name[key]=0;
            if(!strcmp(name,"transfer-encoding")){status=501;goto respond;}
            if(!strcmp(name,"expect")){status=417;goto respond;}
            if(!strcmp(name,"content-length")){
                char *p=colon+1;if(have_length++)goto respond;while(p<next&&(*p==' '||*p=='\t'))++p;
                if(p==next)goto respond;
                while(p<next){if(*p<'0'||*p>'9'||length>4096)goto respond;length=length*10+(unsigned)(*p++-'0');}
                if(length>4096){status=413;goto respond;}
            }line=next+2;
        }
        if(offset+length>=sizeof request){status=413;goto respond;}
        while(used<offset+length){n=recv(fd,request+used,(int)(offset+length-used),0);if(n<=0)goto respond;used+=(unsigned)n;}
        status=ps_remote_dispatch(provider,method,path,(unsigned char*)request+offset,length,append,&reply);break;
    }
respond:
    if(reply.overflow){status=507;reply.size=0;}
    snprintf(header,sizeof header,"HTTP/1.1 %d Result\r\nContent-Type: text/plain; charset=utf-8\r\nContent-Length: %u\r\nConnection: close\r\n\r\n",status,reply.size);
    if(send_all(fd,header,(unsigned)strlen(header)))send_all(fd,(char*)reply.bytes,reply.size);
}
int main(int argc,char **argv){
    socket_t listener,client;struct sockaddr_in address;DemoDevice device={{0},0,0};ps_remote_provider provider=demo_provider(&device);
    unsigned port=argc>1?(unsigned)strtoul(argv[1],0,10):8088,count=argc>2?(unsigned)strtoul(argv[2],0,10):8;
#ifdef _WIN32
    WSADATA wsa;if(WSAStartup(MAKEWORD(2,2),&wsa))return 1;
#endif
    if(!port||port>65535||!count)return 2;
    listener=socket(AF_INET,SOCK_STREAM,0);if(listener==INVALID_SOCKET)return 1;
    memset(&address,0,sizeof address);address.sin_family=AF_INET;address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);address.sin_port=htons((unsigned short)port);
    if(bind(listener,(struct sockaddr*)&address,sizeof address)||listen(listener,4)){socket_close(listener);return 1;}
    while(count--){
        client=accept(listener,0,0);if(client==INVALID_SOCKET)break;
#ifdef _WIN32
        {DWORD timeout=5000;setsockopt(client,SOL_SOCKET,SO_RCVTIMEO,(char*)&timeout,sizeof timeout);setsockopt(client,SOL_SOCKET,SO_SNDTIMEO,(char*)&timeout,sizeof timeout);}
#else
        {struct timeval timeout={5,0};setsockopt(client,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof timeout);setsockopt(client,SOL_SOCKET,SO_SNDTIMEO,&timeout,sizeof timeout);}
#endif
        serve(client,&provider);socket_close(client);
    }
    socket_close(listener);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
