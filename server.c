#include "protocol.h"

int main ( int argc, char * argv[] )
{
  printf("Using %d Byte \n", (int)sizeof(clients));

  /* Socket */
  int serv_sock;
  int clnt_sock;

  struct sockaddr_in serv_adr;
  struct sockaddr_in clnt_adr;

  socklen_t clnt_adr_size;
  socklen_t adr_sz;
  
  /* Multi */
  struct timeval timeout;
  fd_set reads, temps;
  int fd_max, packet_len, fd_num, i;
  char buf[BUF_SIZE];



  if( argc != 2 ){
    printf("Usage : %s <port> \n", argv[0]);
    exit(1);
  }

  serv_sock = socket(PF_INET, SOCK_STREAM, 0);
  if ( serv_sock == -1 )
    error_handling("socket() error");
  
  memset( &serv_adr, 0, sizeof( serv_adr ));

  serv_adr.sin_family = AF_INET;
  serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_adr.sin_port = htons(atoi(argv[1]));

  if( bind(serv_sock, (struct sockaddr*) &serv_adr, sizeof(serv_adr)) == -1 )
    error_handling("bind() error");

  if( listen(serv_sock, 5 ) == -1)
    error_handling("listen() error");
  // -------------------------- SOCKET SETTING ----------------------------------
  

  // reads 초기화
  FD_ZERO( &reads );
  FD_SET(0, &reads);

  // 서버 소켓 감시 대상 추가
  FD_SET(serv_sock, &reads);
  fd_max = serv_sock;

  while(1){
    temps = reads;
    timeout.tv_sec = 5;
    timeout.tv_usec = 5000;
    
    if( (fd_num = select( fd_max+1, &temps, 0, 0, &timeout)) == -1 )
      break;
    
    if( fd_num == 0 )
      continue;
    
    for( i = 0; i <fd_max + 1; i++ )
      {
	// 모든 FD 순회하면서 데이터 체크
	if( FD_ISSET(i, &temps))
	  {
	    
	    // 현재 fd가 서버 소켓이라면
	    if( i == serv_sock ){
	      adr_sz = sizeof( clnt_adr );
	      
	      // 연결요청 허용해 주고 fd 얻음
	      clnt_sock =
		accept( serv_sock, ( struct sockaddr*)&clnt_adr, &adr_sz );
	      printf("accept \n");

	      // 클라이언트 등록 시도
	      if( register_client(clnt_sock, getIP_from_sockaddr_in(&clnt_adr)) 
		  == SUCCESS ){
		// IF 블럭
		
		// 새로운 소켓 감시 대상으로 추가
		FD_SET(clnt_sock, &reads);
		
		// 새로운 fd를 제일 높은 fd로 설정 (단, 제일높은 fd보다 높을 경우)
		if( fd_max < clnt_sock )
		  fd_max = clnt_sock;
		
		// 연결 알림
		printf("connected client: %d \n", clnt_sock);
		
	      } else {
		// 등록 실패시 연결 취소
		// 실패 사유는 클라이언트 최대 동시접속을 초과했을 때
		
		close(clnt_sock);
		printf("can't connect client: %d \n\treason: over concurrent\n", clnt_sock);
	      }
	      
	    }

	    // FD가 서버소켓이 아니고 클라이언트 소켓이라면
	    else { 
	      // fd 로 클라이언트 구조체 찾기
	      client *_client = c_search_from_fd(i);
	      
	      
	      if( _client != NULL )
		printf( "\tarrived packet from :%s \n", _client->IP );
	      
	      
	      
	      // 패킷가져옴
	      packet_len = read(i , buf, BUF_SIZE);
	      //printf("arrived text : %s \n", buf );
	      
	      
	      
	      // EOF가 도착하면 연결이 끊어진 것 이므로 클라이언트 소켓을 닫고
	      // 클라이언트 구조체를 비활성화 한다.
	      if( packet_len == 0 ) {
		
		// 해당 클라이언트fd를 감시 대상에서 제외
		FD_CLR( _client->fd, &reads );
		
		// 소켓 닫기
		close( _client->fd );
		
		// 구조체 비활성
		_client->available = false;
		
		// 연결 종료 알림
		printf("closed client: %d \n", _client->fd );
		
	      }
	      else{ 
		stream_state stream_result;
		
		// 패킷 저장
		stream_result = stream_put_to_ci(_client, buf, packet_len);
		
		// 패킷 저장후 패킷 스트림 상태에 따라 처리
		if( stream_result == STREAM_END ){
		  stream_interpreter(_client);
		} 
		
		  
		
	      }
	    }
	  }
      }
  }
  
  // -------------------------- PROGRAM END ------------------------------------
  //close(clnt_sock);
  close(serv_sock);
  return 1;
}

