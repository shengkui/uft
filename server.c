/******************************************************************************
*
* FILENAME:
*     server.c
*
* DESCRIPTION:
*     A server to transmit file via datagram socket(UDP).
*
* REVISION(MM/DD/YYYY):
*     05/06/2015  Shengkui Leng (lengshengkui@outlook.com)
*     - Initial version 
*
******************************************************************************/
#include <unistd.h>
#include <libgen.h>
#include <signal.h>
#include "dsc.h"


volatile sig_atomic_t loop_flag = 1;

static uint32_t g_sequence_number = 0;
static FILE *g_fp = NULL;

/*
 * Save data to file
 */
dsc_command_t *cmd_save_data(dsc_command_t *req)
{
    dsc_command_t *res;
    dsc_request_send_data_t *req_data = (dsc_request_send_data_t *)req;
    int rc = STATUS_SUCCESS;

    //printf("CMD_SEND_DATA\n");
    //printf("SEQ number: %d\n", req_data->common.seq_no);

    do {
        if (g_sequence_number != req_data->common.seq_no) {
            printf("Invalid SEQ number\n");
            rc = STATUS_INVALID_SEQ_NO;
            break;
        }

        int len = req_data->common.data_len;
        if (len == 0) {
            /* The end of file, close it. */
            printf("File saved\n");
            fclose(g_fp);
            g_fp = NULL;
            break;
        }

        int bytes = fwrite(req_data->data, 1, len, g_fp);
        if (bytes < len) {
            perror("write file error");
            rc = STATUS_WRITE_FILE_ERROR;
            break;
        }

        printf("Write data: %d\n", bytes);
        g_sequence_number++;
    } while (0);

    res = (dsc_command_t *)malloc(sizeof(dsc_command_t));
    if (res != NULL) {
        res->status = rc;
        res->seq_no = req_data->common.seq_no;
        res->data_len = 0;
    }

    return (dsc_command_t *)res;
}


/*
 * Start a new file transmition
 */
dsc_command_t *cmd_start(dsc_command_t *req)
{
    dsc_command_t *res;
    dsc_request_start_t *req_data = (dsc_request_start_t *)req;
    int rc = STATUS_SUCCESS;

    //printf("CMD_START\n");
    printf("File: %s\n", basename(req_data->filename));

    if (g_fp) {
        fclose(g_fp);
        g_fp = NULL;
    }

    g_fp = fopen(basename(req_data->filename), "w");
    if (NULL == g_fp) {
        perror("open file error");
        rc = STATUS_OPEN_FILE_ERROR;
    }

    g_sequence_number = req_data->common.seq_no + 1;

    res = (dsc_command_t *)malloc(sizeof(dsc_command_t));
    if (res != NULL) {
        res->status = rc;
        res->seq_no = req_data->common.seq_no;
        res->data_len = 0;
    }

    return (dsc_command_t *)res;
}


/*
 * Unknown request type
 */
dsc_command_t *cmd_unknown(dsc_command_t *req)
{
    dsc_command_t *res;

    printf("Unknown request type\n");

    res = (dsc_command_t *)malloc(sizeof(dsc_command_t));
    if (res != NULL) {
        res->status = STATUS_INVALID_COMMAND;
        res->data_len = 0;
    }

    return res;
}


/*
 * The handler to process all requests from client
 */
dsc_command_t *my_request_handler(dsc_command_t *req)
{
    dsc_command_t *resp = NULL;

    switch (req->command) {
    case CMD_SEND_DATA:
        resp = cmd_save_data(req);
        break;

    case CMD_START:
        resp = cmd_start(req);
        break;

    default:
        resp = cmd_unknown(req);
        break;
    }

    return resp;
}


/*
 * When user press CTRL+C, quit the server process.
 */
void handler_sigint(int sig)
{
    loop_flag = 0;
}

void install_sig_handler()
{
    struct sigaction act;

    sigemptyset(&act.sa_mask);
    act.sa_handler = handler_sigint;
    act.sa_flags = 0;
    sigaction(SIGINT, &act, 0);
}


/******************************************************************************
 * NAME:
 *      print_usage
 *
 * DESCRIPTION: 
 *      Print usage information and exit the program.
 *
 * PARAMETERS:
 *      pname - The name of the program.
 *
 * RETURN:
 *      None
 ******************************************************************************/
void print_usage(char *pname)
{
    printf("\n"
        "===================================================\n"
        "    Server to transmit file via datagram socket    \n"
        "                      v%d.%d                       \n"
        "===================================================\n"
        "\n"
        "Usage: %s [-p port_number]\n"
        "\n"
        "Options:\n"
        "    -p port_number   The port number of server, default: %d\n"
        "\n"
        "Example:\n"
        "    %s -p 9000\n"
        "\n",
        VERSION_MAJOR, VERSION_MINOR,
        pname, SERVER_PORT, pname
        );
    exit(STATUS_ERROR);
}


int main(int argc, char *argv[])
{
    dsc_server_t *s;
    char *pname = argv[0];
    int serv_port = SERVER_PORT;
    int opt;

    while ((opt = getopt(argc, argv, ":hp:")) != -1) {
        switch (opt) {
        case 'p':
            serv_port = strtol(optarg, NULL, 10);
            if (serv_port <= 0) {
                printf("Error: invalid port number!\n");
                print_usage(pname);
            }
            break;

        case 'h':
            print_usage(pname);
            break;

        case ':':
            printf("Error: option '-%c' needs a value\n", optopt);
            print_usage(pname);
            break;

        case '?':
        default:
            printf("Error: invalid option '-%c'\n", optopt);
            print_usage(pname);
            break;
        }
    }
    if (optind < argc) {
        printf("Error: invalid argument '%s'\n", argv[optind]);
        print_usage(pname);
    }

    printf("Server listening on port %d\n", serv_port);
    s = server_init(&my_request_handler, serv_port);
    if (s == NULL) {
        printf("Error: server init error\n");
        return STATUS_INIT_ERROR;
    }

    install_sig_handler();

    while (loop_flag) {
        server_accept_request(s);
    }

    server_close(s);
    return STATUS_SUCCESS;
}
