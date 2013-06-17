#include "protocol.h"
#include "client.h"

#define DEBUG 0

// Client Variable
client_state state;



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

  // Client 구조체 초기화
  client.fd = sock;
  client.stream_state = EMPTY;
  client.available = true;
  
  state = cstate_command_input;  



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
    
    print_by_state(state, &client);
    
    //    if( (fd_num = select( fd_max+1, &temps, 0, 0, &timeout)) == -1 )
    //      break;

    if( (fd_num = select( fd_max+1, &temps, 0, 0, NULL)) == -1 )
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

		// 명령 선택 상태
	      case cstate_command_input :
		{
		  scanf(" %d", (int*)&command);
		  state = command;
		  
		  // 선택한 명령의 번호에 따른 처리 분기
		  switch(command){
		  case ccmd_register_name : // 이름등록

		    state = cstate_input_new_name;
		    // 완료
		    break;
		    
		  case ccmd_request_chat : // 채팅요청
		    {
		      pro_req_matching_chat prmc;
		      
		      if( request_chat_dialog( &prmc ) ){
			
			
			stream_sender(client.fd, REQUEST_MATCHING_CHAT,
				      &prmc, sizeof(pro_req_matching_chat));

			// 채팅 응답 대기 상태로 변경
			state = cstate_wait_for_response_conn;
		      }

		    }
		    break;
		    
		  case ccmd_logout : // 로그아웃

		    //state = cstate_input_new_name;
		    break;
		    
		  case ccmd_client_list : // 클라이언트리스트
		    
		    state = cstate_wait_for_client_list;
		    
		    // 클라이언트 리스트 요청 
		    request_client_list(sock);
		    break;

		  case ccmd_file_transfer : // 파일전송
		    
		    //state = cstate_input_new_name;
		    break;
		    
		  case ccmd_exit : // 종료
		    
		    exit(1);
		    break;

		  default :
		    break;
		  }
		}
		break;
		
		// 이름입력 상태일때
	      case cstate_input_new_name :
		{
		  pro_reg_name pro_name;
		  scanf(" %s",pro_name.name);
		  stream_sender( sock, REGISTER_NAME, (void*)&pro_name, sizeof(pro_reg_name));
		  state = cstate_command_input;

		  memcpy(client.name, pro_name.name, NAME_SIZE);
		}
		break;
		
	      case cstate_chat :
		{
		  pro_transfer_chat_message *ptcm = malloc(sizeof(pro_transfer_chat_message));
		  
		  // 메세지 입력받음
		  //fgets(ptcm->message, 1, stdin);
		  //scanf(" %s",ptcm->message);

		  //fprintf(stdout, "Enter msg: ");
		  //fgets(ptcm->message, MESSAGE_MAX_SIZE, stdin);

		  // 줄바꿈 문자 치환
		  //ptcm->message[strlen(ptcm->message)] = '\0';

		  //printf("Input \n");
		  
		  // stdin 읽기
		  read(0, ptcm->message, MESSAGE_MAX_SIZE);

		  // 마지막 개행문자를 널문자로 변경
		  ptcm->message[strlen(ptcm->message)-1] = '\0';
		  
		  //printf("length : %d \n", (int)strlen(ptcm->message));
		  
		  // 첫번째 문자가 q나 Q이고 두번째 문자가 널문자이면
		  if( (*(ptcm->message) == 'Q' || *(ptcm->message) == 'q') &&
		      (strlen(ptcm->message)) == 1 ){
		    // 채팅 종료 통보

		    
		    stream_sender(client.fd, NOTICE_CHAT_END,
				  &tmpbyte, sizeof( char ) );
		    
		    // 상태 되돌리기
		    //state = cstate_command_input;
		    
		  }
		  
		  //printf("%s\n", ptcm->message);
		  
		  stream_sender(client.fd, SEND_MESSAGE_TO_ONE,
				ptcm, sizeof( pro_transfer_chat_message ) );
		  
		}
		break;
	      default :
		fgets(buf, BUF_SIZE, stdin);
		//state = cstate_command_input;
		break;
		
	      }
	      
	      
	      
	    } else if ( i == sock ){ // Read
	      /*
		READ
	      */
	      packet_len = read(i , buf, BUF_SIZE);
	      
	      if( packet_len == 0 ) {
		
		// 소켓 닫기
		close( sock );
		// 연결 종료 알림
		printf("closed from server \n");
		break;
	      }
	      else{
		stream_state stream_result;
		
		// 패킷 저장
		stream_result = stream_put_to_ci(&client, buf, packet_len);
		
		// 패킷 저장후 패킷 스트림 상태에 따라 처리
		if( stream_result == STREAM_END ){
		  stream_interpreter(&client);
		}
		
		
	      }
	    }
	  }
      }
  }
  
  // -------------------------- PROGRAM END ------------------------------------
  //close(clnt_sock);
  close(sock);
  return 1;
}



