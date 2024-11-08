
#include "libbb.h"

#define DISCLAIMER_CONTENT   "/etc/wg/logon_banner/eula.inc"
#define DISCLAIMER_TITLE     "/etc/wg/logon_banner/title.inc"

#define   MAX_PRINT_LEN    64

#define  ACCEPT "\nYou must read and accept the Logon Disclaimer message.\nI have read and accept the Logon Disclaimer message.Yes or No?"

/* Macro to build Ctrl-LETTER. Assumes ASCII dialect */
#define CTL(x)          ((x) ^ 0100)

/*
 * Display before the issue
 * If DISCLAIMER_TITLE and DISCLAIMER_CONTENT are not defined, getty will never display
 * the contents of the /etc/wg/logon_banner/eula.inc and /etc/wg/logon_banner/title.inc
 * file. 
 */

static int read_file(char* path, char* dst, int size)
{
    FILE *fp = NULL;
    char tmp[20480] = {0};
    char output[1024]= {0} ;
    int  nread = 0;
    if (path && dst) {
        /*Check whether the file exist or not*/
        if (access(path, F_OK ) == -1) {
            return 0;
        }
	/*Open the file if it's accessable*/
        fp = fopen(path, "r");
        if (fp == NULL) {
            snprintf(output, sizeof(output), "Failed to open the file:%s", path);
            fputs(output, stderr);
            return -1;
        }
        nread = fread(tmp, sizeof(char), sizeof(tmp), fp);
        if (nread <= 0) {
            snprintf(output, sizeof(output), "Failed to read %s nread = %d.", path, nread);
            fputs(output, stderr);
            fclose(fp);
            return 0;
        }
        fclose(fp);
        snprintf(dst, size > nread? nread:size-1, "%s", tmp);
    }
    return nread;
}


static void format_output(const char* output)
{
    char *ptr = (char*)output;
    int  i = 0, new_ph = 1;
    if (output) {
        /*Print the header*/
        for (i = 0; i < MAX_PRINT_LEN; i++) {
            putc('-', stdout);
        }
        putc('\n', stdout);

        /*Print the body*/
        i = 0 ; 
        do {
            if (i >= MAX_PRINT_LEN - 2) {
                putc(' ', stdout);
                putc('-', stdout);
                putc('\n', stdout);
                i = 0; 
            } else if (i == 0) { 
                putc('-', stdout);
                putc(' ', stdout);
                i += 2;
                if (new_ph) {
                    putc(' ', stdout);
                    putc(' ', stdout);
                    i += 2;
                    new_ph = 0;
                }
            }else{
                if (*ptr == '\n' || *ptr == '\r') {
                    if (i < MAX_PRINT_LEN - 1) {
                        for ( ; i < MAX_PRINT_LEN -1; i++) {
                            putc(' ', stdout); 
                        } 
                        putc('-', stdout);
                        putc('\n', stdout);
                        if (*ptr == '\r') {
                            ptr ++;
                        }
                        new_ph = 1;
                        ptr ++;
                        i = 0;
                    }
                } else {
                    putc(*ptr, stdout);
                    ptr ++;
                    i ++;
                }
            }
        } while (*ptr != '\0');

        if (i < MAX_PRINT_LEN - 1) {
            for ( ; i < MAX_PRINT_LEN -1; i++) {
                putc(' ', stdout); 
            } 
            putc('-', stdout);
            putc('\n', stdout);
        }
        
        /*Print the fooder*/
        for (i = 0; i < MAX_PRINT_LEN; i++) {
            putc('-', stdout);
        }
        putc('\n', stdout);
    }
}

/*
 * Print disclaimer and
 * return 1 when get user input "yes"
 * return 0 when get user input "no"
 */
int display_disclaimer(void)
{
    char title[256] = {0}; /*255 is defined in schema*/
    char content[20480]  = {0}; /*20000 is define in schema*/
    int  nread = 0, accept = 0;
    char output[1024]= {0} ;
    char input[8] = {0}, c;

    nread = read_file(DISCLAIMER_TITLE, title, sizeof(title));
    if (nread <= 0) {
        return 0;
    }

    nread = read_file(DISCLAIMER_CONTENT, content, sizeof(content));
    if (nread <= 0) {
        return 0;
    }

    format_output(title);  
    format_output(content);  

    fputs("I have read and accept the Logon Disclaimer message.Yes or No?", stdout);
    fflush(NULL);

    do {
        nread = 0;
        memset(input, 0, sizeof(input));
        while(1){
            /* Do not report trivial EINTR/EIO errors */
            errno = EINTR; /* make read of 0 bytes be silent too */
            if (read(STDIN_FILENO, &c, 1) < 1) {
                if (errno == EINTR || errno == EIO) {
                    exit(EXIT_SUCCESS);
                }
            }
            switch(c){ 
            case '\r':
            case '\n':
                input[nread++] = '\0';
                goto read_done;
            case CTL('H'): 
            case 0x7f:
                if (nread > 0) {
                    full_write(STDOUT_FILENO, "\010 \010", 3);
                    nread --;
                }
                break;
            case CTL('U'):
                while(nread > 0){
                    full_write(STDOUT_FILENO, "\010 \010", 3);
                    nread --;
                }
                break;
            case CTL('C'):
            case CTL('D'):
                exit(EXIT_SUCCESS);
            case '\0':
                /* BREAK. If we have speeds to try,
                /* fall through and ignore it */
                return 0;
            default: 
                if ((unsigned char)c < ' ') {
                    /*Ignore the garbage characters*/ 
                } else if (nread < sizeof(input)) {
                    /* echo and store the character */ 
                    full_write(STDOUT_FILENO, &c, 1);
                    input[nread++] = c;
                } else {
                    goto read_done; 
                }
                break; 
            }
        }
read_done:

        if (nread <= 0) {
            fputs(ACCEPT, stdout);
            fflush_all();
        } else {
            if ((strncasecmp(input, "y", 1) == 0 && nread == 2)
                ||(strncasecmp(input, "ye", 2) == 0 && nread == 3)
                ||(strncasecmp(input, "yes", 3) == 0 && nread == 4)){
                accept = 1;
            } else {
                fputs(ACCEPT, stdout);
                fflush_all();
            }
        }
    } while(!accept);

    return 1;
}
