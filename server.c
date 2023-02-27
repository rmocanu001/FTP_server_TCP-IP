#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>

#define COUNT 1024

typedef struct packet{
    char data[COUNT];
}Packet;

typedef struct frame{
    int frame_kind;
    int sq_no;
    int ack;
    Packet packet;
}Frame;
Frame frame_recv,frame_send;



int strremove(char *str, const char *sub) {
    size_t len = strlen(sub);
    if (len > 0) {
        char *p = str;
        while ((p = strstr(p, sub)) != NULL) {
            memmove(p, p + len, strlen(p + len) + 1);
        }
    }
    return 1;
}

void populateList(char*lista){
  DIR *d;
    struct dirent *dir;
    d = opendir("./files/");
    if (d)
    {
        while ((dir = readdir(d)) != NULL)
        {
          if(strcmp(dir->d_name,".")!=0&&strcmp(dir->d_name,"..")!=0){
          strcat(lista,dir->d_name);
          strcat(lista,"\n");
          //printf("%s\n", lista);}

        }
        }

        printf("%s\n", lista);

        closedir(d);
    }
    return(0);
}

ssize_t xread(int fd, void *buf, size_t count)
{
    size_t bytes_read = 0;
    while (bytes_read < count) {
        ssize_t bytes_read_now = read(fd, buf + bytes_read,count - bytes_read);

        if (bytes_read_now == 0) 
            return bytes_read;

        if (bytes_read_now < 0) 
            return -1;

        bytes_read += bytes_read_now;
    }
    return bytes_read;
}

void send_to(int sock_fd,struct timeval timeout,struct sockaddr_in new_addr)
{
  setsockopt (sock_fd,SOL_SOCKET, SO_SNDTIMEO,&timeout,sizeof(timeout));
  sendto(sock_fd,&frame_send,sizeof(frame_send),0,(struct sockaddr*)&new_addr,sizeof(new_addr));
}

int receive_from(int sock_fd,struct timeval timeout,struct sockaddr_in new_addr,socklen_t addr_size)
{
  setsockopt (sock_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
  return recvfrom(sock_fd,&frame_recv,sizeof(frame_recv),0,(struct sockaddr*)&new_addr,&addr_size);
}

int main(int argc, char **argv)
{
  if(argc != 2 ){
    printf("Usage: %s port\n",argv[0]);
    exit(1);
  }
  int port=atoi(argv[1]);

  int sock_fd;
  struct sockaddr_in server_addr, new_addr; 
  char buffer[COUNT];
  char filename[COUNT];
  socklen_t addr_size;

  sock_fd=socket(AF_INET,SOCK_DGRAM,0);

    char list_file[1024];
  memset(list_file,0,sizeof(list_file));

  memset(&server_addr,0,sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port=htons(port);
  server_addr.sin_addr.s_addr=inet_addr("127.0.0.1");

   struct timeval timeout;
   timeout.tv_sec = 10;
   timeout.tv_usec = 0;

  int frame_id=0;
  int ack_recv=1;
  bind(sock_fd, (struct sockaddr*)&server_addr,sizeof(server_addr));
  addr_size=sizeof(new_addr);

  char ob[COUNT];

  
  populateList(list_file);
  int kk=1;
  do{
  memset(ob,0,sizeof(ob));
      int received = recvfrom(sock_fd,
                          &ob,
                          sizeof(filename),
                          0,
                          (struct sockaddr*)&new_addr,
                          &addr_size);

  
  if(strcmp(ob,"GET")==0){

  int received = recvfrom(sock_fd,
                          &filename,
                          sizeof(filename),
                          0,
                          (struct sockaddr*)&new_addr,
                          &addr_size);
  char fullPath[300];
  memset(fullPath,0,sizeof(fullPath));
  strcpy(fullPath,"./files/");
  strcat(fullPath,filename);
  int fd=open(fullPath,O_RDONLY);
  if(fd<0){
    printf("[-]File doesn't exist on server...(%s)\n",filename );
    //exit(1);

  }
  else{
  int bytes_read = 0;
int kl = 1;

while (kl) {
    if (ack_recv == 1) {
        // Prepare and send new frame
        frame_send.sq_no = frame_id;
        frame_send.frame_kind = 1;
        frame_send.ack = 0;
        bytes_read = xread(fd, buffer, COUNT);
        buffer[bytes_read] = '\0';

        // Check if end of file is reached
        if (bytes_read == 0 && frame_id > 0) {
            frame_send.frame_kind = 2;
            send_to(sock_fd, timeout, new_addr);
            printf("[+]Last frame sent. Connection is closing...\n");
            kl = 0;
            break;
        }

        strcpy(frame_send.packet.data, buffer);
        send_to(sock_fd, timeout, new_addr);
        printf("[+]Frame Send. Frame: %d. Waiting for ack...\n", frame_id);
    }

    // Wait for acknowledgement
    int received = receive_from(sock_fd, timeout, new_addr, addr_size);
    if (received > 0 && frame_recv.sq_no == 0) {
        if (frame_recv.ack == frame_id + 1) {
            printf("[+]Ack Received. Frame: %d\n", frame_id);
            ack_recv = 1;
        } else {
            printf("[-]Waited ACK not received. Frame: %d. Sending packet again...\n", frame_id);
            send_to(sock_fd, timeout, new_addr);
            ack_recv = 0;
            frame_id--;
        }
    } else {
        printf("[-]Ack not received. Frame: %d \n", frame_id);
        kl = 0;
        ack_recv = 0;
        break;
    }

    frame_id++;
}

  //close(sock_fd);
  }
  }

  if(strcmp(ob,"LS")==0){
    
    strcpy(frame_send.packet.data, list_file);

    send_to(sock_fd,timeout,new_addr);
    //memset(&frame_send,0,sizeof(frame_send));
  }

  if(strcmp(ob,"DELETE")==0){

    char fullPath[300];
      int received = recvfrom(sock_fd,
                          &filename,
                          sizeof(filename),
                          0,
                          (struct sockaddr*)&new_addr,
                          &addr_size);

    if(received<0)
      exit(1);
    //printf("aici\n");
    memset(fullPath,0,sizeof(fullPath));
    strcpy(fullPath,"./files/");
    //printf("aici2\n");
    strcat(fullPath,filename);
    //printf("aici3\n");
    strcat(filename,"\n");
    printf("%s\n",fullPath);
    if(strremove(list_file,filename)==1){
      printf("Succesfull remove\n");
      remove(fullPath);
    }
    else{
      printf("ERROR remove\n");

    }
    

  }
    if(strcmp(ob,"EX")==0){
      kk=0;
      break;
    }
    memset(&frame_send,0,sizeof(frame_send));
    memset(&frame_recv,0,sizeof(frame_send));
  
  }while (kk==1);
  close(sock_fd);
  return 0;
}
