#define KEYWORD_CNT 15 // add header fseek -> rewind 실험 후 변경
#define UNDEFINED_CNT 2
#define DATATYPE_CNT 10
#define MINLEN 64
#define BUFFER_SIZE 1024
#define VARIABLE_SIZE 100
#define HEADER_CNT 8
#define OPERATOR_CNT 10

typedef struct KEYWORD {
    char *java_keyword; // java keyword
    char *c_keyword; // c keyword
    int is_func; // 함수 = 1, 이외 = 0
}keyword;

char data_return_type[DATATYPE_CNT][MINLEN] = { // datatype 선언
    {"int"}, {"char"}, {"double"}, {"float"}, {"long"}, {"short"}, {"byte"}
    , {"boolean"}, {"void"}, {"File"}
};

keyword KEYWORD[KEYWORD_CNT] = { // 키워드 구조체
    {"public", "", 0}, {"for", "for", 0}, {"if", "if", 0}, {"else", "else", 0}
    , {"class", "", 0}, {"static", "static", 0}, {"final", "const", 0}
    , {"new", "malloc()", 0}, {"return", "return", 0}
    , {"System.out.printf", "printf()", 1}, {"FileWriter", "fopen()", 1}
    , {"flush", "fflush()", 1}
    , {"nextInt", "scanf()", 1}, {"write", "fputs()", 1}, {"close", "fclose()", 1}
};

char undefined_key[UNDEFINED_CNT][MINLEN] = {{"Scanner"}, {"import"}};

char *java; // java파일명 저장
FILE *fp1; // java file pointer
char header[MINLEN][BUFFER_SIZE]; // header 저장
int c; // 옵션받는 변수
int brace = 0; // 중괄호 개수 카운트
int header_CNT = 0; // 헤더 배열에 담을 때 개수를 세는 변수
int var_CNT = 0; // 변수 배열에 담을 때 개수를 세는 변수
FILE *c_fp; // c file pointer
char *fname[100]; // c file name 저장, file 개수 100개 제한
int java_line_count = 0; // 옵션에서 라인카운트 붙일 때 처리 나중에 해줘야댐
int c_line_count = 0; // c_line을 카운트하는 변수
char *con_name[1000]; // 객체 생성 개수 1000으로 제한
int con_count = 0; // 객체 생성시 개수 카운트
bool no_brace = 0; // if or for시 중괄호가 없는 경우 체크
char *java_fp[1000]; // 오픈할 수 있는 파일을 1000개로 제한
char *java_fname[1000]; // 오픈한 파일명저장
char *wr_name[1000]; // FileWriter의 객체명 저장
char c_op_buf[100][BUFFER_SIZE]; // write한 c file의 버퍼를 저장
char j_op_buf[BUFFER_SIZE]; // read한 java file의 버퍼를 저장
int file_top = 0; // write file count
bool l_option = 0; // option flag
bool f_option = 0;
bool j_option = 0;
bool c_option = 0;
bool p_option = 0;
bool r_option = 0;
int file_count = 0; // c file count
int file_size[100]; // 생성한 c file의 사이즈 보관, 파일 개수 100개 제한
int file_line[100]; // 생성한 c file line수 저장
int p_check[100]; // p option시에 사용한 함수 저장
int p_count = 0; // p option시에 사용한 함수 카운트
int pid = 0; // r-option, fork - pid
char *extern_buf[100]; // 메소드 extern을 위한 변수
int extern_count = 0; // extern 할 메소드 카운트 변수
bool prev_main = 0; // extern시에 이전 class에 메인이 있는지 확인 체크를 위한 변수
bool IOException = 0; // q3와 같은 예외처리가 필요한 경우 체크

void makefile(); // makefile을 만들어주는 함수
void cfile_write(char *c_buf); // c file에 파일출력을 해주는 함수
void add_header(); // 프로그램 초반 헤더를 추가해주는 함수
char *ltrim(char *_str); // 버퍼 왼쪽공백 제거함수
int count_brace(char *buf); // 버퍼에서 중괄호 개수를 셈, { == +1, } == -1
int func_call(char *buf); // 각 버퍼별로 필요한 조치(함수호출)을 해주는 함수
void convert(); // 메인함수에서 번역을 시작하기 위해 호출하는 함수
void remove_con(char *buf); // convert함수에서 java파일을 읽을때마다 버퍼에 있는 객체명을 제거해주는 함수
int seperation_keyword(char *buf); // 버퍼에 자바 키워드가 있는지 확인 -> 헤더에 선언된 구조체 배열 인덱스 리턴, 없을 시 -1리
int seperation_datatype(char *buf); // 버퍼에 datatype이 있는지 확인, 헤더에 선언된 배열에 해당되는 인덱스 return, 없을 때 - -1리턴
void public_handling(char *buf); // public이 붙어있는 메소드, 함수 ... 처리
void is_new(char *buf); // new 키워드 처리 (객체 생성 or 메모리할당)
void is_scanf(char *buf); // scanf 번역
void is_printf(char *buf); // printf 번역
void check_var(int class_brace); // is_class에서 호출, 메인함수처리, 변수선언을 최상단으로 올리기 위한 함수
void is_main(char *buf);
void is_method(char *buf);
void is_variable(char *buf); // check_var에서 변수일 경우 호출 -> public 제거, final 등 기타 변수를 처리해서 c file에 write
void is_class(char *buf); // class인 경우 처리 (c file open)
void is_return(char *buf); // 특정 리턴값이 없는 경우 exit(0)로 치환
void is_if(char *buf); // if문인 경우 처리
void is_write(char *buf); // write문 처리
void is_flush_close(char *buf); // fflush와 fclose는 처리 프로세스가 같으므로 같이 처리
void p_overlap(char *buf); // p option시 버퍼에 있는 함수가 중복되지 않으면 배열에 추가
void ssu_runtime(struct timeval* begin_t, struct timeval *end_t); // 시간을 재는 함수

