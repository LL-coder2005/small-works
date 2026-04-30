#include "handle.h"
#include "myhead.h"
#define HTML_SIZE 64*1024
int handle_login(int sock,char* request_buf){
    
    char reply_buf[HTML_SIZE] = "";

    char* usrname = strstr(request_buf,"username=");
    char* password = strstr(request_buf,"password=");
    if(usrname == nullptr || password == nullptr){
        return -1;
    }

    usrname += strlen("username=");
    *(password-1)='\0';
    password += strlen("password=");
    
    if(strcmp(usrname, password) == 0)
    {
        //程序执行至此，表示登录成功，此时可以跳转到首页
        sprintf(reply_buf, "<script>localStorage.setItem('usr_user_name', '%s'); </script>", usrname);    //用户名
        strcat(reply_buf, "<script>window.location.href='/index.html'; </script>");                //要跳转的页面

        //消息已经组装成功，此时发给网页端
        send(sock, reply_buf, strlen(reply_buf), 0);

    }
    else{
        const char* fail_script = "<script>alert('用户名或密码错误');window.location.href='/login.html';</script>";
        send(sock, fail_script, strlen(fail_script), 0);
    }

    return 0;
}

int parse_and_process(int sock, char **file, char *request_buf)
{   
    (void)file;
    if(strstr(request_buf,"username=")!=0&&strstr(request_buf,"password=")!=0){

        return handle_login(sock,request_buf);
    }

    return 0;
}