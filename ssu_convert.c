#include <stdio.h> // ftell -> ftello, fseek -> fseeko 바꿔야 됌
#include <stdlib.h> // add_header에서 main있는지 체크
#include <string.h> // default -> exist_main = -1
#include <fcntl.h>
#include <stdbool.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "convert.h"

int main(int argc, char *argv[]) {
    java = (char *)malloc(sizeof(char) * strlen(argv[1]));
    strcpy(java, argv[1]); // java file name 저장

    if ((fp1 = fopen(java, "r")) == NULL) { // java file open error 처리
        fprintf(stderr, "fopen erorr %s\n", java);
        exit(1);
    }

    while ((c = getopt(argc, argv, "jcpflr")) != -1) { // option별 flag에 체크해서 추후 처리
        switch (c) {
            case 'j' :
                j_option = true;
                break;
            case 'c' :
                c_option = true;
                break;
            case 'p' :
                p_option = true;
                break;
            case 'f' :
                f_option = true;
                break;
            case 'l' :
                l_option = true;
                break;
            case 'r' :
                r_option = true;
                break;
            case '?' :
                fprintf(stderr, "존재하지 않는 옵션입니다.\n"); // 존재하지 않는 옵션 입력시 에러메세지 출력 후 종료
                exit(1);
                break;
        }
    }

    convert(); // convert 시작
    makefile(); // 만들어진 c 파일을 컴파일 할 makefile 생성

    return 0;
}

void makefile() {
    FILE *fp;
    char *make_fname = (char *)malloc(sizeof(char) * BUFFER_SIZE);
    char *copy = (char *)malloc(sizeof(char) * strlen(java));
    char *buf = (char *)malloc(sizeof(char) * BUFFER_SIZE);
    char *gcc_buf = (char *)malloc(sizeof(char) * BUFFER_SIZE);
    char *c_fname[100];

    strcpy(copy, java); // java 파일명 복사

    for (int i = strlen(copy); i > 0; i--) { // 자바 파일명에서 확장자를 제거
        if (copy[i] != '.') 
            copy[i] = '\0';
        else {
            copy[i] = '\0';
            break;
        }
    }
Row 1
    sprintf(make_fname, "%s_Makefile", copy); // Makefile 이름 생성

    if ((fp = fopen(make_fname, "w+")) == NULL) { // Makefile 작성
        fprintf(stderr, "Make %s error\n", make_fname);
        exit(1);
    }

    sprintf(buf, "%s :", copy); // 최종 target code 생성
    sprintf(gcc_buf, "\tgcc -o %s", copy); // 최종 compile code 생성

    for (int i = 0; i < file_count; i++) {
        c_fname[i] = malloc(sizeof(char) * strlen(fname[i])); // cfile명에서 확장자를 .o로 바꾸기위한 작업
        strcpy(c_fname[i], fname[i]);
        for (int j = strlen(c_fname[i]); j > 0; j--) {
            if (c_fname[i][j] != '.')
                c_fname[i][j] = '\0';
            else {
                c_fname[i][j] = '\0';
                break;
            }
        }
        strcat(c_fname[i], ".o"); // .o 확장자 추가
        sprintf(buf, "%s %s", buf, c_fname[i]); // 최종 target code 생성
        sprintf(gcc_buf, "%s %s", gcc_buf, c_fname[i]); // 최종 compile code 생성 
    }
    strcat(buf, "\n");
    strcat(gcc_buf, "\n");
    fputs(buf, fp); // 최종 target code 메이크 파일에 출력
    fputs(gcc_buf, fp); // 최종 compile code 메이크 파일에 출력

    for (int i = 0; i < file_count; i++) { 
        sprintf(buf, "%s : %s\n", c_fname[i], fname[i]); // 중간 target code 생성
        sprintf(gcc_buf, "\tgcc -c %s\n", fname[i]); // 중간 compile code 생성
        fputs(buf, fp); // 중간 target code 파일 출력
        fputs(gcc_buf, fp); // 중간 compile code 파일 출력
    }

    free(make_fname); // 메모리 해제
    free(copy);
    free(buf);
    free(gcc_buf);
}