void error_handling(char *message )
{
  fputs( message, stderr);
  fputc('\n', stderr);
  exit(1);
}


result register_client(int client_fd, char *IP){

  // 연결이 되었음으로 새로운 클라이언트 정보를 담을 구조체 할당
  // clients 배열의 처음부터 안쓰는 클라이언트 구조체 검색
  // 검색후 해당 구조체에 정보 입력
  int search_i;
  client *_client;
  int empty_index = -1;
  
  // 구조체 배열 순회
  for ( search_i = 0; search_i < MAX_CON_USERS ; search_i++){
    // 현재 인덱스에 해당하는 구조체 주소 저장
    _client = &clients[search_i];
    
    // 사용중이 아닌 구조체를 찾으면 인덱스값 저장후 for문 종료
    if( _client->available == false ){
      empty_index = search_i;
      break;
    }
    
  }
  
  // 결과 처리후 반환
  if( empty_index == -1 ){
    return FAIL;
  } else {
    // 구조체를 사용하기전에 이전 데이터 비우기
    memset(_client, 0, sizeof(client));

    // 검색된 비활성 구조체에 데이터 입력
    _client->fd = client_fd;
    _client->available = true;   // 활성상태로 변경
    strcpy(_client->IP, IP); // IP입력
    _client->stream_state = EMPTY;

    return SUCCESS;
  }
}

