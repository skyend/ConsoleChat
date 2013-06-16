typedef enum{
  cstate_command_input,
  cstate_input_new_name,
  cstate_chat,
} client_state;

typedef enum {
  ccmd_register_name,
  ccmd_request_chat,
  ccmd_logout,
  ccmd_why,
  ccmd_file_transfer,
  ccmd_exit
} client_command;


void print_by_state(client_state state);
