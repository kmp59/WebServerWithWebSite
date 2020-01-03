//////Author:  KEVIN PATEL//////
///// CLAsS PROjeCT CS 288 Section 003//////
////UCID: 31369580////////
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <fcntl.h>
#include <math.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/mman.h>
#define MAX_BUFFER 2048 // buffer size
#define ROOT_WEB "/home/kmp59"
#define BACKLOG 10

/////////////////// Predefined Constant Values//////////////////////////////
const char *HTTP_200_STRING = "OK";
const char *HTTP_404_STRING = "Not Found";
const char *HTTP_501_STRING = "Not Implemented";
const char *HTTP_404_CONTENT = "\
<html> \
<head> \
<title>404 Not Found</title> \
</head> \
<body><h1>404 Not Found</h1>The requested \
resource could not be found but may be available again in the \
future.<div style=\"color: #eeeeee; font-size: 8pt;\">Actually, \
it probably won't ever be available unless this is showing up \
because of a bug in your program. :</div></html>";
const char *HTTP_501_CONTENT = "<html><head><title>501 Not \
Implemented</title></head><body><h1>501 Not Implemented</h1>The \
server either does not recognise the request method, or it lacks \
the ability to fulfill the request.</body></html>";

////////////////// file extensions type ////////////////////////////////////
struct
{
  char *ext;
  char *filetype;
} extns[] = {
  {"jpg", "image/jpeg"},
  {"gif", "image/gif"},
  {"png", "image/png"},
  {"html", "text/html"},
  {"ico", "image/webp"},
  {"css", "text/css"},
  {"js", "*/*"}};
  ////////////////////////////////////Main method////////////////////////////////
  int main(int argc, char const *argv[])
  {
    ////////////////////////////////////Checking the CLI/////////////////////////
    if (argc != 2)
    {
      fprintf(stderr, "usage: %s <port> undeclared\n", argv[0]);
      exit(1);
    }
    int Port = atoi(argv[1]);
    int SocketPar, childSocket, pid, j;
    struct sockaddr_in addr;
    int addrlen = sizeof(addr);
    /////////////////////////Creating Socket/////////////////////////
    if ((SocketPar = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
      perror("Socket creation failed");
      exit(EXIT_FAILURE);
    }
    int value = 1;
    setsockopt(SocketPar, SOL_SOCKET, SO_REUSEADDR,(const void *)&value, sizeof(int));
    bzero((char *)&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(Port);

    memset(addr.sin_zero, '\0', sizeof addr.sin_zero);
    ///////////////////////// Checking if port is already in use//////////////////
    if (bind(SocketPar, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
      printf("\nPort number[%d] is in use....", Port);
      exit(0);
    }

    printf("\n Server is running on PORT = %d", Port);
    /////////////////////////Socket listening error Checking/////////////////////////
    if (listen(SocketPar, BACKLOG) < 0)
    {
      exit(0);
    }
    /////////////////////////Accept new requests until the connection is closed/////
    while (1)
    {
      /////////////// Get requestS from Client//////////////////////
      if ((childSocket = accept(SocketPar, (struct sockaddr *)&addr, (socklen_t *)&addrlen)) < 0)
      {
        exit(0);
      }

      long inpval, i;
      int j, file_fd, buflen, len;
      char *fstr;
      char buffer[MAX_BUFFER + 1] = {0};
      inpval = read(childSocket, buffer, 30000);

      if (inpval == 0 || inpval == -1)
      {
        puts("failed to read browser code\n");
        exit(0);
      }

      /////////////prints the browser request///////////////////////
      printf("\nRequest From Browser\n%s", buffer);

      if (inpval > 0 && inpval < MAX_BUFFER)
      buffer[inpval] = 0;
      else
      buffer[0] = 0;
      for (i = 0; i < inpval; i++)
      {
        if (buffer[i] == '\r' || buffer[i] == '\n')
        buffer[i] = '*';
      }
      /////////501 error if the request is not GET////////////////
      if (strncmp(buffer, "GET ", 4))
      {
        sprintf(buffer, "HTTP/1.0 501 %s\r\nContent-Type: %s\r\n\r\n%s", HTTP_501_STRING, "text/html", HTTP_501_CONTENT);
        write(childSocket, buffer, strlen(buffer));
        close(childSocket);
        continue;
      }
      for (i = 4; i < MAX_BUFFER; i++)
      {
        if (buffer[i] == ' ')
        {
          buffer[i] = 0;
          break;
        }
      }
      /////////////////// GEts the html file////////////////////////
      if (!strncmp(&buffer[0], "GET /\0", 6) || !strncmp(&buffer[0], "get /\0", 6))
      strcpy(buffer, "GET /index.html");
      buflen = strlen(buffer);
      fstr = (char *)0;
      /////////////////// GEts the icon file////////////////////////
      if (strcmp(&buffer[buflen - 3], "ico") == 0)
      {
        write(childSocket, "HTTP/1.1 200 OK\n", 16);
        write(childSocket, "Content-length: 46\n", 19);
        write(childSocket, "Content-Type: image/webp\n\n", 25);
        write(childSocket, "", 0);
      }
      else
      {
        for (i = 0; extns[i].ext != 0; i++)
        {
          len = strlen(extns[i].ext);
          if (!strncmp(&buffer[buflen - len], extns[i].ext, len))
          {
            fstr = extns[i].filetype;
            break;
          }
        }
        if (fstr == 0)
        {
          printf("\n Not supported file extension \n");
          exit(0);
        }
        char filename[4096];
        int status = 200;
        snprintf(filename, sizeof filename, "%s/%s", ROOT_WEB, &buffer[5]);
        printf("\nFilename = %s\n", filename);
        ///////////404 File Not Found error////////////////////////////////
        if ((file_fd = open(filename, O_RDONLY)) == -1)
        {
          sprintf(buffer, "HTTP/1.0 404 %s\r\nContent-Type: %s\r\n\r\n%s", HTTP_404_STRING, "text/html", HTTP_404_CONTENT);
          write(childSocket, buffer, strlen(buffer));
          close(childSocket);
          continue;
        }
        ///////////////////display the file in the browser/////////////////////
        sprintf(buffer, "HTTP/1.0 200 %s\r\nContent-Type: %s\r\n\r\n", HTTP_200_STRING, fstr);
        write(childSocket, buffer, strlen(buffer));
        while ((inpval = read(file_fd, buffer, MAX_BUFFER)) > 0)
        {
          write(childSocket, buffer, inpval);
        }
        close(childSocket); // close sockets
      }
    }
    return 0;
  }