void cfile_write(char *c_buf) { // 나중에 옵션처리시 idx로 자바,c 키워드 받아옴
    int write_brace = count_brace(c_buf);
    char *temp = (char *)malloc(sizeof(char) * BUFFER_SIZE);

    ++c_line_count; // 

    if (write_brace == -1) { // 닫는 괄호는 즉시 중괄호 갯수를 빼줌
        if (!brace) { // 중괄호 카운트가 0일때 중괄호 카운트를 빼면 리턴 -> class 하나가 끝남을 의미
            fclose(c_fp); // class가 끝났으므로 c file을 close해줌
            printf("%s convert Success!\n", fname[file_count]); // 변환 완료 메세지 출력

            if (c_option) // c_option 시에 완료된 c file buffer를 출력
                printf("%s\n", c_op_buf);

            memset(c_op_buf, 0, sizeof(char) * BUFFER_SIZE); // c file buffer 초기화

            if (l_option)
                file_line[file_count] = --c_line_count; // c file line을 저장하는 배열에 라인 수 저장
            c_line_count = 0;

            if (r_option) // r option시에 class하나 끝날 때마다 convert finish 메세지를 출력
                printf("%s converting is finished !\n", fname[file_count]);
            file_count++; // file 개수를 하나 더해준다
            return;
        }
        brace -= 1 ; // 중괄호 개수 하나 빼줌
    }

    for (int i = 0; i < brace; i++) {
        fwrite("\t", 1, 1, c_fp); // indent를 count한 중괄호 개수만큰 출력
    }

    if (no_brace) {
        fwrite("\t", 1, 1, c_fp); // 만약 이전 문장이 for문이나 if문이었는데 중괄호가 없었다면 한 칸 tab
        no_brace = false;
    }

    if (((strstr(c_buf, "if") != NULL) || (strstr(c_buf, "for")!= NULL)) && (strstr(c_buf, "{") == NULL)) // if or for문에 여는 중괄호가 없으면 다음 문장에 tab추가
        no_brace = true;

    fwrite(c_buf, sizeof(char) * strlen(c_buf), 1, c_fp); // buffer를 받아서 write 
    fwrite("\n", 1, 1, c_fp); // buffer를 받아서 write 


    if (c_option || r_option) { // c_option or r_option시에 c buffer 저장
        sprintf(temp, "%d ", c_line_count);

        for (int i = 0; i < brace; i++)
            strcat(temp, "\t");
        strcat(temp, c_buf);
        strcat(temp, "\n");
        strcat(c_op_buf, temp);
    }

    if (r_option) { // r option시에 c file과 java 파일의 입출력 stream을 초기화 (입출력버퍼가 남아있는 경우의 처리)
        fflush(c_fp);
        fflush(fp1);
        if ((pid = fork()) < 0) { // 자식 프로세스 생성 에러시 에러 메세지 출력 후 종료
            fprintf(stderr, "fork error\n");
            exit(1);
        }
        else if (pid == 0) { // 자식 프로세스일 때
            system("clear"); // terminal clear
            printf("%s Converting...\n", fname[file_count]);
            printf("------\n");
            printf("%s\n", java); // java file name print
            printf("------\n");
            printf("%s\n", j_op_buf); // java file buffer print
            printf("------\n");
            printf("%s\n", fname[file_count]); // c file name print
            printf("%s\n", c_op_buf); // c file buffer printf
            printf("------\n");
            sleep(1); // 변환되는 과정을 보이기 위해 1초 delay
            exit(0); // 자식 프로세스 종료
        }
        else
            sleep(1); // 부모 프로세스일 때 1초 delay
    }

    if (write_brace == 1) { // 여는 괄호는 다음 줄부터 indent 계산
        brace += 1;
    }

    free(temp); // 메모리 해제
}

