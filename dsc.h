/******************************************************************************
*
* FILENAME:
*     dsc.h
*
* DESCRIPTION:
*     Define some structure for datagram socket communication(UDP).
*
* REVISION(MM/DD/YYYY):
*     07/30/2014  Shengkui Leng (lengshengkui@outlook.com)
*     - Initial version 
*     05/06/2015  Shengkui Leng (lengshengkui@outlook.com)
*     - Add new request types and structures
*
******************************************************************************/
#ifndef _DSC_H_
#define _DSC_H_
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* Version of the programm */
#define VERSION_MAJOR           1
#define VERSION_MINOR           0

/*--------------------------------------------------------------
 * Definition for both client and server
 *--------------------------------------------------------------*/
/* Default IP address and port number of server */
#define SERVER_IP               "127.0.0.1"
#define SERVER_PORT             7777

/* The read/write buffer size of socket */
#define DSC_BUF_SIZE            4096

/* The signature of the request/response packet */
#define DSC_SIGNATURE           0xDEADBEEF

/* Make a structure 1-byte aligned */
#define BYTE_ALIGNED            __attribute__((packed))


/* Request type */
enum dsc_request_type {
    CMD_START = 0x8001,         /* Send filename to server */
    CMD_SEND_DATA,              /* Send data to server */

    CMD_UNKNOWN                 /* */
};

/* Status code, start from 160 to skip all system pre-defined error code */
enum dsc_status_code {
    STATUS_SUCCESS = 0,         /* Success */
    STATUS_ERROR = 160,         /* Generic error */
    STATUS_INIT_ERROR,          /* Server/client init error */
    STATUS_INVALID_COMMAND,     /* Unknown request type */
    STATUS_INVALID_SEQ_NO,      /* Invalid sequence number */
    STATUS_OPEN_FILE_ERROR,     /* Open file error */
    STATUS_WRITE_FILE_ERROR,    /* Write file error */
    STATUS_READ_FILE_ERROR,     /* Read file error */

    STATUS_UNKNOWN              /* */
};

/* Common header of both request/response packets */
typedef struct dsc_command {
    uint32_t signature;         /* Signature, shall be DSC_SIGNATURE */
    union {
        uint32_t command;       /* Request type, refer dsc_request_type */
        uint32_t status;        /* Status code of response, refer dsc_status_code */
    };
    uint32_t seq_no;            /* Sequence number */
    uint32_t data_len;          /* The data length of packet */

    uint16_t checksum;          /* The checksum of the packet */
} BYTE_ALIGNED dsc_command_t;


/* Request for CMD_START */
#define DSC_FILENAME_LEN    1024
typedef struct dsc_request_start {
    dsc_command_t common;               /* Common header of request */
    char filename[DSC_FILENAME_LEN];    /* File name */
} BYTE_ALIGNED dsc_request_start_t;

/* Request for CMD_SEND_DATA */
#define DSC_DATA_SIZE       2048
typedef struct dsc_request_send_data {
    dsc_command_t common;           /* Common header of request */
    char data[DSC_DATA_SIZE];       /* Data */
} BYTE_ALIGNED dsc_request_send_data_t;


/*--------------------------------------------------------------
 * Definition for client only
 *--------------------------------------------------------------*/

/* Keep the information of client */
typedef struct dsc_client {
    int sockfd;                     /* Socket fd of the client */
    struct sockaddr_in serv_addr;   /* Server address */
} dsc_client_t;


dsc_client_t *client_init(const char *server_ip, int server_port);
dsc_command_t *client_send_request(dsc_client_t *c, dsc_command_t *req);
void client_close(dsc_client_t *c);


/*--------------------------------------------------------------
 * Definition for server only
 *--------------------------------------------------------------*/

typedef dsc_command_t * (*request_handler_t) (dsc_command_t *);

/* Keep the information of server */
typedef struct dsc_server {
    int sockfd;                         /* Socket fd of the server */
    struct sockaddr_in addr;            /* Server address */
    request_handler_t request_handler;  /* Function pointer of the request handle */
} dsc_server_t;


dsc_server_t *server_init(request_handler_t req_handler, int port);
int server_accept_request(dsc_server_t *s);
void server_close(dsc_server_t *s);


#endif /* _DSC_H_ */
