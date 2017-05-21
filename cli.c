#include "hw3.h"

typedef struct sockaddr SA;
int sockfd;
struct sockaddr_in serv;
pthread_mutex_t flck;
void log_in(){
	char name[20];
	// login
	printf("Connected!\nEnter your name to login: ");
	scanf("%s",name);
	write(sockfd,name,strlen(name));
	bzero(&name, strlen(name));
	getchar();	

	//put file list
	struct dirent *d;
	DIR *dir;
	dir = opendir(".");
	while((d = readdir(dir)) != NULL){
		if (d->d_type == DT_REG){
			write(sockfd,d->d_name,MAXLINE);
			char size[20];
			struct stat st;
			stat(d->d_name, &st);
			sprintf(size,"%lld",st.st_size);
			usleep(1000);
			write(sockfd,size,strlen(size));
		}
			
	}
	write(sockfd,"end",4);
	
	closedir(dir);
}

int make_connection(char* addr, int port){
	bzero(&serv, sizeof(serv));
	if((sockfd = socket(AF_INET, SOCK_STREAM,0)) < 0){
		printf("socket error\n");
		return -1;
	}
	serv.sin_family = AF_INET; 
	serv.sin_port = htons(port);
	if(inet_pton(AF_INET, addr, &serv.sin_addr) <= 0){
		printf("inet_pton error for %s\n", addr);
		return -1;
	}
		
	if(connect(sockfd, (SA *)&serv, sizeof(serv)) < 0){
		printf("connet error\n");
		return -1;
	}	
	return sockfd;
}

void get_file(void* args){
	getFile f = *(getFile*)args;
	int sock = make_connection(f.ip, 1500);
	if(sock < 0) return;
	else{
		write(sock,"to_me",MAXLINE);
		printf("connected\n");
		char size[10];
		//tell file
		write(sockfd,f.file,strlen(f.file));
		usleep(1000);
		//tell size
		sprintf(size,"%d",f.chunk);
		write(sockfd,size,strlen(size));
		usleep(1000);
		// tell offset
		sprintf(size,"%d",f.offset);
		write(sock, size, strlen(size));
		usleep(1000);

		//get file
		char buf[MAXLINE];
		FILE* fp = fopen(f.file,"a");
		int total = 0,download;
		while(total < f.chunk){
			download = read(sock,buf,MAXLINE);
			pthread_mutex_lock(&flck);
			fwrite(buf,sizeof(char),download,fp);
			pthread_mutex_unlock(&flck);
			total += download;
		}
		fclose(fp);
	}
	printf("download complete\n");
	pthread_detach(pthread_self());
}

void tell(void* args){
	getFile f = *(getFile*)args;
	int sock = make_connection(f.ip,1500);
	if(sock < 0) return;
	write(sock,"to_server",MAXLINE);
	printf("connected\n");
	//tell file
	write(sockfd,f.file,strlen(f.file));
	usleep(1000);
	//tell size
	char size[10];
	sprintf(size,"%d",f.chunk);
	write(sockfd,size,strlen(size));
	usleep(1000);
	// tell offset
	sprintf(size,"%d",f.offset);
	write(sock, size, strlen(size));
	usleep(1000);
	pthread_detach(pthread_self());
	pthread_exit(NULL);

}

void push_server(int connfd){
	char buf[MAXLINE] = "";
	//get file name
	read(connfd,buf,MAXLINE);
	FILE* fp = fopen(buf,"r");
	if(fp == NULL){
		printf("error\n");
		return;
	}	
	usleep(1000);
	//get file size
	bzero(&buf,strlen(buf));
	read(connfd,buf,MAXLINE);
	int size = atoi(buf);
	usleep(1000);
	//get offset
	read(connfd,buf,MAXLINE);
	int offset = atoi(buf);
	fseek(fp,offset-1,SEEK_SET);
	//read file
	int total = size;
	int upload;
	char f[MAXLINE];

	while(total > 0){
		if(total > MAXLINE)
			fread(f,sizeof(char),MAXLINE,fp);
		else
			fread(f,sizeof(char),total,fp);
		if(strlen(f) == 0){
			printf("error\n");
			return;
		}

		upload = write(sockfd,f,strlen(f));
		total -= upload;
	}
	fclose(fp);
	printf("upload complete\n");
	return;
}