void print_by_state(client_state state, client* _client)
{
  switch ( state ){
  case cstate_command_input :
    printf("NAME : [%s] \n", _client->name); // 클라이언트 이름 출력
    printf("------------------- Commands --------------------\n");
    printf("\t[%d] 이름 등록(로그인) \n", ccmd_register_name);
    printf("\t[%d] 채팅 요청 \n", ccmd_request_chat);
    printf("\t[%d] 로그아웃 \n", ccmd_logout);
    printf("\t[%d] 접속 클라이언트 리스트 보기 \n", ccmd_client_list);
    printf("\t[%d] 파일전송 \n", ccmd_file_transfer);
    printf("\t[%d] 종료 \n", ccmd_exit);
    printf("-------------------------------------------------\n\n");
    
    printf(" -> 명령어 번호를 입력하세요 :\n");
    
    break;
  case cstate_input_new_name :
    printf("---------------------------------------\n");
    printf("| 이름을 입력하세요                   |\n");
    printf("---------------------------------------\n");
    
    break;
  case cstate_wait_for_client_list :
    printf("---------------------------------------\n");
    printf("| 클라이언트 리스트를 기다리는 중입니다.\n");
    printf("---------------------------------------\n");
    
    break;
  case cstate_wait_for_response_conn :
    printf("---------------------------------------\n");
    printf("| 상대방의 응답을 기다리는 중입니다 .\n");
    printf("---------------------------------------\n");

  case cstate_wait_for_server_response :
    printf("---------------------------------------\n");
    printf("| 서버 응답을 기다리는 중입니다. \n");
    printf("---------------------------------------\n");
    
  case cstate_chat :
    printf("---------------------------------------\n");
    printf("| 나:%s 상대:%s \n",_client->name, _client->p2p_name);


  default :
    break;
  }
      
}

void request_client_list(int fd)
{
  
  
  stream_sender( fd, REQUEST_LIST, &tmpbyte, 1);
  
}


void error_handling(char *message )
{
  fputs( message, stderr);
  fputc('\n', stderr);
  exit(1);
}
 
 
 