void add_header() { // 각 함수에 맞는 헤더들을 미리 생성해둔 헤더테이블에서 찾아서 c file에 추가
    char *fname = "header"; // 각 함수들의 헤더들을 갖고 있는 파일
    int cnt = 0;
    char buf[MINLEN];
    char func[MINLEN];
    char temp[BUFFER_SIZE];
    char tmp_header[BUFFER_SIZE];
    char *save_ptr = malloc(sizeof(char) * BUFFER_SIZE);
    char *define_buf = malloc(sizeof(char) * BUFFER_SIZE);
    char *C_KEY = malloc(sizeof(char) * BUFFER_SIZE);
    char *ptr;
    FILE *fp2;
    off_t cur_pos;
    char *tok;
    int header_brace = 1;

    bool overlap = 0; // 검사한 함수의 헤더가 이미 추가 할 헤더 목록에 있는지 중복여부 체크하는 bool 변수
    bool exit_loop = 0; // 읽은 buffer에 c에는 정의 되어 있지 않은 함수 일 때 체크해서 반복문 한번 continue를 해주기 위한 bool변수
    bool new_check = 1; // new keyword가 나왔을 때 클래스 객체 생성 or 동적 할당 여부를 체크하기 위한 bool 변수

    memset(buf, 0, sizeof(char) * MINLEN); 
    memset(func, 0, sizeof(char) * MINLEN);
    memset(temp, 0, sizeof(char) * BUFFER_SIZE);
    memset(tmp_header, 0, sizeof(char) * BUFFER_SIZE);
    memset(define_buf, 0, sizeof(char) * BUFFER_SIZE);
    fp2 = fopen(fname, "r"); // 헤더 파일 open

    cur_pos = ftell(fp1); // 파일을 검색하기전 오프셋 저장, 헤더추가가 끝나면 다시 현재 파일 위치로 이동하기 위함

    while (fgets(buf, BUFFER_SIZE, fp1) != NULL) {
        strcpy(buf, ltrim(buf));

        if (!strlen(buf)) { // 빈 문자열, continue
            continue;
        }

        buf[strlen(buf) - 1] = '\0'; // 맨 끝 개행문자 제거

        header_brace += count_brace(buf);

        if (header_brace == 0) { // 중괄호 개수를 카운트해서 닫는괄호와 여는괄호의 개수가 같아지면 종료
            break;
        }

        exit_loop = false;

        for (int i = 0; i < UNDEFINED_CNT; i++) {
            if (!strncmp(buf, undefined_key[i], strlen(undefined_key[i]))) { // 이번 버퍼에 undefine된 키가 있다면 반복문 continue
                exit_loop = true;
            }
        }

        if (exit_loop)
            continue;   

        for (int i = 0; i < KEYWORD_CNT; i++) { // new가 메모리 할당인지 객체 생성인지 판단해서 메모리 할당이라면 malloc에 맞는 헤더추가
            if ((ptr = strstr(buf, KEYWORD[i].java_keyword)) != NULL) {
                strcpy(C_KEY, KEYWORD[i].c_keyword); // 찾은 java_keyword에 매칭되는 c_keyword 저장
                if (!strncmp(ptr, "new", 3)) {
                    for (int p = 0; p < DATATYPE_CNT; p++) {
                        if (strstr(ptr, data_return_type[p]) != NULL) { // new가 있는 buffer에 자료형이 있다면 malloc으로 판단해서 C_KEY변경
                            strcpy(C_KEY, "malloc");
                            break;
                        }
                    }
                }

                if (!strncmp(ptr, "return", 6)) { // 어떤 값을 반환하지 않는 경우엔 exit으로 변환해서 헤더추가
                    ptr = strstr(ptr, " ");
                    if (*ptr == ' ')
                        ptr++;
                    if ((*ptr == ';' || *ptr == '0')) {
                        strcpy(C_KEY, "exit"); // C_KEY 변경
                    }
                }

                save_ptr = strstr(buf, KEYWORD[i].java_keyword); // buffer에서 자바 키워드를 찾음

                if ((save_ptr = strstr(buf, "final")) != NULL) { // final이 나오면 #define문으로 치환
                    tok = strtok(save_ptr, " =;");
                    tok = strtok(NULL, " =;");
                    tok = strtok(NULL, " =;");
                    sprintf(save_ptr, "#define %s ", tok);
                    tok = strtok(NULL, " =;");
                    strcat(save_ptr, tok);
                    strcpy(define_buf, save_ptr);

                    break;
                }

                for (int j = 0; j < HEADER_CNT; j++) {
                    fscanf(fp2, "%s ", func); // 헤더테이블에서 함수명을 뽑아낸다
                    fgets(tmp_header, BUFFER_SIZE, fp2); // 헤더테이블에서 함수명의 헤더를 뽑아냄

                    if (strncmp(func, C_KEY, strlen(func))) { // 위에서 구분한 keyword와 헤더테이블의 함수명이 일치하지 않으면 continue
                        continue;
                    }

                    tmp_header[strlen(tmp_header) - 1] = '\0'; // 맨 뒤의 개행문자 제거
                    tok = strtok(tmp_header, " ");
                    cnt++;

                    while (tok != NULL) {
                        if (cnt) { // #include를 이미 받았을 때
                            sprintf(temp, "%s %s", tok, strtok(NULL, " ")); // token과 #include 결합
                            cnt = 0;
                        }
                        else { // #include 일경우 strtok만 해준다
                            tok = strtok(NULL, " ");
                            cnt++;
                        }

                        for (int i = 0; i < header_CNT; i++) {
                            if(!strncmp(header[i], temp, strlen(header[i])))
                                overlap = true;
                        }

                        if(!overlap) { // 중복 아닐 때
                            strcpy(header[header_CNT++], temp);
                        }

                        overlap = false;
                    }
                }
                fseek(fp2, 0, SEEK_SET); // 맨 처음 위치로 이동해서 다시 함수를 찾음
                break;
            }
        }
        new_check = true;
    }
    fseek(fp1, cur_pos, SEEK_SET); // add_header 함수 호출 전 파일 오프셋으로 재이동

    for (int i = 0; i < header_CNT; i++) {
        cfile_write(header[i]); // 중복없이 추가된 헤더 파일들을 파일에 출력해준다
    }

    cfile_write(""); // 가독성을 위한 한 칸 개행

    if (strlen(define_buf)) { // define문이 있는 경우 write를 해준다
        cfile_write(define_buf);
        cfile_write("");
    }


    free(save_ptr);
    free(define_buf);
    free(C_KEY);
}

char *ltrim(char *_str) { // 왼쪽 공백 제거
    char *start = _str;

    while (*start != '\0' && isspace(*start))
        ++start;
    _str = start;

    return _str;
}

int seperation_keyword(char *buf) { // 키워드 구분해서 리턴
    char *copy = (char *)malloc(sizeof(char) * strlen(buf));

    strcpy(copy, buf);
    strcpy(copy, ltrim(copy));

    if (buf[0] == '/')
        return -1; // 주석이면 끝냄

    for (int i = 0; i < file_top; i++) { // buffer에서 FileWriter 객체이름 제거
        if (wr_name[i] == NULL)
            continue;
        if (!strncmp(copy, wr_name[i], strlen(wr_name[i]))) {
            copy += strlen(wr_name[i]) + 1;
        }
    }

    for (int i = 0; i < KEYWORD_CNT; i++) { // 버퍼에서 java_keyword를 찾으면 구조체 인덱스를 리턴하고 함수종료
        if (strstr(copy, KEYWORD[i].java_keyword) != NULL) {
            return i;
        }
        else
            continue;
    }

    free(copy);

    return -1;
}

