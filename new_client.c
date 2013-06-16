#include "protocol.h"
#include "client.h"

int main ( int argc, char * argv[] )
{
  printf("Using %d Byte \n", (int)sizeof(clients));

  /* Socket */
  int sock;

  struct sockaddr_in serv_adr;
  struct sockaddr_in clnt_adr;

  socklen_t clnt_adr_size;
  socklen_t adr_sz;
  
  /* Multi */
  struct timeval timeout;
  fd_set reads, temps;
  int fd_max, packet_len, fd_num, i;
  char buf[BUF_SIZE];
  

  // Client Variable
  client_state state = cstate_command_input;
  client client; // 클라이언트 구조체 ( 자기자신 )


  if(argc != 3){
    printf("Usage: %s <IP> <port> \n", argv[0]);
    exit(1);
  }

  sock = socket(PF_INET, SOCK_STREAM, 0);
  if ( sock == -1 )
    error_handling("socket() error");
  
  memset( &serv_adr, 0, sizeof( serv_adr ));
  serv_adr.sin_family = AF_INET;
  serv_adr.sin_addr.s_addr = inet_addr(argv[1]);
  serv_adr.sin_port = htons(atoi(argv[2]));

  if( connect(sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1 )
    error_handling("connect() error!");
  else
    puts("Connected...............\n");


  // -------------------------- SOCKET SETTING ----------------------------------


  

  // reads 초기화
  FD_ZERO( &reads );

  // Standard Input 감시대상에 추가
  FD_SET(0, &reads);

  // 서버 소켓 감시 대상 추가
  FD_SET(sock, &reads);
  
  fd_max = sock;

  while(1){
    temps = reads;
    timeout.tv_sec = 100;
    timeout.tv_usec = 5000;
    
    print_by_state(state);
    
    if( (fd_num = select( fd_max+1, &temps, 0, 0, &timeout)) == -1 )
      break;
    
    if( fd_num == 0 )
      continue;
    
    for( i = 0; i <fd_max + 1; i++ )
      {
	// 모든 FD 순회하면서 데이터 체크
	if( FD_ISSET(i, &temps))
	  {
	    client_command command;
	    
	    if( i == 0 ){ // Input
	      
	      // 클라이언트 상태별 처리분기
	      switch(state){
	      case cstate_command_input :
		{
		  scanf(" %d", (int*)&command);
		  state = command;
		  
		  switch(command){
		  case ccmd_register_name :
		    
		    state = cstate_input_new_name;
		    break;
		  default :
		    break;
		  }
		}
		break;

	      case cstate_input_new_name :
		{
		  pro_reg_name pro_name;
		  scanf(" %s",pro_name.name);
		  stream_sender( sock, REGISTER_NAME, (void*)&pro_name, sizeof(pro_reg_name));
		  state = cstate_command_input;
		}
		break;
		
		
	      default :
		fgets(buf, BUF_SIZE, stdin);
		state = cstate_command_input;
		break;
		
	      }
	      
	
	      
	    } else if ( i == sock ){ // Read
	      
	    }
	  }
      }
  }
  
  // -------------------------- PROGRAM END ------------------------------------
  //close(clnt_sock);
  close(sock);
  return 1;
}

void print_by_state(client_state state)
{
  switch ( state ){
  case cstate_command_input :
	
    printf("------------------- Commands --------------------\n");
    printf("\t%d. 이름 등록(로그인) \n", ccmd_register_name);
    printf("\t%d. 채팅 요청 \n", ccmd_request_chat);
    printf("\t%d. 로그아웃 \n", ccmd_logout);
    printf("\t%d. 뭐할까 \n", ccmd_why);
    printf("\t%d. 파일전송 \n", ccmd_file_transfer);
    printf("\t%d. 종료 \n", ccmd_exit);
    printf("-------------------------------------------------\n\n");
    
    printf(" 명령어 번호를 입력하세요 :\n");
    
    break;
  case cstate_input_new_name :
    
    printf("이름을 입력하세요 : \n");
    
    
    break;
  default :
    break;
  }
      
}

void error_handling(char *message )
{
  fputs( message, stderr);
  fputc('\n', stderr);
  exit(1);
}

// return 
//   client or NULL
client* c_search_from_fd(int fd)
{
  client * _client = NULL;
  int i;
  
  // 클라이언트 구조체 배열을 순회하여 모든 클라이언트 구조체에 접근
  for( i = 0; i < MAX_CON_USERS ; i++ ){
    
    // 해당인덱스에 있는 구조체 가져오기(참조)
    _client = &clients[i];
    
    // 현재 인덱스에 있는 구조체가 찾는 구조체라면 포문종료
    if( fd == _client->fd && _client->available == true )
      break;
    else 
      _client = NULL; // 찾는 구조체가 아니라면 참조한 구조체 참조취소
  }
  
  // 찾은 구조체 주소 반환
  return _client; 
}

// 패킷 누적함수 :
/*
  패킷은 스트림을 이루는 연속된 바이트이다.
  스트림은 헤더와 바디 엔더로 나누어 지며
  헤더의 크기와 엔더의 크기는 고정이며
  바디의 크기는 가변이다. 바디의 크기는 헤더에 명시되어 있다.


 */
stream_state stream_put_to_ci(client * _client, void *data, int size )
{
  int i;
  int seeker = 0; // 현재까지 패킷을 읽은 인덱스 위치
  int body_seeker = 0; // 현재까지 읽은 스트림바디의 인덱스 위치
  
  while(seeker < size){
    
    
    /* 클라이언트 패킷이 비어 있고
       스트림헤더데이터가 완료되지 않고
       받은 데이터가 스트림헤더의 전체 길이보다 크거나 같을 경우
       헤더부분만 잘라옴 */
    if( 
       _client->stream_state == EMPTY            &&
       //client->stream_header.complete != true   &&
       size >= sizeof(s_header) 
	)
      {
	
	// 패킷에서 헤더부분 복사
	memcpy(&_client->stream_header, data, sizeof(s_header));
	
	// 헤더 검증
	if( _client->stream_header.complete == true ){
	  
	  // 유효한 헤더라면 헤더 정보 출력
	  printf("- receive header\n");
	  print_s_header( &_client->stream_header );
	  
	  // body 데이터를 받을 준비 ( 초기화 )
	  _client->stream_body_length = 0;
	  _client->stream_state = STREAM_REMAIN;
	  memset( _client->stream_body, 0, MAX_STREAM_BODY_SIZE );
	  
	  // seeker 이동 ( 헤더의 끝으로 )
	  seeker = sizeof(s_header);
	} else {
	  // 유효하지 않은 헤더라면 프로그램 종료
	  error_handling("유효하지 않은 헤더입니다.");
	}
      } 
    else if ( _client->stream_state == STREAM_REMAIN ){
      /* 받을 스트림이 더 남아 있다면
	 stream body 적재 */
      
      // 현재까지 받은 stream body 길이가 최대 길이 보다 크거나 같다면
      // 더이상 받지 않고 경고 출력
      if( _client->stream_body_length >= MAX_STREAM_BODY_SIZE ){
	printf("warn : Over stream body \n");
	
	_client->stream_state = STREAM_ERROR;
	return STREAM_ERROR;
      }
      
      // 주소를 계산해 이어서 한 바이트씩 적재
      memcpy(  
	     _client->stream_body + 
	     _client->stream_body_length ,
	     data + seeker, 1 );
      
      //누적 길이 증가
      _client->stream_body_length++;

      // 받을 사이즈와 현재 사이즈가 같다면
      if( _client->stream_body_length == _client->stream_header.stream_body_size ){
	printf("- stream body all received \n");

	// 스트림 완료
	_client->stream_state = STREAM_END;
	return STREAM_END;
      }
      
      // seeker 증가
      seeker++;
      
      // body_seeker 증가
      //body_seeker++; // 이게 필요한가?
    } else { // 그 외의 상황이라면 에러로 간주
      
      error_handling("stream error");
    }
    
  }
  
  _client->stream_state = STREAM_REMAIN;
  return STREAM_REMAIN;
}

// 스트림 처리 함수 :  스트림이 모두 도착하면 이 함수를 실행에 스트림정보에 따른
// 처리를 수행한다.
result stream_interpreter(client *_client )
{
  printf("[func] Stream interpreting.. \n");
  

  // 스트림의 타입에 따른 처리 분기
  switch ( _client->stream_header.type ){
    // 디버그 메세지 
  case DEBUG_MESSAGE :
    {
      printf("- DEBUG MESSAGE: \n\t%s\n\tfrom:%s[%s]\n", _client->stream_body, _client->IP, _client->name );
    }
    break;

  DEFAULT :
    break;
  }
  
  _client->stream_state = EMPTY;
  
  return SUCCESS;
}

/* Util functions */

// 헤더 정보 출력
void print_s_header(s_header *header)
{
  printf("- header DESC\n\tSTS: 0x%x \n\ttype: %d\n\tstream_body_size: %d\n\tcomplete: %d\n", (int)header->STS, (int)header->type, (int)header->stream_body_size, (int)header->complete);
}

// 서버에서 사용하는 함수
void error_sender(int fd, error_code error)
{

  pro_error_return per;
  // 에러코드 세팅
  per.error = sent_chat_accept_to_invalid_client;
  // 에러 보냄
  stream_sender(fd, ERROR_RETURN, (void*)&per, sizeof(pro_error_return));

}
