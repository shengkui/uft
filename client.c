/******************************************************************************
*
* FILENAME:
*     client.c
*
* DESCRIPTION:
*     A client to transmit file via datagram socket(UDP).
*
* REVISION(MM/DD/YYYY):
*     05/06/2015  Shengkui Leng (lengshengkui@outlook.com)
*     - Initial version 
*
******************************************************************************/
#include <unistd.h>
#include <libgen.h>
#include "dsc.h"

#define MAX_RETRY_TIMES     3


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
        "    Client to transmit file via datagram socket    \n"
        "                      v%d.%d                       \n"
        "===================================================\n"
        "\n"
        "Usage: %s [-s server_ip] [-p port_number] <filename>\n"
        "\n"
        "Options:\n"
        "    -s server_ip     The IP address of server, default: %s\n"
        "    -p port_number   The port number of server, default: %d\n"
        "    filename         The file to transmit\n"
        "\n"
        "Example:\n"
        "    %s -s 192.168.1.100 -p 9000 test1.txt\n"
        "\n",
        VERSION_MAJOR, VERSION_MINOR,
        pname, SERVER_IP, SERVER_PORT,
        pname
        );
    exit(STATUS_ERROR);
}


int main(int argc, char *argv[])
{
    dsc_client_t *clnt;
    dsc_command_t *res;
    char *fname = NULL;
    char *pname = argv[0];
    const char *server_ip = SERVER_IP;
    int serv_port = SERVER_PORT;
    int opt;
    int rc = STATUS_SUCCESS;
    int retry_count;
    uint32_t sequence_no = 0;

    while ((opt = getopt(argc, argv, ":hp:s:")) != -1) {
        switch (opt) {
        case 'p':
            serv_port = strtol(optarg, NULL, 10);
            if (serv_port <= 0) {
                printf("Error: invalid port number!\n");
                print_usage(pname);
            }
            break;

        case 's':
            server_ip = optarg;
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
    if ((optind+1) == argc) {
        fname = argv[optind];
    } else {
        print_usage(pname);
    }

    FILE *fp = fopen(fname, "r");
    if (NULL == fp) {
        perror("Open file error");
        return STATUS_OPEN_FILE_ERROR;
    }

    printf("Connect server %s:%d\n", server_ip, serv_port);
    clnt = client_init(server_ip, serv_port);
    if (clnt == NULL) {
        printf("Error: client init error\n");
        fclose(fp);
        return STATUS_INIT_ERROR;
    }

    dsc_request_start_t req_st;

    req_st.common.command = CMD_START;
    req_st.common.seq_no = sequence_no;
    req_st.common.data_len = strlen(basename(fname));
    snprintf(req_st.filename, DSC_FILENAME_LEN-1, "%s", basename(fname));
    req_st.filename[DSC_FILENAME_LEN-1] = 0;

    printf("Start file transmition(send filename to server)...\n");
    retry_count = MAX_RETRY_TIMES;
    while (retry_count-- > 0) {
        res = client_send_request(clnt, (dsc_command_t *)&req_st);
        if (res == NULL) {
            printf("Error: client send request error\n");
            rc = STATUS_ERROR;
            continue;
        }

        rc = res->status;
        free(res);
        if (rc != STATUS_SUCCESS) {
            printf("CMD_START error(%d)\n", rc);
            continue;
        } else {
            /* CMD_START OK */
            break;
        }
    }

    if (rc != STATUS_SUCCESS) {
        printf("Fail to transmit file name\n");
        fclose(fp);
        client_close(clnt);
        return rc;
    }

    sequence_no++;

    int trans_end = 0;
    do {
        dsc_request_send_data_t req_dt;

        int len = fread(req_dt.data, 1, DSC_DATA_SIZE-1, fp);
        if (len < 0) {
            perror("read file error");
            rc = STATUS_READ_FILE_ERROR;
            break;
        } else if (len == 0) {
            /* The end of file, set the trans_end flag and send a packet with 
             * data_len set to 0 to inform server, the file has been
             * transmitted successfully.
             */
            trans_end = 1;
            printf("End of file transmition\n");
        }

        req_dt.common.command = CMD_SEND_DATA;
        req_dt.common.data_len = len;
        req_dt.common.seq_no = sequence_no;

        retry_count = MAX_RETRY_TIMES;
        while (retry_count-- > 0) {
            res = client_send_request(clnt, (dsc_command_t *)&req_dt);
            if (res == NULL) {
                printf("Error: send request error\n");
                rc = STATUS_ERROR;
                continue;
            }

            rc = res->status;
            free(res);
            if (rc != STATUS_SUCCESS) {
                printf("CMD_SEND_DATA error(%d)\n", rc);
                continue;
            } else {
                /* CMD_SEND_DATA OK */
                break;
            }
        }

        sequence_no++;
    } while (!trans_end);

    fclose(fp);
    client_close(clnt);
    return rc;
}
