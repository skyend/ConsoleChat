#include "protocol.h"



void error_handling(char * message);

int main(int argc, char *argv[])
{
  int sock;
  char message[BUF_SIZE];
  int str_len,recv_len;
  struct sockaddr_in serv_adr;
  int recv_cnt;

  if(argc != 3){
    printf("Usage: %s <IP> <port> \n", argv[0]);
    exit(1);
  }

  sock = socket(PF_INET, SOCK_STREAM, 0);
  if( sock == -1 )
    error_handling("socket() error");

  memset(&serv_adr, 0, sizeof(serv_adr));
  serv_adr.sin_family = AF_INET;
  serv_adr.sin_addr.s_addr = inet_addr(argv[1]);
  serv_adr.sin_port = htons(atoi(argv[2]));

  if( connect(sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1 )
    error_handling("connect() error!");
  else
    puts("Connected...............");

  // 이름등록
  {
    pro_reg_name pro_name;
    int a = 0;

    fputs("Input Your name : ", stdout );
    scanf(" %s",pro_name.name);

    stream_sender( sock, REGISTER_NAME, (void*)&pro_name, sizeof(pro_reg_name));
  }

  {// 리스트보기
    s_header recv_header;
    char body_tmp[MAX_STREAM_BODY_SIZE];
    int a;
    int read_len;

    // 테스트 리스트 요청
    for(a=0 ; a < 10000000; a++); // 시간지연
    stream_sender( sock, REQUEST_LIST, &tmpbyte, 1);
    
    // read header
    read(sock, &recv_header, sizeof(s_header));
    printf("body size :%d \n", recv_header.stream_body_size);


    // read body 
    read_len = read(sock, body_tmp, MAX_STREAM_BODY_SIZE);

    // 디버깅용 출력
    printf("read len : %d , info [%d]\n", read_len, (int)(recv_header.stream_body_size / sizeof(client_info)) );
    
    
    // 구조체로 형변환해서 접근하기위해
    for( a = 0 ; a < recv_header.stream_body_size / sizeof(client_info) ; a++){

      // 메모리 영역 구조체로 형변환해서 순회
      client_info *cinfo_ptr = ((client_info*)body_tmp) + a; // 구조체 포인터
      
      // 구조체 정보 출력
      printf("Client \n\tname : %s \n\tip : %s \n", cinfo_ptr->name, cinfo_ptr->IP);
    }
    
  }
  
  while(1)
    {
      fputs("Input message(Q to quit): ", stdout);
      scanf("\n");
      fgets(message, BUF_SIZE, stdin);
      if(!strcmp(message, "q\n") || !strcmp(message, "Q\n"))
	break;
      {
	s_header head;
	head.STS = 0xFF;
	head.type = DEBUG_MESSAGE;
	head.stream_body_size = strlen(message);
	head.complete = true;
	
	write(sock, &head, sizeof(s_header));
	
      }
      
      str_len = write(sock, message, strlen(message));
      //str_len = read(sock, message, BUF_SIZE-1);
      //message[str_len] = 0;
      
      recv_len = 0;
      while(recv_len < str_len)
	{
	  recv_cnt = read(sock, &message[recv_len], BUF_SIZE -1);
	  if(recv_cnt == -1)
	    error_handling("read() error!");
	  recv_len += recv_cnt;
	}
      
      message[recv_len] = 0;
      
      
      
      printf("Message from server: %s", message);

    }

  close(sock);
  return 0;
}


void error_handling(char *message)
{
  fputs(message, stderr);
  fputc('\n', stderr);
  exit(1);
}
