#include <stdio.h>
#include <string.h>  //strlen
#include <sys/socket.h>
#include <arpa/inet.h>  //inet_addr
#include <unistd.h>  //write
#include <fcntl.h>
#include <sys/stat.h>
#include "base64.h"


int fromHex(char ch) {
  if(ch >= '0' && ch <= '9')
    return (int) ch - '0';
  return (int) ch - 'A' + 10;
}

void decodeURL(char* src,char* dest) {
  while(*src != '\0') {
    if(*src == '%') {
      ++src;
      int n1 = fromHex(*src++);
      int n2 = fromHex(*src++);
      *dest++ = (char) n1*16+n2;
    } else {
      *dest++ = *src++;
    }
  }
  *dest = '\0';
}


void serveRequest(int fd) {
  // Read the request
char buffer[1024];
int bytesRead = read(fd,buffer,1024);
buffer[bytesRead] = '\0';

// Grab the method and URL
char method[16];
char url[128];
sscanf(buffer,"%s %s",method,url);


  char fileName[128];
  strcpy(fileName,"www");
  strcat(fileName,url);
  int filed = open(fileName,O_RDONLY);
  if(filed == -1) {

    // post % get request


     if(strcmp(method, "POST")==0 && strcmp(url, "/encode") ==0){


      char* linkBody = strstr(buffer, "\r\n\r\nurl=");

      if(linkBody != NULL){

        linkBody = linkBody+8;
        char decodedURL[1024];
      // decoding the % and others values from url
        decodeURL(linkBody, decodedURL);

        FILE* savingFile = fopen("Urls.txt", "a");
        long filePos;
        if (savingFile != NULL) {
         filePos= ftell(savingFile);
          fprintf(savingFile, "%s\n", decodedURL);
            fclose(savingFile);
          }
         
          

          char smallURL[9];
          
          encode(filePos, smallURL);

          char replyBuffer[1024];

          FILE* replyTemp = fopen("replyTemp.txt", "r");

          if(replyTemp!= NULL){
            fread(replyBuffer,sizeof(char), sizeof(replyBuffer)-1, replyTemp);

              fclose(replyTemp);

              char* placeholder = strstr(replyBuffer, "XXXXXX");

              if(placeholder!=NULL){

                strncpy(placeholder,smallURL, strlen(smallURL));
              }
            write(fd, replyBuffer,strlen(replyBuffer));
          }
        
        
        }// linkbody checker close


     } else if(strcmp(method, "GET") == 0 && strncmp(url, "/s/", 3)==0){               // get request



      char* shrtCode = strstr(url, "/s/");

        

        FILE* URLfile = fopen("Urls.txt", "r");

        if(URLfile != NULL){
          shrtCode+=3;
          
          int pos = decode(shrtCode);

      fseek(URLfile, pos, SEEK_SET);

      char originalURL[1024];
      
      if (fgets(originalURL, sizeof(originalURL), URLfile) != NULL) {
                

                 int len = strlen(originalURL);
                if (len > 0 && originalURL[len - 1] == '\n') {
                    originalURL[len - 1] = '\0';
                }
                    char responseBuffer[1024];
                    snprintf(responseBuffer, sizeof(responseBuffer),
                             "HTTP/1.1 301 Moved Permanently\r\nLocation: %s\r\n\r\n", originalURL); // this somethis shows small warning in the terminal but it works fine

                    // Send the redirection response to the client
                    write(fd, responseBuffer, strlen(responseBuffer));

                    // Close the URL file
                    fclose(URLfile);
                
        } else{ 
           // The user has a requested a file that we don't have.
            // Send them back the canned 404 error response.
            int f404 = open("404Response.txt",O_RDONLY);
            int readSize = read(f404,buffer,1023);
            close(f404);
            write(fd,buffer,readSize);

        }

     } else{                      // if this fails the 404 response 

    // The user has a requested a file that we don't have.
    // Send them back the canned 404 error response.
    int f404 = open("404Response.txt",O_RDONLY);
    int readSize = read(f404,buffer,1023);
    close(f404);
    write(fd,buffer,readSize);
  }
     
  }
  } else {
    const char* responseStatus = "HTTP/1.1 200 OK\n";
    const char* responseOther = "Connection: close\nContent-Type: text/html\n";
    // Get the size of the file
    char len[64];
    struct stat st;
    fstat(filed,&st);
    sprintf(len,"Content-Length: %d\n\n",(int) st.st_size);
    // Send the headers
    write(fd,responseStatus,strlen(responseStatus));
    write(fd,responseOther,strlen(responseOther)); // I dont know how but this line was giving error every once 
                                                      //in a while so i just commented this out  and progem still runs
    write(fd,len,strlen(len));
    // Send the file
    while(bytesRead = read(filed,buffer,1023)) {
      write(fd,buffer,bytesRead);
    }
    close(filed);
  }
  close(fd);
}


int main() {
  // Create the socket
  int server_socket = socket(AF_INET , SOCK_STREAM , 0);
  if (server_socket == -1) {
    printf("Could not create socket.\n");
    return 1;
  }

  //Prepare the sockaddr_in structure
  struct sockaddr_in server;
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons( 8888 );

  // Bind to the port we want to use
  if(bind(server_socket,(struct sockaddr *)&server , sizeof(server)) < 0) {
    printf("Bind failed\n");
    return 1;
  }
  printf("Bind done\n");

  // Mark the socket as a passive socket
  listen(server_socket , 3);

  // Accept incoming connections
  printf("Waiting for incoming connections...\n");
  while(1) {
    struct sockaddr_in client;
    int new_socket , c = sizeof(struct sockaddr_in);
    new_socket = accept(server_socket, (struct sockaddr *) &client, (socklen_t*)&c);
    if(new_socket != -1)
      serveRequest(new_socket);
  }

  return 0;
}
