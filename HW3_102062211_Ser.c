#include "hw3.h"


typedef struct sockaddr SA;
struct sockaddr_in addr;
user users[100];
files fileList[100];
int num = 0;
int fileListSize = 0;
pthread_mutex_t num_lck;
pthread_mutex_t f_lck;

int has_file(char* file){
	struct dirent *d;
	DIR *dir;
	dir = opendir(".");
	while((d = readdir(dir)) != NULL){
		if (d->d_type == DT_REG){
			if(strcmp(file,d->d_name) == 0){
				struct stat st;
				stat(file, &st);
				return st.st_size;
			}
				
		}
			
	}
	return 0;
}

void str_echo(void* arg){
	int n;
	char buf[MAXLINE] = {};
	char name[20];
	login l = *(login*)arg;
	int sockfd = l.connfd;
	int first = 1;
	while(1){
		while((n=read(sockfd,buf,MAXLINE)) > 0){
			if(first){ //login
				printf("%s %s login\n", buf, inet_ntoa(l.ip.sin_addr));
				strcpy(name,buf);
				user u;
				int i = 0;
				strcpy(u.name,buf);
				memcpy(u.ip,inet_ntoa(l.ip.sin_addr),sizeof(u.ip));
				u.sock = sockfd;
				//read file list
				while(1){
					bzero(&buf,MAXLINE);
					read(sockfd,buf,MAXLINE);
					if(strcmp(buf,"end") == 0)
						break;
					char size[100] = "";
					read(sockfd,size,MAXLINE);
					strcat(u.file,buf);
					strcat(u.file," ");
					u.file_num++;
					int j, flag = 0;
					pthread_mutex_lock(&f_lck);
					for(j = 0; j < fileListSize; j++){
						if(strcmp(fileList[i].file,buf) == 0){
							flag = 1;
							break;
						}
					}

					if(flag == 0){
						strcpy(fileList[fileListSize].file,buf);
						fileList[fileListSize++].size = atoi(size);
					}	
					pthread_mutex_unlock(&f_lck);
				}

				pthread_mutex_lock(&num_lck);
				for(i = 0; i < num; i++){
					if(strlen(users[i].name) == 0){
						break;
					}
				}
				users[i] = u;
				memcpy(users[i].ip,u.ip, sizeof(u.ip));
				num++;
				pthread_mutex_unlock(&num_lck);
				printf("%s\n", users[i].file);
				first = 0;
			}	
			else{ // start command
				// log out
				if(strncmp("exit",buf,4) == 0){
					printf("%s logout\n", name);
					int i = 0;
					pthread_mutex_lock(&num_lck);
					for(i = 0; i < num; i++){
						if(strcmp(name,users[i].name) == 0)
							break;
					}
					num--;
					pthread_mutex_lock(&num_lck);
					bzero(users[i].name, strlen(users[i].name));
					int j = 0;
					for(;j < users[i].file_num; j++)
						bzero(&users[i].file[j], strlen(users[i].file));
					pthread_detach(pthread_self());
					return;
				}
				if(strncmp("put",buf,3) == 0){
					char file[MAXLINE];
					read(sockfd,file,MAXLINE);
					printf("file %s\n", file);
					FILE* fp;
					if((fp = fopen(file,"wb")) == NULL){
						char warn[] = "open file error";
						write(sockfd,warn,strlen(warn));
					}

					read(sockfd,buf,MAXLINE);
					int total = atoi(buf);

					// check other owner
					char ips[20][MAXLINE];
					int socks[20];
					int count = 1, i = 0;
					char* cnt[20];
					bzero(&ips,sizeof(MAXLINE));
					pthread_mutex_lock(&num_lck);
					for(; i < num; i++){
						if(strstr(users[i].file,file) != NULL){
							if(users[i].sock != sockfd){
								strcpy(ips[count],users[i].ip);
								socks[count++] = users[i].sock;
							}	
						}
											
					}

					pthread_mutex_unlock(&num_lck);
					sprintf(cnt,"%d",count);
					write(sockfd,cnt,MAXLINE);	

					for(i = 1; i < count; i++)
						write(sockfd,ips[i],strlen(ips[i]));				

					printf("Start receiving...\n");
					fflush(stdout);
					bzero(&buf,MAXLINE);
					int upload;
					int sz = total/count;
					while(sz > 0){
						if(sz > MAXLINE)
							upload = read(sockfd, buf, MAXLINE);
						else
							upload = read(sockfd,buf,sz);
            			 sz -= upload;
            			 upload = fwrite(buf, sizeof(char), upload, fp);
					}

					int chunk = total/count;
					int current = total-chunk;
					for(i = 1; i < count; i++){
						int ch;
						if(current < chunk || i == count-1)
							ch = current;
						else ch = chunk;
						int rcv;
						bzero(&buf,MAXLINE);
						printf("read from %d\n", socks[i]);
						while(ch > 0){
							if(ch > MAXLINE)
								rcv = read(socks[i],buf,MAXLINE);
							else
								rcv = read(socks[i],buf,ch);
							ch -= rcv;
							fwrite(buf,sizeof(char),rcv,fp);
						}

					}
					printf("Upload complete\n");

					fclose(fp);

				}
				else if(strncmp("get",buf,3) == 0){
					char file[MAXLINE];
					read(sockfd,file,MAXLINE);
					char n[3];
					char ips[20][MAXLINE];
					int i = 0;
					int count = 0,size;
					pthread_mutex_lock(&num_lck);
					for(; i < num; i++){
						if(strstr(users[i].file,file) != NULL){
							if(users[i].sock != sockfd)
							strcpy(ips[count++],users[i].ip);
						}
											
					}
					pthread_mutex_unlock(&num_lck);
					sprintf(n,"%d",count);
					write(sockfd,n,MAXLINE);

					if((size=has_file(file)) > 0){
						write(sockfd,"yes",4);
						FILE* fp = fopen(file,"rb");
						char dest[MAXLINE] = "";

						if(count == 0){
							char filesz[10];
							sprintf(filesz,"%d",size);
							write(sockfd,filesz,MAXLINE);
							//fseek(fp,0,SEEK_SET);
							while(!feof(fp)){
								fread(dest,sizeof(char),MAXLINE,fp);
								write(sockfd,dest,strlen(dest));
							}
							fclose(fp);
							printf("download complete\n");
						} else{
							int total = size/(count+1);
							int reads = 0;
							char filesz[10];
							sprintf(filesz,"%d",size);
							write(sockfd,filesz,MAXLINE);

							sprintf(filesz,"%d",total);
							write(sockfd,filesz,MAXLINE);
							fseek(fp,0,SEEK_SET);
							if(total < MAXLINE){
								fread(dest,1,total,fp);
								write(sockfd,dest,total);
								fclose(fp);
							} else{
								while(total > 0){
									fread(dest,1,MAXLINE,fp);
									reads = write(sockfd,dest,MAXLINE);
									total -= reads;
								}
								fclose(fp);
								printf("download complete\n");
							}
							usleep(1000);
							int j = 0;
							for(; j < count; j++){
								write(sockfd,ips[j],MAXLINE);
							}
							continue;
						}
					
					}
					else{
						write(sockfd,"no",3);
						int j = 0;
						for(; j < fileListSize; j++){
							if(strcmp(fileList[j].file,file) == 0){
								char sz[20] = "";
								sprintf(sz,"%d",fileList[j].size);
								write(sockfd,sz,MAXLINE);
								break;
							}
						}
						for(j = 0; j < count; j++){
							write(sockfd,ips[j],MAXLINE);
						}
					}	
					
					// update user's file list
					for(i = 0; i < num; i++){
						if(strcmp(users[i].name,name) == 0){
							strcat(users[i].file, file);
							strcat(users[i].file,"\n");
						}
					}
				} else if(strncmp("user",buf,4) == 0){
					int i = 0, nn;
					char list[MAXLINE];
					pthread_mutex_lock(&num_lck);
					for(;i < num; i++){
						char d[2];
						sprintf(d,"%d",i+1);
						strcat(list,d);
						strcat(list,". ");
						strcat(list, users[i].name);
						strcat(list,"\n");
						int j = 0;
						strcat(list,users[i].file);
						strcat(list,"\n");
					}
					pthread_mutex_unlock(&num_lck);
					
					printf("%s\n", list);
					write(sockfd,list,MAXLINE);
				}

			 	//write(sockfd,buf,MAXLINE);
			}
			fflush(stdout);
			memset(buf,0,sizeof(buf));
		}
		if(n < 0 && errno == EINTR) continue;
		else if(n < 0){ 
			printf("read error\n");
		    printf("%s terminated\n", name);
		    pthread_detach(pthread_self());
		return;	
		}
	}
}

int make_connection(){
	int listenfd = socket(AF_INET, SOCK_STREAM,0);
	bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons(1300);
    int a = 1;
    setsockopt(listenfd,SOL_SOCKET, SO_REUSEADDR,&a,sizeof(a));
    bind(listenfd, (SA*)&addr, sizeof(addr));
    return listenfd;
}

int main(int argc, char** argv){

	int connfd;
	struct sockaddr_in serv, client;
	int listenfd = make_connection();
	listen(listenfd, LISTENQ);
	bzero(&users,sizeof(users));
	
	while(1){
		int len = sizeof(client);
		connfd = accept(listenfd, (SA*)&client,(socklen_t *)&len);
		printf("IP address is: %s, ", inet_ntoa(client.sin_addr));
        printf("port is: %d\n", (int) ntohs(client.sin_port));
        pthread_t tid;
        login l;
        memcpy(&l.ip, &client, sizeof(client));
        l.connfd = connfd;
        //close(connfd);
        pthread_create(&tid,NULL,&str_echo,&l);
	}
}



