#ifndef COMMANDS_H
#define COMMANDS_H

// socket password
#define PASSWORD "123456789"

// system commands
#define GOOD "true"
#define BAD "false"
#define SYSTEM_MESSAGE_START '$'

// must begin with SYSTEM_MESSAGE_START
#define VALIDATION "$connect"
#define SIGNUP "$signup"
#define DUPLICATE "$duplicate"
#define LOG "$log_request"
#define LOG_CONTACTS "$log_contacts"
#define LOGPHOTO "$logphoto_request"
#define UPDATE_LOG "$update_log"
#define UPDATE_LOG_TS "$update_log_ts"
#define LOG_FINISH "$log_finished"
#define CONNECTED "$connected"
#define DISCONNECTED "$disconnected"
#define IS_ONLINE "$is_online"
#define ADD_CONTACT "$add_contact"
#define ERROR "$error"
#define ALL "$all"
#define ID "$id"
#define USER "$user"
#define EXIT "$exit"
#define ADD_GROUP "$add_group"
#define CREATE_GROUP "$create_group"
#define DELETE_GROUP "$delete_group"
#define JOIN_GROUP "$join_group"

//Количество зарегистрированных пользователей
//to-server: $user_count
//from-server: $user_count 20
#define USER_COUNT "$user_count"

//Информация о пользователе
//to-server: $user_info 10
//from-server: $user_info 10 denis isOnline[true/false]
#define USER_INFO "$user_info"



// send message or photo
#define SEND "send"
#define SENDPHOTO "sendphoto"
#define SENDGROUP "sendgroup"
#define PHOTO "photo"
#define FILE_C "file"
// end

#endif