int seperation_datatype(char *buf) { // buffer에 데이터 타입이 있는지 찾아주는 함수
    char *tok;
    char tmp[BUFFER_SIZE];
    bool datatype_check = false;

    memset(tmp, 0, sizeof(char) * BUFFER_SIZE);

    strcpy(tmp, buf);
    strcpy(tmp, ltrim(tmp));

    for (int i = 0; i < DATATYPE_CNT; i++) {
        if ((strstr(tmp, data_return_type[i]) != NULL) &&
                (strstr(tmp, "for") == NULL) && // for문은 따로 함수호출을 하기때문에 예외처리
                (strstr(tmp, "printf") == NULL) && // printf에 "int"가 들어가므로 예외처리
                (strstr(tmp, "(") == NULL)) { // 괄호가 들어가면 메소드이거나 변수선언은 아니므로 예외처리
            tok = strtok(tmp, " "); // 자료형만 떼어냄

            if (!strncmp(tok, data_return_type[i], strlen(data_return_type[i]))) { // 자료형이라면 자료형 배열의 인덱스 리턴
                datatype_check = true;
                return i;
            }

            for (int j = 0; j < KEYWORD_CNT; j++) { // 만약 자료형이아니고 java keyword인 경우를 걸러냄
                if (!strncmp(KEYWORD[j].java_keyword, tok, strlen(KEYWORD[j].java_keyword))) {
                    datatype_check = true;
                    break;
                }
            }
        }

        if ((strncmp(tmp, "FileWriter", 10) != 0) && (!strncmp(tmp, "File", 4))) // File 이 괄호때문에 위에서 걸러지기 때문에 따로처리
            return 9;
        if (datatype_check)
            return i;
    }

    return -1;
}

int count_brace(char *buf) { // 입력받은 버퍼의 중괄호 개수를 세어줌 ( '{' -> count++, '}' -> count--)
    int _brace = 0;
    char *temp = malloc(sizeof(char) * BUFFER_SIZE);

    strcpy(temp, buf);

    if (strstr(temp, "{") != NULL) {
        _brace++;
    }
    if (strstr(temp, "}") != NULL)
        _brace--;

    return _brace;
}

int func_call(char *buf) { // 키워드를 찾아서 각 키워드에 맞는 함수를 호출
    int re = 0;
    bool overlap = 0;

    if ((strlen(buf) == 1) && ((!strncmp(buf, "{", 1)) || (!strncmp(buf, "}", 1)))) { // 버퍼에 중괄호만 있을 때 바로 write

        cfile_write(buf);
        return -1;
    }

    if ((re = seperation_datatype(buf)) != -1) { // 변수 선언은 초기에 처리 해줬기 때문에 다른 처리가 필요없음
        return -1;
    }
    else if((re = seperation_keyword(buf)) != -1) { 
        if (KEYWORD[re].is_func && p_option) { // p option시에 변환되는 함수목록을 저장하기 위한 처리
            for (int i = 0; i < p_count; i++) {
                if (p_check[i] == re) { // 만약 저장한 배열에 이미 현재 버퍼의 함수가 포함되어있다면 중복처리
                    overlap = true;
                    break;
                }
            }
            if (!overlap) { // 중복되지 않았다면 배열에 추가
                p_check[p_count++] = re;
            }
        }
        switch(re) { // 각 구조체 인덱스를 사용하여 키워드에 맞는 함수 호출
            case 0 :
                public_handling(buf);
                break;
            case 1 :
                cfile_write(buf);
                break;
            case 2 :
                is_if(buf);
                break;
            case 3 :
                cfile_write(buf);
                break;
            case 4 :
                is_class(buf);
                break;
            case 7 :
                is_new(buf);
                break;
            case 8 :
                is_return(buf);
                break;
            case 9 :
                is_printf(buf);
                break;
            case 11 :
                is_flush_close(buf);
                break;
            case 12 :
                is_scanf(buf);
                break;
            case 13 :
                is_write(buf);
                break;
            case 14 :
                is_flush_close(buf);
                break;
            default :
                break;
        }
    }
    else { // 모든 사항에 해당되지 않으면 연산식 or 주석으로 간주
        cfile_write(buf);
    }
}