/*
  - 패킷 누적함수 -
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
	  if (DEBUG) printf("- receive header\n");
	  if (DEBUG) print_s_header( &_client->stream_header );
	  
	  // body 데이터를 받을 준비 ( 초기화 )
	  _client->stream_body_length = 0;
	  _client->stream_state = STREAM_REMAIN;
	  memset( _client->stream_body, 0, MAX_STREAM_BODY_SIZE );
	  
	  // seeker 이동 ( 헤더의 끝으로 )
	  seeker = sizeof(s_header);
	} else {
	  // 유효하지 않은 헤더라면 프로그램 종료
	  //error_handling("유효하지 않은 헤더입니다.");
	}
      } 
    else if ( _client->stream_state == STREAM_REMAIN ){
      /* 받을 스트림이 더 남아 있다면
	 stream body 적재 */
      
      // 현재까지 받은 stream body 길이가 최대 길이 보다 크거나 같다면
      // 더이상 받지 않고 경고 출력
      if( _client->stream_body_length >= MAX_STREAM_BODY_SIZE ){
	
	  if (DEBUG)  printf("warn : Over stream body \n");
	
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

	if (DEBUG) printf("- stream body all received \n");

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
  if (DEBUG) printf("[func] Stream interpreting.. \n");
  

  // 스트림의 타입에 따른 처리 분기
  switch ( _client->stream_header.type ){
    // 디버그 메세지 
  case DEBUG_MESSAGE :
    {
      if (DEBUG) printf("- DEBUG MESSAGE: \n\t%s\n\tfrom:%s[%s]\n", _client->stream_body, _client->IP, _client->name );
    }
    break;

  case RESPONSE_LIST :
    {
      //  리스트 데이터 출력
      int a;

      printf("Client List ====================\n");
      for( a = 0 ; a < _client->stream_header.stream_body_size / sizeof(client_info) ; a++){
	
	// 메모리 영역 구조체로 형변환해서 순회
	client_info *cinfo_ptr = ((client_info*)_client->stream_body) + a; // 구조체 포인터
	
	// 구조체 정보 출력
	printf("\tname : %s \n\tip : %s \n--------------------------\n", 
	       cinfo_ptr->name, cinfo_ptr->IP);
      }
      
      state = cstate_command_input;
    }
    break;
  case SUGGEST_CHAT :
    {
      pro_sug_chat *psc = (pro_sug_chat*)_client->stream_body;
      
      printf("[%s]님으로 부터 채팅요청이 왔습니다.\n 수락 하시겠습니까?\n", psc->who_name);
      
      // 수락할 경우 
      if( yes_and_no() ){
	pro_accept_chat pac;
	strcpy(pac.who_name, psc->who_name);
	pac.agree = true;

	// 서버응답 대기상태로 변경
	state = cstate_wait_for_server_response;


	// 수락 메세지 전송 
	stream_sender(_client->fd, RESPONSE_ACCEPT_CHAT,
		      &pac, sizeof(pro_accept_chat));
	

      } else {
	
	pro_accept_chat pac;
	strcpy(pac.who_name,_client->name);
	pac.agree = false;
	
	state = cstate_command_input;

	// 거절 메세지 전송 
	stream_sender(_client->fd, RESPONSE_ACCEPT_CHAT,
		      &pac, sizeof(pro_accept_chat));
	

      }

    }
    break;
  case NOTICE_CHAT_CONNECT :
    {
      pro_notice_chat_connect *pncc = (pro_notice_chat_connect*)_client->stream_body;
      
      if( pncc->conn_state == connected ){
	printf("=== [%s] 님과의 채팅이 시작되었습니다. ===\n", pncc->who_name );
	printf("= 채팅을 그만두시려면 : 'Q/q' 를 입력하여 주세요. \n");
	
	// 공지 후 채팅 모드로
	state = cstate_chat;

	// 대상 입력
	memcpy(_client->p2p_name,pncc->who_name, NAME_SIZE);
	_client->p2p_conn_state = connected;

      } else if ( pncc->conn_state == broken ){
	
	printf("=== [%s] 님과의 채팅이 끝났습니다. ===\n", _client->p2p_name );
	printf(" 메인으로 돌아갑니다. \n");

	// p2p 이름 초기화
	memset(_client->p2p_name,0, NAME_SIZE);
	
	// 연결 해제 상태
	_client->p2p_conn_state = broken;
	
	state = cstate_command_input;
      }
    }
    break;
    
  case TRANSFER_CHAT_MESSAGE :
    {
      pro_transfer_chat_message *ptcm = (pro_transfer_chat_message*)_client->stream_body;
      
      //printf("[%s] : %s \n", ptcm->who_name, ptcm->message );
      printf("상대방: \"%s\"\n",  ptcm->message );
      
    }
    break;
  default :
    break;
  }
  
  _client->stream_state = EMPTY;
  
  return SUCCESS;
}


bool request_chat_dialog( pro_req_matching_chat *prmc )
{
  //char name[NAME_SIZE];
  printf("채팅을 요청할 사람의 이름을 입력하세요 \n");
  
  scanf(" %s", prmc->name);

  printf("당신이 채팅을 요청할 클라이언트 이름이 %s가 맞습니까?\n", prmc->name);
  
  return yes_and_no();
}

bool yes_and_no(){
  char A[2];
  printf("\t예(y) / 아니오(n)\n");
  
  scanf(" %s", A);


  if( strcmp("y",A) == 0 || strcmp("Y",A) == 0 ){
    return true;
  } else if ( strcmp("n", A) == 0 || strcmp("N",A) == 0){
    return false;
  }
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