void push_file(int connfd){
	char buf[MAXLINE] = "";
	//get file name
	read(connfd,buf,MAXLINE);
	FILE* fp = fopen(buf,"r");
	if(fp == NULL){
		printf("error\n");
		return;
	}	
	usleep(1000);
	//get file size
	bzero(&buf,strlen(buf));
	read(connfd,buf,MAXLINE);
	int size = atoi(buf);
	usleep(1000);
	//get offset
	read(connfd,buf,MAXLINE);
	int offset = atoi(buf);
	fseek(fp,offset-1,SEEK_SET);
	//read file
	int total = size;
	int upload;
	char f[MAXLINE];
	while(total > 0){
		if(total > MAXLINE)
			fread(f,sizeof(char),MAXLINE,fp);
		else
			fread(f,sizeof(char),total,fp);
		if(strlen(f) == 0){
			printf("error\n");
			return;
		}
		upload = write(connfd,f,strlen(f));
		total -= upload;
	}
	fclose(fp);
	printf("upload complete\n");
	return;
}

void str_cli(){
	char cmd[MAXLINE];
	char buf[MAXLINE];
	log_in();
	
	// input command
	printf(">> ");
	while(scanf("%s",cmd)){
		//log out
		if(strncmp("exit",cmd,4) == 0){
			printf("exit!!\n");
			write(sockfd, cmd,strlen(cmd));
			return;
		}

		if(strncmp("put",cmd,3) == 0){
			char file[MAXLINE];
			scanf("%s", file);
			FILE* fp;
			struct stat filestat;
			if(lstat(file,&filestat) < 0 || (fp = fopen(file,"rb")) == 0){
				printf("File error\n");
				bzero(&cmd,strlen(cmd));
				continue;
			}

			write(sockfd, cmd,strlen(cmd));
			write(sockfd,file,strlen(file));
			char size[MAXLINE];
			usleep(1000);
			sprintf(size,"%lld",filestat.st_size);
			write(sockfd, size,strlen(size));
			read(sockfd, buf,MAXLINE);

			int count = atoi(buf);
			char owner[20][MAXLINE];
			int i = 1;
			for(; i < count; i++)
				read(sockfd,owner[i],MAXLINE);

			int total = (int)filestat.st_size;
			int sz = total/count;
			int upload = 0;
			char f[MAXLINE];
			while(sz > 0){
				if(total < MAXLINE)
					upload = fread(f,sizeof(char),sz,fp);
				else
					upload = fread(f,sizeof(char),MAXLINE,fp);
				printf("%s\n", f);
				write(sockfd,f,upload);
				sz -= upload;
			}
			fclose(fp);
			printf("Upload complete!\n");
			int chunk = total/count;
			int offset = chunk+1;
			int current_sz = total-chunk;
			for(i = 1; i < count; i++){
				pthread_t t;
				getFile f;
				strcpy(f.ip,owner[i]);
				strcpy(f.file,file);
				if(current_sz < chunk || i == count-1)
					f.chunk = current_sz;
				else f.chunk = chunk;
				current_sz -= chunk;
				f.offset = offset;
				offset += f.chunk;
				pthread_create(&t,NULL,&tell,&f);
				//pthread_join(&t,NULL);
			}
			printf("wtf\n");
		}

		else if(strncmp("get",cmd,3)== 0){
			char file[MAXLINE];
			scanf("%s",file);
			write(sockfd, cmd,strlen(cmd));
			write(sockfd,file,strlen(file));

			read(sockfd,buf,MAXLINE);
			int num = atoi(buf);

			read(sockfd,buf,MAXLINE);

			//server has file
			if(strcmp("yes",buf) == 0){
				
				// only server has file
				if(num == 0){
					FILE* fp = fopen(file,"wb");
					read(sockfd,buf,MAXLINE);
					int total = atoi(buf);
					int download;
					while(total > 0){
						download = read(sockfd,buf,MAXLINE);
						fwrite(buf,sizeof(char),download,fp);
						total -= download;
					}
					
					fclose(fp);
					printf("download complete\n");
				} else{ //not only server has file

					FILE* fp = fopen(file,"wb");
					read(sockfd,buf,MAXLINE);
					int total = atoi(buf);
					printf("total %d\n", total);
					read(sockfd,buf,MAXLINE);
					int server_chunk = atoi(buf);
					printf("server_chunk %d\n", server_chunk);
					int downcount = server_chunk/(num+1);
					int download;
					while(downcount > 0){
						download = read(sockfd,buf,MAXLINE);
						fwrite(buf,sizeof(char),download,fp);
						printf("ser %s\n", buf);
						downcount -= download;
					}
					fclose(fp);
					// get other client's ip
					char l[20][MAXLINE];
					int i = 0;
					bzero(&buf,MAXLINE);
					for(; i < num; i++){
						read(sockfd,buf,MAXLINE);
						printf("%s\n", buf);
						strcpy(l[i],buf);
					}
					int chunk = total/(num+1);
					int tmp = total-chunk;
					for(i = 0; i < num; i++){
						pthread_t tid;
						getFile f;
						strcpy(f.ip,l[i]);
						strcpy(f.file,file);
						f.offset = (chunk+1)*(i+1);
						if(tmp < chunk || i == num-1)
							f.chunk = tmp;
						else
							f.chunk = chunk;
						tmp -= chunk;
						pthread_create(&tid,NULL,&get_file,&f);
					}

				}
			} else{ // server doesn't have file
				char size[100] = "";
				int total;
				read(sockfd,size,MAXLINE);
				total = atoi(size);
				printf("total %d\n", total);
				// get other client's ip
				char l[20][MAXLINE];
				int i = 0;
				bzero(&buf,MAXLINE);
				for(; i < num; i++){
					read(sockfd,buf,MAXLINE);
					strcpy(l[i],buf);
				}
				int chunk = total/num;
				int tmp = chunk;
				for(i = 0; i < num; i++){
					pthread_t tid;
					getFile f;
					strcpy(f.ip,l[i]);
					strcpy(f.file,file);
					if(i == 0)
						f.offset = i;
					else
						f.offset = (chunk+1)*(i+1);
					if(tmp < chunk || i == num-1)
							f.chunk = tmp;
					else
							f.chunk = chunk;
					tmp -= chunk;
					pthread_create(&tid,NULL,&get_file,&f);
				}
			}
		}

		else if(strncmp("user",cmd,4) == 0){
			printf("list user\n");
			write(sockfd, cmd,strlen(cmd));
			bzero(&buf,strlen(buf));
			int n = read(sockfd, buf,MAXLINE);
			//printf("%d\n", n);
			printf("%s", buf);
		}

		if(read(sockfd,buf,MAXLINE) > 0){
			printf("%s\n", buf);
		}
		
		
		// printf("%s", buf);
		bzero(&buf, strlen(buf));
		bzero(&cmd, strlen(cmd));
		printf(">> ");
	}
}	

void be_server(){
	struct sockaddr_in server,client;
	int connfd;
	int listenfd = socket(AF_INET, SOCK_STREAM,0);
	bzero(&server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(1500);
    int a = 1;
    setsockopt(listenfd,SOL_SOCKET, SO_REUSEADDR,&a,sizeof(a));
    bind(listenfd, (SA*)&server, sizeof(server));
    listen(listenfd,LISTENQ);
    char buf[MAXLINE] = "";
    while(1){
    	int len = sizeof(client);
		connfd = accept(listenfd, (SA*)&client,(socklen_t *)&len);
		read(connfd,buf,MAXLINE);
		if(strcmp("to_me",buf) == 0){
			push_file(connfd);
		} else 	push_server(connfd);		
    }
}

int main(int argv, char** argc){

	if(argv != 2){
		printf("invalid parameter\n");
		return 0;
	}
	pthread_t tid; 
	pthread_create(&tid,NULL,&be_server,NULL);
	// make connection
	int n = make_connection(argc[1],1300);
	if(n == -1) return 0;
	mkdir("share", S_IRWXU);
	chdir("share/");
	char path[1024];
	if(getcwd(path,sizeof(path)) != NULL)
		printf("%s\n", path);
	str_cli();
	
	return 0;
}