void convert() { // convert 시작
    int re = -1;
    char buf[BUFFER_SIZE];
    char temp[BUFFER_SIZE];
    bool undefined = false;
    struct stat statbuf;

    memset(buf, 0, sizeof(char) * BUFFER_SIZE);

    while (fgets(buf, BUFFER_SIZE, fp1) != NULL) {
        fflush(fp1); // 버퍼를 받을 때마다 입출력버퍼 초기화
        fflush(c_fp);
        ++java_line_count; // 자바 라인수 count
        if (j_option || r_option) { // j option or r option시 java file을 출력하므로 line_count를 붙여서 저장
            memset(temp, 0, sizeof(char) * BUFFER_SIZE);
            sprintf(temp, "%d %s", java_line_count, buf);
            strcat(j_op_buf, temp);
        }

        strcpy(buf, ltrim(buf)); // 왼쪽 공백 제거
        if (!strlen(buf)) { // 빈 문자열, continue
            continue;
        }

        buf[strlen(buf) - 1] = '\0'; // 맨 끝 개행문자 제거

        remove_con(buf); // 객체이름 제거 (함수 내부에서 재귀적으로 호출해서 한 line에 여러개의 객체 이름이 있어도 모두 제거가능)

        for (int i = 0; i < UNDEFINED_CNT; i++) { // java에만 있는 keyword일 때, continue
            if (!strncmp(buf, undefined_key[i], strlen(undefined_key[i])))
                undefined = true;
        }

        if (undefined) {
            undefined = false;
            continue;
        }

        if (re = func_call(buf)) // 버퍼에 있는 keyword, 변수 등을 찾아서 맞는 함수호출을 하기위한 처리
            continue;
    }

    fclose(fp1); // 버퍼를 모두 읽으면 java file close
    if (l_option) { // l option 시에 count 했던 java와 c파일의 line count 출력
        printf("%s line number is %d lines\n", java, java_line_count);
        for (int i = 0; i < file_count; i++) {
            printf("%s line number is %d lines\n", fname[i], file_line[i]);
        }
    }
    if (f_option) { // f option시 stat함수를 사용해서 파일 크기 출력
        stat(java, &statbuf);
        printf("%s file size is %ld bytes\n", java, statbuf.st_size);
        for (int i = 0; i < file_count; i++) {
            stat(fname[i], &statbuf);
            printf("%s file size is %ld bytes\n", fname[i], statbuf.st_size);
        }
    }
    if (j_option) { // j option시 위에서 저장한 java buffer 출력
        printf("%s\n", j_op_buf);
    }
    if (p_option) { // p option시 배열에 저장된 숫자들로 키워드 구조체에 인덱스로 접근해서 출력
        for (int i = 0; i < p_count; i++) {
            printf("%d %s() -> %s\n", i+1, KEYWORD[p_check[i]].java_keyword, KEYWORD[p_check[i]].c_keyword);
        }
    }
}

void remove_con(char *buf) { // 객체 이름 제거
    char str[BUFFER_SIZE];
    char result[BUFFER_SIZE];
    char *ptr;
    bool find = 0;
    int con_len;
    int len;
    int cnt = 0;

    strcpy(str, buf);

    for (int i = 0; i < con_count; i++) { // 객체 이름을 찾고 객체 카운트 추가
        if ((ptr = strstr(str, con_name[i])) != NULL) {
            con_len = strlen(con_name[i]);
            find = 1;
            cnt++;
        }
    }

    if (find) { // 찾았다면
        len = strlen(str) - strlen(ptr); // 객체이름이 나오기 이전까지의 길이
        str[len] = '\0'; // 객체 이름이 나오기 전까지로 자름
        sprintf(result, "%s%s", str, ptr + con_len); // 객체이름을 건너뛰고 붙여서 제거
        strcpy(buf, result); // 버퍼에 다시 넣어준다
        find = 0;
        remove_con(buf); // 여러개의 메소드 호출에 대비한 재귀 구조
    }
    else // 없다면 함수 종료
        return;
}

void public_handling(char *buf) { // main, 메소드, 변수 등등 c에는 없는 public이 달려있을때 각 종류별로 처리해주는 함수
    char *tok;
    char tmp[BUFFER_SIZE];
    int main = 0;
    int method = 0;
    int class = 0;

    memset(tmp, 0, sizeof(char) * BUFFER_SIZE);
    strcpy(tmp, buf);
    strcpy(tmp, ltrim(tmp));

    tok = strtok(tmp, " ");

    while (tok != NULL) {
        if (!strncmp("main", tok, 4)) { // class -> check_var에서 처리
            break;
        }
        else if (!strncmp("class", tok, 5)) { // is_class를 호출하기위한 체크
            class = 1;
            break;
        }
        else if (main != 1 && class != 1){ // main도 아니고 class도 아닐때 괄호가 붙어있다면 메소드
            for (int i = 0; i < strlen(tok); i++) {
                if (tok[i] == '(') {
                    method = 1;
                    break;
                }
            }
        }
        tok = strtok(NULL, " ");
    }

    if (method) // 각 경우에 맞는 함수 호출
        is_method(buf);
    else if (class) {
        is_class(buf);
    }
}

