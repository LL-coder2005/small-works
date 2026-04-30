#include "http.h"
#include "myhead.h"
#include <ctime>
#include "handle.h"

#define SIZE 4096
#define METHOD_SIZE 15
#define URL_SIZE 100
using namespace std;

void breakspace(int& ptr,char* buf)
{
    while(ptr<strlen(buf)-1&&isspace(buf[ptr])){
        ptr++;
    }
}
int init_http(int port)
{   
    //生成套接字
    int sersock = socket(AF_INET,SOCK_STREAM,0);
    if(sersock==-1){
        perror("socket error:");
        return -1;
    }

    //设置端口号快速重用
    int opt = 1;
    if(setsockopt(sersock,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt))==-1){
        perror("setsockopt error:");
        return -1;
    }

    //绑定ip地址和端口号
    /**
     * bind的“绑定”并不意味着设置此主机的ip地址，主机的ip地址由DHCP协议分配
     * 此处的绑定代表这个进程监听此ip地址的此端口号
     * sin.sin_addr.s_addr = htonl(INADDR_ANY);
     * 代表此主机监听它所有ip地址的port端口号
     * 注意，必须是属于此主机的ip地址，不能监听其他主机的
     * 客户端连接的时候，可以连接ip地址1，也可以连接ip地址2，但必须是此主机的ip地址
     * 
     */
    sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    if(bind(sersock,(sockaddr*)&sin,sizeof(sin))==-1){
        perror("bind error");
        return -1;
    }

    if(listen(sersock,128)==-1){
        perror("listen error:");
        return -1;
    }

    return sersock;
}

//读第一行数据
int get_line(int sock,char *buf)
{   
    //每次读取数据之前都要把ptr重置为0，否则会出问题
    int ptr = 0;
    char ch = '\0';
    int ret=0;
    while(ptr<SIZE&&ch!='\n'){
        ret = recv(sock,&ch,sizeof(ch),0);
        //recv == 0 表示对端关闭连接（EOF），ret < 0 才是系统错误
        if(ret==0){
            return 0;  // EOF
        }
        if(ret<0){
            return -1;
        }
        if(ret>0&&ch=='\r'){
            char next;
            ret = recv(sock,&next,sizeof(next),MSG_PEEK);

            if(ret==0){
                return 0;  // EOF
            }
            if(ret<0){
                return -1;
            }

            //如果是换行，直接读出来
            if(ret>0&&next=='\n'){
                recv(sock,&ch,sizeof(ch),0);
                continue;
            }
            else{
                ch='\n';
                continue;
            }
        }
        buf[ptr++] = ch;
    }
    buf[ptr] = '\0';
    return ptr;
}


int parse_msg(char* buf,char* method,char* url,char** file){
    
    int ptr = 0; //标志位
    //解析请求方法
    //跳过空格
    breakspace(ptr,buf);
    int p = 0;
    while(ptr<strlen(buf)-1&&p<METHOD_SIZE&&buf[ptr]!=' '){
        method[p++] = buf[ptr++];
    }
    method[p]='\0';

    breakspace(ptr,buf);
    int q = 0;
    while(ptr<strlen(buf)-1&&q<URL_SIZE&&buf[ptr]!=' '){
        if(buf[ptr]=='?'){
            *file = &url[q+1];
            url[q]='\0';
        }
        else{
            url[q] = buf[ptr];
        }
        q++;
        ptr++;
    }
    url[q]='\0';

    return 0;
}

int clear_header(int sock){
    char buf[SIZE]="";
    int ret=0;
    while(true){
        ret = get_line(sock,buf);
        if(ret==0){
            // 对端关闭连接，但没有真正的系统调用错误
            return -1;
        }
        if(ret<0){
            // 系统调用错误，errno有效
            perror("clear_header recv error");
            return -1;
        }
        // ret > 0，成功读到一行
        if(buf[0]=='\0'){
            // 读到空行（经过 get_line 处理后变成空字符串），header结束
            break;
        }
    }
    return 0;
}

int show405(int sock){
    clear_header(sock);
    //组装头部
    const char* msg = "HTTP/1.1 405 Method Not Allowed\r\n";
    if(send(sock,msg,strlen(msg),0)==-1){
        perror("send error : ");
        return -1;
    }
    if(send(sock,"\r\n",2,0)==-1){
        perror("send error : ");
        return -1;
    } //发送一个空行

    //把对应的文件返回给浏览器
    struct stat st;
    stat("wwwroot/405.html",&st);
    int fd = open("wwwroot/405.html",O_RDONLY);
    if(fd==-1){
        perror("open error : ");
        return -1;
    }
    if(sendfile(sock,fd,nullptr,st.st_size)==-1){
        perror("sendfile error: ");
        close(fd);
        return -1;
    }

    close(fd);

    return 0;
}

int show404(int sock){
    clear_header(sock);
    //组装头部
    const char* msg = "HTTP/1.1 404 NOT FOUND\r\n";
    if(send(sock,msg,strlen(msg),0)==-1){
        perror("send msg error:");
        return -1;
    }
    if(send(sock,"\r\n",2,0)==-1){
        perror("send msg error:");
        return -1;
    }
    //发送一个空行
    //把对应的文件返回给浏览器
    struct stat st;
    stat("wwwroot/404.html",&st);
    int fd = open("wwwroot/404.html",O_RDONLY);
    if(fd==-1){
        perror("open error:");
        return -1;
    }
    if(sendfile(sock,fd,nullptr,st.st_size)==-1){
        perror("sendfile error");
        close(fd);
        return -1;
    }
    close(fd);

    return 0;
}

