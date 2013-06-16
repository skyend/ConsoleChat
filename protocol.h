/*

  Stream > Packet

 Stream Format 1 (Old)
 /---------------------/     /-----/
 | STS | TYPE | P_SIZE | ... | STE |
 /---------------------/     /-----/
 |------------- STREAM ------------|


 Stream Format 2 Current
 /---------------------/---/    
 | STS | TYPE | P_SIZE | 1 | .. BODY ..
 /---------------------/---/     
 |------------- STREAM ----------------|

*/



/**  Define  **/
#define BUF_SIZE 1024
#define NAME_SIZE 20

// 최대 동시 접속 (Concurrent users)
#define MAX_CON_USERS 64

// 최대 패킷 길이
#define MAX_PACKET 1024
#define MAX_STREAM_BODY_SIZE 1024

#define MESSAGE_MAX_SIZE 512


/** ENUM **/
// 어떤 함수의 처리 결과를 리턴할 때 사용
typedef enum{ 
  FAIL, 
  SUCCESS 
} result;

// 스트림의 상태를 나타내기위한 상수
// EMPTY : 비어있음 ( 패킷의 처리가 방금전 끝났을경우)
// STREAM_END : 스트림이 끝남
// STREAM_REMAIN : 아직 이어받아야 할 데이터가 있음
// STREAM_START  : 스트림이 시작됨
typedef enum{ 
  EMPTY , 
  STREAM_END, 
  STREAM_REMAIN, 
  STREAM_START , 
  STREAM_ERROR
} stream_state;

// 스트림의 목적
typedef enum {
  
  // Plain
  DEBUG_MESSAGE,

  // SEND TO
  SEND_MESSAGE_TO_ONE, // 한명을 대상으로한 메세지 보내기
  SEND_MESSAGE_TO_BROAD, // 접속한 모두에게 보내는 메세지
  
  // Transfer  
  TRANSFER_CHAT_MESSAGE, // 채팅 메세지 전송
  
  // Register
  REGISTER_NAME, // 이름 등록
  
  // Request
  REQUEST_MATCHING_CHAT, // 상대에게 채팅 요청
  REQUEST_LIST, // 접속자 리스트 요청

  // Response
  RESPONSE_ACCEPT_CHAT, // 채팅 요청 응답
  RESPONSE_LIST, // 접속자 리스트 제공

  // Notice
  NOTICE_CHAT_CONNECT,

  // Suggest
  SUGGEST_CHAT,

  // Server error
  ERROR_RETURN,

} stream_purposes;



// bool type 선언
typedef enum {
  true = 1,
  false = 0,
} bool;


typedef enum {
  wait, // 연결 기다리는중 ( 누군가에게 요청을 한 상태 )
  connected, // 연결됨 ( 두 클라이언트의 연결이 성사 되었을 때 )
  broken, // 연결깨짐 ( 요청을 허가 하지 않았을 때나 끝났을 때 )
  recv_reguest, // 요청중 (누군가 요청했을때의 상태 )
} connection;

typedef enum{
  sent_chat_accept_to_invalid_client, //잘못된 대상에게 채팅허가 메세지를 보냄
  
}error_code;

/** Structure **/

typedef struct stream_header {
  unsigned char    STS ; // 0xFF 고정
  stream_purposes  type; // 0~255 
  unsigned int     stream_body_size; // 0~4294967295
  bool             complete; // true 고정 헤더의 끝
} s_header;

// 사용안함
typedef struct stream_ender {
  unsigned char STE ; // 0xEE 고정
} s_ender;

typedef struct client {
  bool            available; // 유효한 클라이언트인가
  int             fd; // 클라이언트에 해당하는 소켓 fd
  char            IP[20]; // 클라이언트 ip
  char            name[NAME_SIZE]; // 클라이언트 이름
  int             reg_timestamp; // 이름 등록 시간

  // 1:1 채팅 연결 정보
  char            p2p_name[NAME_SIZE]; // 채팅상대
  connection      p2p_conn_state; // 채팅이 연결 되었는지에 대한 플래그
  
  // 스트림 관리
  s_header        stream_header;
  int             stream_body_length; // 현재까지 유효한(받은) 패킷의 길이
  char            stream_body[MAX_STREAM_BODY_SIZE]; // 패킷데이터를 쌓아둘 공간
  stream_state    stream_state; // 스트림의 상태 
  //stream_purposes stream_purpose;
} client;

// 클라이언트 접속 정보로 제공할 구조체
typedef struct client_info{
  char   name[NAME_SIZE];
  char   IP[20];
  int    reg_timestamp;
} client_info;

/*** Protocol Structures ***/

// 이름 입력 프로토콜 구조체
typedef struct PRO_REG_NAME {
  char name[NAME_SIZE];
} pro_reg_name;

// 채팅 요청 프로토콜 구조체
typedef struct PRO_REQ_MATCHING_CHAT {
  char name [NAME_SIZE];
} pro_req_matching_chat;

// 대상 클라이언트에게 채팅을 요청하는 프로토콜 구조체
typedef struct PRO_SUG_CHAT {
  char who_name[NAME_SIZE];
} pro_sug_chat;

// 채팅허가 프로토콜
typedef struct PRO_ACCEPT_CHAT {
  char who_name[NAME_SIZE];
  bool agree;
} pro_accept_chat;


// 채팅 연결 결과 공지 프로토콜
typedef struct PRO_NOTICE_CHAT_CONNECT {
  char       who_name[NAME_SIZE];
  connection conn_state;
} pro_notice_chat_connect;

// 클라이언트에게 에러를 반환하는 프로토콜
typedef struct PRO_ERROR_RETURN {
  error_code error;
} pro_error_return;

// 메세지 전송 프로토콜
typedef struct PRO_TRANSFER_CHAT_MESSAGE {
  char who_name[NAME_SIZE];
  char message[MESSAGE_MAX_SIZE];
} pro_transfer_chat_message;

/** Include **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>


/** Variable **/

// 클라이언트를 관리하기 위한 공간 최대 동시접속 수 만큼 선언 할당
client clients[MAX_CON_USERS]; 

// 1바이트 데이터 
char tmpbyte = 'A';


/** Fcuntion Header **/

void          error_handling(char *message );
char*         getIP_from_sockaddr_in(struct sockaddr_in *addr_in);
result        register_client(int client_fd, char *IP);
client*       c_search_from_fd(int fd);
stream_state  stream_put_to_ci(client * _client , void *data, int size );
result        stream_interpreter(client *_client );
void          print_s_header(s_header *header);


/*** Send Functions ***/

void          send_client_list(client *_client);
client *      c_search_from_name(char *name);
void          error_sender(int fd, error_code error);

//void stream_sender(client * _client, stream_purposes type, void *data_ptr, int size);




/** Function implements **/

void stream_sender(int fd, stream_purposes type, void *data_ptr, int size){
  s_header header;
  int i = 0;

  // 헤더 정보 세팅
  header.STS = 0xFF;
  header.type = type;
  header.stream_body_size = size;
  header.complete = true;
  
  write( fd , &header, sizeof(s_header) );
  //for( i = 0 ; i < 1000000 ; i++ );
  write( fd , (char*)data_ptr, size );
  
}

