/* 
This code primarily comes from 
http://www.prasannatech.net/2008/07/socket-programming-tutorial.html
and
http://www.binarii.com/files/papers/c_sockets.txt
 */

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>

char latest[50] = {'\0'};
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void* start_server(void* port)
{
      int PORT_NUMBER = *(int*)port;
      // structs to represent the server and client
      struct sockaddr_in server_addr,client_addr;    
      
      int sock; // socket descriptor

      // 1. socket: creates a socket descriptor that you later use to make other system calls
      if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
	perror("Socket");
	exit(1);
      }
      int temp;
      if (setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&temp,sizeof(int)) == -1) {
	perror("Setsockopt");
	exit(1);
      }

      // configure the server
      server_addr.sin_port = htons(PORT_NUMBER); // specify port number
      server_addr.sin_family = AF_INET;         
      server_addr.sin_addr.s_addr = INADDR_ANY; 
      bzero(&(server_addr.sin_zero),8); 
      
      // 2. bind: use the socket and associate it with the port number
      if (bind(sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) {
	perror("Unable to bind");
	exit(1);
      }

      // 3. listen: indicates that we want to listn to the port to which we bound; second arg is number of allowed connections
      if (listen(sock, 5) == -1) {
	perror("Listen");
	exit(1);
      }
          
      // once you get here, the server is set up and about to start listening
      printf("\nServer configured to listen on port %d\n", PORT_NUMBER);
      fflush(stdout);
     
      while (1) {
        // 4. accept: wait here until we get a connection on that port
        int sin_size = sizeof(struct sockaddr_in);
        int fd = accept(sock, (struct sockaddr *)&client_addr,(socklen_t *)&sin_size);
        printf("Server got a connection from (%s, %d)\n", inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));
        
        // buffer to read data into
        char request[1024];
        
        // 5. recv: read incoming message into buffer
        int bytes_received = recv(fd,request,1024,0);
        // null-terminate the string
        request[bytes_received] = '\0';
        printf("Here comes the message:\n");
        printf("%s\n", request);
  
        char reply[100] = {'\0'};
        pthread_mutex_lock(&lock);
        sprintf(reply, "{\n\"name\": \"%s\"\n}\n", latest);
        pthread_mutex_unlock(&lock);
        printf("the reply is %s\n", reply);
        
        // 6. send: send the message over the socket
        // note that the second argument is a char*, and the third is the number of chars
        send(fd, reply, strlen(reply), 0);

        // 7. close: close the socket connection
        close(fd);
      }
      
      close(sock);
      printf("Server closed connection\n");
  
      return 0;
} 

void getTemperature() {

  int fd_ar = open("/dev/cu.usbmodem1421", O_RDWR);
  if (fd_ar == -1) {
    printf("usb error\n");
  }

  struct termios options; // struct to hold options
  tcgetattr(fd_ar, &options); // associate with this fd
  cfsetispeed(&options, 9600); // set input baud rate
  cfsetospeed(&options, 9600); // set output baud rate
  tcsetattr(fd_ar, TCSANOW, &options); // set options 
  
  int bytes_read;
  char buf[2];
  char current[50] = {'\0'};

      while (1) {
        buf[0] = '\0';
        bytes_read = read(fd_ar, buf, 1);       
        
        if (bytes_read != 0) {
          buf[1] = '\0';
          if (buf[0] != '\n') {
            strcat(current, buf);
          } else {
            pthread_mutex_lock(&lock);
            strcpy(latest, current);
            pthread_mutex_unlock(&lock);
            current[0] = '\0';
          }
        }         
      }
}

int main(int argc, char *argv[])
{
  pthread_t t1;
  int port_number;

  // check the number of arguments
  if (argc != 2)
    {
      printf("\nUsage: server [port_number]\n");
      exit(0);
    }

  port_number = atoi(argv[1]);

  pthread_create(&t1, NULL, &start_server, (void*)&port_number);
  getTemperature();
  pthread_join(t1, NULL);
  
}