void echo_error(int sock,int err){
    switch(err){
    case 405:
        show405(sock);
    break;
    case 404:
        show404(sock);
    break;
    }
}

int handle_request(int sock,char* method,char* path,char** file){
    
    char line[SIZE]="";
    int ret = 0;
    int countlen = 0;
    //判断请求方法
    //get则直接清空首部 
    if(strcasecmp(method,"get")==0){
        clear_header(sock);

    }
    else if(strcasecmp(method,"post")==0){
        // 按行读取header，直到空行，提取Content-Length
        while(true){
            ret = get_line(sock,line);
            if(ret<0){
                perror("handle_request recv error:");
                return -1;
            }
            if(line[0]=='\0'){
                break;
            }
            if(strncasecmp(line,"content-length:",15)==0){
                char* ptr = line + 15;
                while(*ptr && isspace(*ptr)) ptr++;
                countlen = atoi(ptr);
            }
        }
    }
    //根据读取到的长度把post的携带的数据解析出来
    
    char request_buf[4096]="";
    if(strcasecmp(method,"post")==0){
        int total = 0;
        while(total < countlen && total < (int)sizeof(request_buf)-1){
            ret = recv(sock,request_buf + total,countlen - total,0);
            if(ret==0) return 0;
            if(ret<0){
                perror("recv error:");
                return -1;
            }
            total += ret;
        }
        request_buf[total] = '\0';
    }
    //组装报头，发送给浏览器
    const char* msg = "HTTP/1.1 200 OK\r\n";
    if(send(sock,msg,strlen(msg),0)==-1){
        perror("send error : ");
        return -1;
    }
    if(send(sock,"Content-Type: text/html; charset=UTF-8\r\n",40,0)==-1){
        perror("send error : ");
        return -1;
    }
    if(send(sock,"Connection: close\r\n",19,0)==-1){
        perror("send error : ");
        return -1;
    }
    if(send(sock,"\r\n",2,0)==-1){
        perror("send error : ");
        return -1;
    } //发送一个空行
    //读取携带的数据并把数据传到一个新的函数里统一判断处理
    parse_and_process(sock,file,request_buf);

    return 0;

}

int msg_handle(int sock)
{
    char sum_buf[SIZE]=""; //总的请求数据
    if(recv(sock,sum_buf,SIZE,MSG_PEEK)==-1){
        perror("recv error : ");
        close(sock);
        return -1;
    }
    cout << sum_buf << endl; //打印请求数据，测试用
    char buf[SIZE]=""; //请求行
    char method[METHOD_SIZE]=""; //请求方法
    char url[URL_SIZE]=""; //url
    char* file = nullptr; //指向可能存在的文件

    int res = get_line(sock,buf); //解析请求行
    if(res==-1||strlen(buf)==0){
        //出现错误了,需要关闭套接字
        close(sock);
        return -1;
    }
    parse_msg(buf,method,url,&file);

    cout << buf << endl;
    cout << method << endl;
    log_request(method, url, 0); 
    //请求方法既不是post也不是get
    if(strcasecmp(method,"GET")!=0&&strcasecmp(method,"POST")!=0){
        cout << "method error" << endl;
        echo_error(sock,405);
        close(sock);
        return -1;
    }
    //如果是post请求，则需要额外处理
    bool need_handle = false;
    if(strcasecmp(method,"POST")==0){
        need_handle = true;
    }
    cout << url << endl;
    if(file != nullptr){
        cout << file << endl;
    }
    else{
        cout << "(no query)" << endl;
    }
    //如果是get请求并且带着要处理的数据，即file不为空，也要处理
    if(strcasecmp(method,"GET")==0&&file!=nullptr){
        need_handle = true;
    }
    //对URL进行处理
    char path[SIZE]="";
    sprintf(path,"./wwwroot%s",url);

    //如果URL为空，即/ 那么自动返回index
    if(url[strlen(url)-1]=='/'){
        strcat(path,"index.html");
    }
    cout << path << endl;

    //判断在服务器中是否存在path这个路径
    struct stat st;
    //返回-1说明文件不存在
    if(stat(path,&st)==-1){
        cout << "canot find this file" << endl;
        log_request(method, url, 404);
        echo_error(sock,404);
        close(sock);
        return -1;
    }

    //开始判断
    if(need_handle==1){
        
        int ret = handle_request(sock,method,path,&file);
        if(ret==-1){
            perror("post error : ");
            close(sock);
            return -1;
        }
    }
    else{
        clear_header(sock);
        //返回get请求的页面
        const char* msg = "HTTP/1.1 200 OK\r\n";
        send(sock,msg,strlen(msg),0);
        send(sock,"\r\n",2,0); //发送一个空行
        log_request(method, url, 200);
        //把对应的文件返回给浏览器
        int fd = open(path,O_RDONLY);
        if(fd==-1){
            perror("open error : ");
            close(sock);
            return -1;
        }
        if(sendfile(sock,fd,nullptr,st.st_size)==-1){
            perror("sendfile error: ");
            close(fd);
            close(sock);
            return -1;
        }
        close(fd);

    }

    //流程结束
    close(sock);

    return 0;
}

void log_request(const char* method, const char* url, int status)
{
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    char timestamp[30];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);
    
    FILE* logfile = fopen("server.log", "a");
    if(logfile == NULL) {
        perror("fopen error");
        return;
    }
    
    fprintf(logfile, "[%s] %s %s -> %d\n", timestamp, method, url, status);
    fclose(logfile);
}
