/*

  Stream > Packet

 Stream Format 1 (Old)
 /---------------------/     /-----/
 | STS | TYPE | P_SIZE | ... | STE |
 /---------------------/     /-----/
 |------------- STREAM ------------|


 Stream Format 2 Current
 /---------------------/     
 | STS | TYPE | P_SIZE | ... ......
 /---------------------/     
 |------------- STREAM ------------|

*/



/**  Define  **/
#define BUF_SIZE 1024
#define NAME_SIZE 20

// 최대 동시 접속 (Concurrent users)
#define MAX_CON_USERS 64

// 최대 패킷 길이
#define MAX_PACKET 1024
#define MAX_STREAM_BODY_SIZE 1024


/** ENUM **/
// 어떤 함수의 처리 결과를 리턴할 때 사용
typedef enum{ FAIL, SUCCESS } result;

// 스트림의 상태를 나타내기위한 상수
// EMPTY : 비어있음 ( 패킷의 처리가 방금전 끝났을경우)
// STREAM_END : 스트림이 끝남
// STREAM_REMAIN : 아직 이어받아야 할 데이터가 있음
// STREAM_START  : 스트림이 시작됨
typedef enum{ EMPTY , STREAM_END, STREAM_REMAIN, STREAM_START , STREAM_ERROR} stream_state;

// 스트림의 목적
typedef enum {
  
  // Plain
  DEBUG_MESSAGE,

  // SEND TO
  SEND_MESSAGE_TO_ONE,
  SEND_MESSAGE_TO_BROAD,
  
  // Register
  REGISTER_NAME,
  
  // Request
  REQUEST_MATCHING_CHAT,
  REQUEST_LIST,

  // Response
  RESPONSE_ACCEPT_CHAT,
  RESPONSE_LIST,


} stream_purposes;



// bool type 선언
typedef enum {
  true = 1,
  false = 0,
} bool;


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
typedef struct PRO_REQ_MATCHING {
  char name [NAME_SIZE];
} pro_req_matching;




/** Include **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>


/** Variable **/

// 클라이언트를 관리하기 위한 공간100개 선언 할당
client clients[MAX_CON_USERS]; 

// 1바이트 데이터 
char tmpbyte = 'A';


/** Fcuntion Header **/

void error_handling(char *message );
char* getIP_from_sockaddr_in(struct sockaddr_in *addr_in);
result register_client(int client_fd, char *IP);
client* ci_search_from_fd(int fd);
stream_state stream_put_to_ci(client * _client , void *data, int size );
result stream_interpreter(client *_client );
void print_s_header(s_header *header);


/*** Send Functions ***/

void send_client_list(client *_client);
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


