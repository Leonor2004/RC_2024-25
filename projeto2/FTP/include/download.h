#define USER_DEAFAULT "anonymous"
#define PASS_DEAFAULT "anonymous"

#define BUFFER_SIZE 1024
#define DOWNLOAD_DIR "downloads/"

#define LOGED_IN_CODE 220
#define PASSWORD_CODE 331
#define LOGIN_SUCCESSFULL_CODE 230
#define PASSIVE_MODE_CODE 227
#define START_TRANSFER_CODE 150
#define TRANSFER_COMPLETED_CODE 226
#define N_TRIES 3

#define AT              "%*[^/]//%s@"
#define HOST_REGEX      "%*[^/]//%[^/]"
#define HOST_AT_REGEX   "%*[^/]//%*[^@]@%[^/]"
#define PATH_REGEX  "%*[^/]//%*[^/]/%s"
#define USER_REGEX      "%*[^/]//%[^:/]"
#define PASS_REGEX      "%*[^/]//%*[^:]:%[^@\n$]"
#define PASSIVE_REGEX   "%*[^(](%d,%d,%d,%d,%d,%d)%*[^\n$)]"