void is_new(char *buf) { // new일때 객체 생성인지 메모리 할당인지 구분해서 맞는 처리를 해주기 위한 함수
    char *copy = malloc(sizeof(char) * BUFFER_SIZE);
    char *result = malloc(sizeof(char) * BUFFER_SIZE);
    char tmp[100][BUFFER_SIZE];
    char *tok;

    bool mem_alloc = false;
    char *constructor = malloc(sizeof(char) * BUFFER_SIZE);

    int cnt = 0;

    memset(tmp, 0, sizeof(char) * 100 * BUFFER_SIZE);

    strcpy(copy, buf);

    for (int i = 0; i < DATATYPE_CNT; i++) 
        if ((strstr(copy, data_return_type[i])) != NULL) { // data type이 있으면 메모리 할당
            mem_alloc = true;
            if (strstr(copy, "FileWriter") != NULL) // 하지만 FileWriter일 경우는 객체 생성으로 구분
                mem_alloc = false;
            break;
        }

    if (mem_alloc) { // 메모리 할당인 경우
        if (p_option) // p옵션시 변환한 함수에 new 추가
            p_overlap("new");

        tok = strtok(copy, " ");
        while (tok != NULL) {
            if (strncmp(tok, "new", 3)) { // 토큰분리
                strcpy(tmp[cnt++], tok);
            }
            tok = strtok(NULL, " [];=");
        }
        sprintf(result, "%s = malloc(sizeof(%s) * %s);", tmp[0], tmp[1], tmp[2]); // 메모리 할당 line으로 치환
        cfile_write(result);
    }
    else if (strstr(copy, "FileWriter") != NULL) { // FileWriter인 경우
        if (p_option) // p_option시에 변환한 함수 목록에 FileWriter(fopen)추가
            p_overlap("FileWriter");

        for (int i = 0; i < file_top; i++) {
            if ((strstr(copy, java_fp[i])) != NULL) {
                strtok(copy, " ");
                tok = strtok(NULL, " ");
                wr_name[i] = (char *)malloc(sizeof(char) * strlen(tok));
                strcpy(wr_name[i], tok); // FileWriter writer, writer 저장 (객체 이름 저장)

                strcpy(copy, buf);
                if (IOException) { // IOException 유무 별로 예외처리
                    if ((strstr(copy, "true")) != NULL)
                        sprintf(result, "if((%s = fopen(%s, \"a\")) == NULL) {", java_fp[i], java_fname[i]);
                    else if ((strstr(copy, "false")) != NULL)
                        sprintf(result, "if((%s = fopen(%s, \"w\")) == NULL) {", java_fp[i], java_fname[i]);

                    cfile_write(result);
                    cfile_write("fprintf(stderr, \"fopen error\\n\");");
                    cfile_write("exit(1);");
                    cfile_write("}");
                }
                else {
                    if ((strstr(copy, "true")) != NULL)
                        sprintf(result, "%s = fopen(%s, \"a\");", java_fp[i], java_fname[i]);
                    else if ((strstr(copy, "false")) != NULL)
                        sprintf(result, "%s = fopen(%s, \"w\");", java_fp[i], java_fname[i]);

                    cfile_write(result);
                }
                return;
            }
        }
    }
    else { // 생성자 함수호출화 ex ) Stack();
        tok = strtok(copy, " ");
        sprintf(constructor, "%s();", tok);

        cfile_write(constructor);

        tok = strtok(NULL, " ");
        con_name[con_count] = (char *)malloc(sizeof(char) * strlen(tok));
        sprintf(con_name[con_count++], "%s.", tok);
    }

    free(copy);
    free(result);
    free(constructor);
}

void is_scanf(char *buf) { // nextInt -> scanf 작업
    char *var;
    char *copy = malloc(sizeof(char) * strlen(buf));
    char *scanf = malloc(sizeof(char) * BUFFER_SIZE);

    strcpy(copy, buf);

    var = strtok(copy, " ="); // 변수명 받음

    sprintf(scanf, "scanf(\"%%d\", &%s);", var); // 토큰분리해서 scanf문으로 바꿈
    cfile_write(scanf);
}

void is_printf(char *buf) {
    char tmp[BUFFER_SIZE];
    char *copy = malloc(sizeof(char) * BUFFER_SIZE);

    memset(tmp, 0, sizeof(char) * BUFFER_SIZE);

    strcpy(tmp, buf);
    copy = strstr(tmp, "printf"); // print문의 경우 java에서 system.out.printf에서 printf부터는 c와 문법이 같으므로 printf앞으로는 잘라냄
    cfile_write(copy);
}

void is_main(char *buf) { // main함수 치환
    char *main = (char *) malloc (sizeof(char) * BUFFER_SIZE);

    strcpy(main, "int main(int argc, char *argv[])");

    if (count_brace(buf)) // main함수 뒤에 중괄호가 붙어있다면 중괄호 추가
        strcat(main, " {");

    if ((strstr(buf, "IOException")) != NULL) // IOException 처리를 위한 체크
        IOException = true;

    cfile_write(main);
}

