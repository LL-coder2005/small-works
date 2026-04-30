# 简单HTTP服务器总结   

## 经验  

1. 为每一个线程分配单独的资源，避免在循环中多个线程访问同一地址资源。 
2. 对可能出现的错误及时处理，不要想当然认为运行不会出错    
3. 边界检查要细致  
4. 使用`char*` 时，要用`strlen`表示长度。
5. 判断分支的变量要初始化。
6. 为了防止对端下线导致系统发送信号中断进程，可以忽略信号  
   ```c++
    signal(SIGPIPE, SIG_IGN);
   ```

## 流程框架  

1> 初始化服务器  
运行程序时从外界传入端口号，使用此端口号初始化服务器套接字。  
使用bind函数指定监听的IP地址和端口号，设置为监听模式。  
循环处理浏览器建立的连接请求，使用多线程进行不同连接的并发操作。  
同时将每个线程设置为分离态，自动释放资源。  

值得注意的是，为了防止程序执行过快导致连接套接字发生变化，从而线程间相互竞争。要给每一个线程都分配一份资源    

```c++

        int sock = accept(sersock,(sockaddr*)&cin,&socklen);
        if(sock==-1){
            perror("accepr error : ");
            return -1;
        }

        //为了防止多个线程相互竞争一个地址导致错误
        //每一个线程分配一个地址，存储sock的值
        int* pthsock = new int(sock);

        //使用多线程和浏览器通信
        pthread_t pthread;
        pthread_create(&pthread,nullptr,msg,(void*)pthsock);   
```

记住要手动释放申请的资源。    

2> 线程体中的流程。  

2.1 首先把请求报文的请求行完整获取下来，然后以空格为界限分别把请求方法，URL资源，和URL资源中问号后面的参数获取下来。   

获取参数的时候，可以直接用一个指针指向URL数组中的？，然后将？改为 \0 接着指针偏移就可以了。  

2.2 获取完请求方法后,先判断请求方法类型，如果不是`get`或`post`请求，则向浏览器返回405错误。  

如果是post方法，或者带着参数的get方法，那么把`need_handle`置为1，表示需要额外处理   

2.3 利用提取的URL，拼接得到具体路径，如果没有，那么拼接默认返回`index.html`，同时检查请求的文件是否存在，如果不存在，直接向浏览器发送404错误。  

```c++
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

```

2.4 如果`need_handle==false` 说明只是get请求，正常返回浏览器请求的页面就好。  

**注意：**  
1.向浏览器发送数据时如果用`char*`指针组装状态行，那么发送的时候务必务必用`strlen`表示发送的长度，而不是`sizeof`。  
2.别忘了首部行结束要多发一个`\r\n`。   
3.在向浏览器发送数据前先将之前缓冲区中的报文读掉 

如果`need_handle==true`说明是pos或者是带参数的get请求，单独处理。因为这个版本并没有处理带参数的get请求，所以当判断方法为get时，直接清空缓冲区就好了。   

如果是post请求，循环读取首部行，提取出`content-length:`这部分，拿到实体的长度   
```c++
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
```  
然后就可以读取发送的数据了