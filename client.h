
// 클라이언트 상태
typedef enum{
  cstate_command_input,
  cstate_input_new_name,
  cstate_chat,
  cstate_wait_for_client_list,
  cstate_wait_for_response_conn,
  cstate_wait_for_server_response,
} client_state;


// 클라이언트 명령어
typedef enum {
  ccmd_register_name,
  ccmd_request_chat,
  ccmd_logout,
  ccmd_client_list,
  ccmd_file_transfer,
  ccmd_exit
} client_command;


void print_by_state(client_state state, client * _client);
void request_client_list(int fd);

bool request_chat_dialog(pro_req_matching_chat *prmc);
bool yes_and_no();