void is_method(char *buf) { // 메소드 선언에서 public을 지워주고 생성자에는 리턴 자료형이 붙어 있지 않기 때문에 자료형을 붙여준다
    char *copy = malloc(sizeof(char) * BUFFER_SIZE);
    char *temp = malloc(sizeof(char) * BUFFER_SIZE);
    char *public = "public";
    int chk = 0;

    strcpy(copy, buf);

    if (!strncmp(copy, public, strlen(public))) { // 앞에 붙어있는 public 제거
        copy += strlen(public) + 1;
    }

    for (int i = 0; i < DATATYPE_CNT; i++, chk++) { // q2와 같이 메인함수가 없는 class에서 구현한 메소드의 프로토타입이 필요할 수 있으므로 메소드(함수)의 프로토타입 저장
        if (strstr(copy, data_return_type[i]) != NULL) { // 리턴형이 붙어있다면 바로 파일에 쓴다 (생성자 X, 일반 메소드)
            cfile_write(copy);
            for (int j = strlen(copy); j > 0; j--) {
                if (copy[j] != ')') // 괄호이후 문구 전부 제거하고 괄호바로 다음에 세미콜론을 붙여서 저장
                    copy[j] = '\0';
                else {
                    copy[j+1] = ';';
                    extern_buf[extern_count] = (char *)malloc(sizeof(char) * strlen(copy));
                    strcpy(extern_buf[extern_count++], copy);
                }
            }
            break;
        }
        else
            continue;
    }

    if (chk >= DATATYPE_CNT) { // 만약 리턴 자료형을 찾지 못한 경우(생성자)
        sprintf(temp, "void %s", copy); // void를 붙여서 저장
        cfile_write(temp);

        for (int j = strlen(temp); j > 0; j--) { // 마찬가지로 프로토타입 저장
            if (temp[j] != ')')
                temp[j] = '\0';
            else {
                temp[j+1] = ';';
                extern_buf[extern_count] = (char *)malloc(sizeof(char) * strlen(temp));
                strcpy(extern_buf[extern_count++], temp);
            }
        }
    }
}

void is_variable(char *buf) { // [] -> *, final -> const
    char *copy = malloc(sizeof(char) * strlen(buf));
    char *var_type;
    char *name;
    char *re = malloc(sizeof(char) * strlen(buf));
    char *tok;

    strcpy(copy, buf);

    if (strstr(copy, "[]") != NULL) { // java int[] a -> c int *a 작업
        var_type = strtok(copy, " []");
        name = strtok(NULL, " []");
        sprintf(copy, "%s *%s", var_type, name);
    }

    if (strstr(copy, "public") != NULL) { // public 제거
        copy = copy + strlen("public") + 1;
    }

    if (strstr(copy, "final") != NULL) { // final은 이미 위에서 define으로 처리했으므로 함수종료
        return;
    }

    if (!strncmp(copy, "File", 4)) {
        tok = strtok(copy, " ="); // File file -> FILE *file을 위한 작업
        tok = strtok(NULL, " =");
        java_fp[file_top] = (char *)malloc(sizeof(char) * (strlen(tok) + 1));
        strcpy(java_fp[file_top], tok);
        sprintf(re, "FILE *%s;", java_fp[file_top]);

        strcpy(copy, buf); // 파일명을 가져오기위한 작업
        tok = strstr(copy, "\"");
        for (int i = strlen(tok); i > 0; i--) {
            if (tok[i] == ')' || tok[i] == ';')
                tok[i] = '\0';
        }
        java_fname[file_top] = (char *)malloc(sizeof(char) * (strlen(tok) + 1));
        strcpy(java_fname[file_top], tok);
        file_top++; // 파일 개수 카운트

        strcpy(copy, re);
    }

    cfile_write(copy);
    free(copy);
    free(re);
}

void check_var(int class_brace) { // 변환 시작 시점에 보통 c를 구현하는 방식에 맞추기위해 변수선언을 최상단으로 올림 (추가 구현)
    char buf[BUFFER_SIZE];
    off_t cur_pos;
    char *ptr;

    memset(buf, 0, sizeof(char) * BUFFER_SIZE);
    add_header();

    if ((!prev_main) && (extern_count)) {
        for (int i = 0; i < extern_count; i++) {
            cfile_write(extern_buf[i]);
        }
        cfile_write(""); // 가독성을 위한 한 칸 개행
    }

    if (!class_brace) { // 만약 class문에 중괄호가 없다면
        fgets(buf, BUFFER_SIZE, fp1); // offset을 한 줄만큼 이동
    }

    cur_pos = ftell(fp1); // 파일을 검색하기전 위치 저장

    while (fgets(buf, BUFFER_SIZE, fp1) != NULL) {
        strcpy(buf, ltrim(buf));
        buf[strlen(buf) - 1] = '\0'; // 맨 끝 개행문자 제거

        if (!strlen(buf))
            continue;

        if (strstr(buf, "main") != NULL) {
            prev_main = true; // q2같이 메인함수가 없는 함수와 있는 함수의 구분이 필요한 경우를 위한 처리
            is_main(buf); // 메인인 경우 함수 호출
            if (!count_brace(buf)) { // main 끝에 중괄호가 안달려있는 경우
                cur_pos = ftell(fp1); 
                fgets(buf, BUFFER_SIZE, fp1);
                strcpy(buf, ltrim(buf));

                buf[strlen(buf) - 1] = '\0';

                if (strlen(buf) == 1 && buf[0] == '{') { // 만약 main에 중괄호가 없고 바로 다음줄이 중괄호만 있다면 중괄호 추가
                    cfile_write("{");
                    cur_pos = ftell(fp1); // 다시 위치 저장
                }
            }
        }

        class_brace += count_brace(buf); // 중괄호 개수 카운트

        if (!class_brace) { // 닫는 괄호와 여는 괄호의 개수가 맞으면 종료
            fseek(fp1, cur_pos, SEEK_SET);
            return;
        }

        if (seperation_datatype(buf) != -1) {
            is_variable(buf); // 변수 처리
        }
    }

}