char* getIP_from_sockaddr_in(struct sockaddr_in *addr_in)
{
  char *IP = malloc(sizeof(char)*20);

  strcpy( IP,(char*) inet_ntoa( addr_in->sin_addr ) );

  return IP;  
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


client *c_search_from_name(char *name)
{
  client * _client = NULL;
  int i;
  
  // 클라이언트 구조체 배열을 순회하여 모든 클라이언트 구조체에 접근
  for( i = 0; i < MAX_CON_USERS ; i++ ){
    
    // 해당인덱스에 있는 구조체 가져오기(참조)
    _client = &clients[i];
    
    // 현재 인덱스에 있는 구조체가 찾는 구조체라면 포문종료
    if( strcmp(_client->name, name) == 0 && _client->available == true )
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

    // 채팅 요청 [클라1(처음요청)] -> [서버] -> [클라2(대상)]
  case REQUEST_MATCHING_CHAT :
    {
      /*
	클라이언트가 채팅 요청을 하면 상대 클라이언트를 찾아 
	서버가 의사를 질문하는 스트림을 보낸다
       */
      client *client_ptr = NULL; // 클라2 구조체 포인터
      pro_req_matching_chat *prmc = (pro_req_matching_chat*)_client->stream_body;
      pro_sug_chat psc; // 클라2 에게 보낼 프로토콜 구조체
      
      // 클라2 에 해당하는 구조체를 찾음
      client_ptr = c_search_from_name(prmc->name);
      
      // 프로토콜 구조체 세팅
      // 어떤 클라이언트가 요청 했는지 전달 하기 위해서 
      // 이름 복사 [클라1]의 이름을 대입
      memcpy(psc.who_name, _client->name, NAME_SIZE);
      
      
      
      // 전송
      if( client_ptr != NULL ){
	
	// [클라1] 채팅 연결 상태 지정
	_client->p2p_conn_state = wait;
	// [1] 채팅 상대 이름 저장
	strcpy(_client->p2p_name, prmc->name);
	
	// [클라2]채팅 상대 클라이언트의 채팅 연결 상태를 요청받은 상태로 변경
	client_ptr->p2p_conn_state = recv_reguest;
	
	// [클라2] 에게 의사확인 스트림 전송
	stream_sender(client_ptr->fd, SUGGEST_CHAT, &psc, sizeof(pro_sug_chat));
	
      }else {
	return FAIL;
      }
      
    }
    break;
    
    // 채팅 요청 허가여부 응답 [클라2(대상)] -> [서버] -> [클라1(처음요청)]
  case RESPONSE_ACCEPT_CHAT :
    {
      /*
	채팅요청을 받아 들이는 구조체를 읽어 
	이름에 해당하는 클라이언트의 채팅연결 상태를 변경하고
	현재 클라이언트의 채팅연결 상태를 변경한다.
       */
      
      // 채팅을 처음 요청한 클라이언트의 구조체를 가리킬 포인터
      client * client_ptr = NULL;
      pro_accept_chat *pac = (pro_accept_chat*)_client->stream_body;

      // 대상클라이언트 찾아서 주소 대입 
      client_ptr = c_search_from_name(pac->who_name);
      
      
      printf("- [protocol] RESPONSE_ACCEPT_CHAT \n");
      
      // 응답한 클라이언트가 요청한 클라이언트가 대상과 일치하는지 확인하고
      // 요청한 클라이언트의 상태가 기다리는 중이 아니고
      // 일치하지 않는다면 진행안함
      if( strcmp(client_ptr->p2p_name, _client->p2p_name) != 0 
	  && client_ptr->p2p_conn_state != wait ){

	// 에러 보냄
	error_sender( _client->fd, sent_chat_accept_to_invalid_client );
	
	break;
      }
      
      // 대상 클라이언트가 유효한지 확인
      if( client_ptr != NULL ){
	
	// 채팅요청을 받아 들인다면
	if( pac->agree == true ){
	  pro_notice_chat_connect pncc;
	  pncc.conn_state = connected;

	  /*
	    처음 요청한 클라이언트 (client_ptr 가 가리킴)의 연결 상태를 변경
	    그리고 받아 들이는 클라이언트(_client 가 가리킴)의 연결 상태도 변경
	  */
	  client_ptr -> p2p_conn_state = connected;
	  _client    -> p2p_conn_state = connected;
	  
	  // 서로의 이름을 교환
	  strcpy( _client    -> p2p_name   ,  client_ptr -> name );
	  strcpy( client_ptr -> p2p_name   , _client     -> name );
	  
	  /*
	    요청한 클라이언트와 요청받은 클라이언트에게 서로 연결 되었다는 메세지
	    를 보낸다.
	    
	    PRO_NOTICE_CHAT_CONNECT 프로토콜 사용
	  */
	  
	  /* [요청자] 에게 전송할 데이터 세팅 */
	  strcpy(pncc.who_name, _client -> name );
	  stream_sender(client_ptr->fd, NOTICE_CHAT_CONNECT, &pncc, sizeof( pro_notice_chat_connect));
	  
	  /* [요청받은자] 에게 전송할 데이터 세팅 */
	  strcpy(pncc.who_name, client_ptr -> name );
	  stream_sender(_client->fd, NOTICE_CHAT_CONNECT, &pncc, sizeof( pro_notice_chat_connect));
	  
	  printf("chat connected [%s] <-O-> [%s] \n", client_ptr->name, _client->name);
	} 
	else if ( pac->agree == false ) // 채팅 요청 거부
	  {
	    pro_notice_chat_connect pncc;
	    pncc.conn_state = broken;
	    
	    /* 
	       요청한   클라이언트에게는 거부했다는    메세지를 보내고
	       요청받은 클라이언트에게는 취소 되었다는 메세지를 보낸다
	       
	       PRO_NOTICE_CHAT_CONNECT 프로토콜 사용
	    */
	    
	    // 상태 변경
	    client_ptr -> p2p_conn_state = broken;
	    _client    -> p2p_conn_state = broken;
	    
	    /* [요청자] 에게 전송할 데이터 세팅 */
	    strcpy(pncc.who_name, _client -> name );
	    stream_sender(client_ptr->fd, NOTICE_CHAT_CONNECT, &pncc, sizeof( pro_notice_chat_connect));
	    
	    /* [요청받은자] 에게 전송할 데이터 세팅 */
	    strcpy(pncc.who_name, client_ptr -> name );
	    stream_sender(_client->fd, NOTICE_CHAT_CONNECT, &pncc, sizeof( pro_notice_chat_connect));
	    
	    printf("chat connected [%s] <-/-> [%s] \n", client_ptr->name, _client->name);	    
	  }
      }
      
    }
    break;

    /*
      상대에게 메세지 전송
      [클라1] -> [서버] -> [클라2]
    */
  case SEND_MESSAGE_TO_ONE :
    {
      client *c1 = _client;
      client *c2 = c_search_from_name(c1->p2p_name);
      pro_transfer_chat_message *ptcm = 
	(pro_transfer_chat_message*)c1->stream_body;
      
      
      // 연결상태가 정상이고 상대 클라이언트가 활성상태이면 
      // 메세지 전송
      if( c1->p2p_conn_state == connected  && c2->available == true ){
	// 트래픽 절약을 위한 사이즈 변경
	stream_sender(c2->fd, TRANSFER_CHAT_MESSAGE, &ptcm, 
		      sizeof(pro_transfer_chat_message) -
		      MESSAGE_MAX_SIZE + strlen(ptcm->message) );
      }
      
      
    }
    break;
  case REQUEST_LIST :
    {
      // 리스트 응답
      send_client_list(_client);
      
    }
    break;
  case REGISTER_NAME :
    {
      // 해당 프로토콜로 형변환
      pro_reg_name *prn = (pro_reg_name*)_client->stream_body;
      
      // 메모리 복사 널문자와 줄바꿈 문자 제외
      memcpy(_client->name, prn->name, NAME_SIZE );
      
      // 등록 알림
      printf("- notice \n\tregistered name: %s [%s]\n", _client->name, _client->IP);
    }
    break;
  DEFAULT :
    break;
  }
  
  _client->stream_state = EMPTY;
  
  return SUCCESS;
}


void send_client_list(client *_client){
  // 한번에 스트림바디에 담아 전송할 수 있는 최대 client_info 구조체 수
  int max_transfer_num = MAX_STREAM_BODY_SIZE/sizeof(client_info);
  
  // 활성화된 클라이언트 구조체
  int available_count = 0;
  
  // 쉬운 접근을 위한 구조체 포인터
  client_info *cinfos;

  // for
  int i;

  // 임시 공간 할당 ( 여기에 데이터를 담아 전송한다 )
  char body_tmp[MAX_STREAM_BODY_SIZE];

  // 클라이언트 배열 순회
  for ( i = 0 ; i < MAX_CON_USERS ; i++ ){
    client *client_ptr = &clients[i];
    
    // 활성 클라이언트 찾기
    if( client_ptr->available == true ){
      // 구조체 포인터 연산으로 메모리 공간을 형변환하여 접근
      client_info * c_info_ptr = ((client_info*)body_tmp + available_count);
      
      // 데이터 입력
      //memcpy(c_info_ptr->name, client_ptr->name, strlen(client_ptr->name));
      //memcpy(c_info_ptr->IP, client_ptr->IP, strlen(client_ptr->IP));
      
      
      strcpy(c_info_ptr->name, client_ptr->name);
      strcpy(c_info_ptr->IP, client_ptr->IP);
      
      //
      memcpy(& c_info_ptr->reg_timestamp, &client_ptr->reg_timestamp, sizeof(int));
      
      // 활성카운트 증가
      available_count++;
    }
  }
  //printf("available count : %d\n", available_count);
  // 스트림 전송
  stream_sender( _client->fd, 
		 RESPONSE_LIST, 
		 body_tmp, 
		 sizeof( client_info ) * available_count );

  //write( _client->fd, body_tmp, sizeof( client_info ) * available_count );
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