void is_class(char *buf) { // class의 경우 c file open 작업
    char tmp[BUFFER_SIZE];
    char *tok;
    int length = 0;

    memset(tmp, 0, sizeof(char) * BUFFER_SIZE);
    strcpy(tmp, buf);
    tok = strtok(tmp, " ");

    while (1) { // class선언 맨 뒤 부분만 가져오기 위한 작업 (c file 이름을 위해서)
        if (!strncmp(tok, "class", 5))
            break;
        tok = strtok(NULL, " ");
    }

    tok = strtok(NULL, " ");
    length = strlen(tok);

    for (int i = 0; i < length; i++) { // 중괄호와 공백을 제거
        if (tok[i] == '{' || tok[i] == ' ')
            tok[i] = '\0';
    }

    fname[file_count] = (char *) malloc(sizeof(char) * strlen(tok));

    strcpy(fname[file_count], tok);
    strcat(fname[file_count], ".c"); // class이름 + .c

    c_fp = fopen(fname[file_count], "w+"); // file open
    check_var(count_brace(buf)); // 변수 최상단으로 올리는 함수
}

void is_return(char *buf) { // 특별한 값을 반환하는것이 아니라면 return을 exit로 치환
    char *copy = malloc(sizeof(char) * BUFFER_SIZE);
    char *exit = "exit(0);";

    strcpy(copy, buf);

    if ((copy = strstr(copy, " ")) != NULL) {
        copy += 1; // 맨 앞 공백제거
    }

    if (*copy == ';' || *copy == '0') // return 0; or return ;
    cfile_write(exit);
    else
        cfile_write(buf); // 이외의 리턴값은 그대로 출력
}

void is_if(char *buf) { // buf 맨마지막에 중괄호가 없으면 일시적으로 brace += 1;
    for (int i = 0; i < file_top; i++) {
        if (strstr(buf, wr_name[i]) != NULL) {
            return ;
        }
    }

    cfile_write(buf);
}

void is_write(char *buf) { // file write -> fputs 처리
    char *copy = (char *)malloc(sizeof(char) * strlen(buf));
    char *next = (char *)malloc(sizeof(char) * strlen(buf));
    char *result = (char *)malloc(sizeof(char) * BUFFER_SIZE);
    int str_len = 0;

    strcpy(copy, buf);

    for (int i = 0; i < file_top; i++) {
        if (!strlen(wr_name[i]))
            continue;

        if ((strstr(copy, wr_name[i])) != NULL) {
            copy += strlen(wr_name[i]); // 앞에 붙어있는 객체명 제거
            next = strstr(copy, "("); // FileWriter와 분리

            for (int i = strlen(next); i > 0; i--)
                if (next[i] == ';' || next[i] == ')') // 뒷부분 제거
                    next[i] = '\0';
            sprintf(result, "fputs%s, %s);", next, java_fp[i]); // fputs로 변환
            cfile_write(result);
            return ;
        }
    }

    free(copy);
    free(next);
    free(result);
}

void is_flush_close(char *buf) { // fflush의 경우 객체이름과 매칭된 파일 포인터를 찾아서 괄호안에 넣어주고 맨앞에 f만 추가해주는 작업
    char *copy = (char *)malloc(sizeof(char) * strlen(buf));

    strcpy(copy, buf);

    for (int i = 0; i < file_top; i++) {
        if ((strstr(copy, wr_name[i])) != NULL) { 
            copy += strlen(wr_name[i]);
            copy[0] = 'f'; // flush -> fflush
        }

        for (int j = 0; j < strlen(buf); j++) {
            if (copy[j] == '(') {
                strcpy(copy + (j+1), java_fp[i]); // 객체명과 파일포인터를 같은 인덱스로 저장했으므로 배열에서 꺼내온다
                strcat(copy, ");");
                cfile_write(copy);
                return;
            }
        }
    }
}

void p_overlap(char *buf) { // new, FileWriter같은 따로 처리가 필요한 함수들이 p option시 출력될 수 있도록 처리
    bool overlap = 0;

    for (int i = 0; i < KEYWORD_CNT; i++) {
        if (!strncmp(buf, KEYWORD[i].java_keyword, strlen(KEYWORD[i].java_keyword))) {
            for (int j = 0; j < p_count; j++)
                if (p_check[j] == i) // 중복인 경우
                    overlap = true;

            if(!overlap) // 중복이 아니라면 구조체 인덱스를 배열에 추가
                p_check[p_count++] = i;
        }
    }
}
