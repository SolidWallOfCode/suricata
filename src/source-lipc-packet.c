/* Copyright (C) 2023 Aviatrix
 */

/**
 *  \defgroup afppacket AF_PACKET running mode
 *
 *  @{
 */

/**
 * \file
 *
 * \author Alan M. Carroll <amc@apache.org>
 *
 * Local IPC packet source.
 *
 */

extern uint16_t max_pending_packets;

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

static char const * const LIPC_SOCKET_NAME = "\0suricata";
static size_t const LIPC_SOCKET_NAME_LEN = sizeof(LIPC_SOCKET_NAME) - 1;
static size_t const LIPC_SOCKET_ADDR_LEN = sizeof(((struct sockaddr_un*)0)->sun_family) + LIPC_SOCKET_NAME_LEN;
static size_t const LIPC_SOCKET_BUFFER_LEN = 4096;

static uint16_t const LIPC_VERSION = 1;

static void lipc_init_addr(struct sockaddr_un * sa) {
    sa->sun_family = AF_UNIX;
    memcpy(&sa->sun_path, LIPC_SOCKET_NAME, LIPC_SOCKET_NAME_LEN);
}

int lipc_open(char const ** err_text) {
    int fd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    *err_text = NULL;
    if (fd >= 0) {
        struct sockaddr_un addr;
        av_init_addr(&addr);
        int r = bind(fd, (struct sockaddr *)&addr, SOCKET_ADDR_LEN);
        if (r == 0) {
            r = listen(fd, 1);
            if (r != 0) {
                *err_text = "Unable to listen";
            }
        } else {
            *err_text = "Unable to bind socket";
        }
    } else {
        *err_text = "Failed to open socket";
    }

    if (*err_text != NULL) {
        if (fd >= 0) {
            close(fd);
            fd = -1;
        }
    }

    return fd;
}

/**
 * \brief Structure to hold thread specific variables.
 *
 * @internal Single structure shared among threads?
 */
typedef struct LIPCThreadVars_
{
    /* counters */
    uint64_t pkts;

    ThreadVars *tv;
    TmSlot *slot;

    uint64_t send_errors_logged; /**< snapshot of send errors logged. */

    int fd; ///< IPC socket.
} LIPCThreadVars;

static void ReceiveLIPCThreadExitStats(ThreadVars *, void *);

/** Initialize the socket address.
 *
 * @param sa Addres to initialize.
 */
static void av_init_addr(struct sockaddr_un * sa) {
    sa->sun_family = AF_UNIX;
    memcpy(&sa->sun_path, SOCKET_NAME, SOCKET_NAME_LEN);
}

/** Open a server socket.
 *
 * @param err_text [out] Error text, if any.
 * @return The file descriptor.
 *
 * If an error occurs, a negative file descriptor is returned.
 */
static int av_open(char const ** err_text) {
    int fd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    *err_text = NULL;
    if (fd >= 0) {
        struct sockaddr_un addr;
        av_init_addr(&addr);
        int r = bind(fd, (struct sockaddr *)&addr, SOCKET_ADDR_LEN);
        if (r == 0) {
            r = listen(fd, 1);
            if (r != 0) {
                *err_text = "Unable to listen";
            }
        } else {
            *err_text = "Unable to bind socket";
        }
    } else {
        *err_text = "Failed to open socket";
    }

    if (*err_text != NULL) {
        if (fd >= 0) {
            close(fd);
            fd = -1;
        }
    }

    return fd;
}


// ----
static TmEcode ReceiveLIPCThreadInit(ThreadVars *tvars, const void *config_data, void ** ctx_data) {
    SCEnter();

    if (NULL == config_data) {
        SCLogError("error: initdata == NULL [LIPC]");
        SCReturnInt(TM_ECODE_OK);
    }

    LIPCThreadVars *ctx = SCMalloc(sizeof(*ctx));
    if (ctx == NULL) {
        SCLogError("error: Failed to allocated thread vars [LIPC]");
        SCReturnInt(TM_ECODE_OK);
    }
    *ctx_data = ctx;

    // Open up the IPC
    char const * err_text = NULL;
    ctx->fd = av_open(&err_text);
    if (ctx->fd < 0) {
        SCLogError("error: Failed to open IPC - %s [%d]", err_text, errno);
        SCReturnInt(TM_ECODE_FAILED);
    }

    SCReturnInt(TM_ECODE_OK);
}

static TmEcode ReceiveLIPCLoop(ThreadVars *tv, void *data, void *slot)
{
    LIPCThreadVars *ctx = (LIPCThreadVars *)data;
    char buff[LIPC_SOCKET_BUFFER_LEN];
    struct av_request * request = (struct av_request*)buff;

    while (ctx->fd >= 0) {
        int c_fd = accept(ctx->fd, NULL, NULL);
        if (c_fd >= 0) {
            while (1) {
                ssize_t len = recv(c_fd, buff, sizeof(buff), 0);
                if (len <= 0) {
                    close(c_fd);
                    break;
                }
                if (len != request->length) {
                    printf("Invalid length - received=%d length=%d\n", len, request->length);
                } else {
                    struct av_response resp;
                    resp.version = AV_VERSION;
                    resp.length = sizeof(resp);
                    resp.id = request->id;
                    resp.result = (++R) % 5;
                    resp.pad = 0;
                    send(c_fd, &resp, sizeof(resp), 0);
                }
            }
        }
    }
}

static TmEcode ReceiveLIPCThreadDeinit(ThreadVars *tvars, void *data) {
}
// ----

void TmModuleReceivedLICPRegister(void)
{
    tmm_modules[TMM_RECEIVELIPC].name = "RECEIVELIPC";
    tmm_modules[TMM_RECEIVELIPC].ThreadInit = ReceiveLIPCThreadInit;
    tmm_modules[TMM_RECEIVELIPC].Func = NULL;
    tmm_modules[TMM_RECEIVELIPC].PktAcqLoop = ReceiveLIPCLoop;
    tmm_modules[TMM_RECEIVELIPC].PktAcqBreakLoop = NULL;
    tmm_modules[TMM_RECEIVELIPC].ThreadExitPrintStats = ReceiveLIPCThreadExitStats;
    tmm_modules[TMM_RECEIVELIPC].ThreadDeinit = ReceiveLIPCTheadDeinit;
    tmm_modules[TMM_RECEIVELIPC].cap_flags = 0;
    tmm_modules[TMM_RECEIVELIPC].flags = TM_FLAG_RECEIVE_TM;
}

static TmECode DecodeLIPC(ThreadVars *tv, Packet *p, void *data) {
}

static TmEcode DecodeLIPCThreadInit(ThreadVars * tvars, void const *config_data, void **ctx_data) {
    SCReturnInt(TM_ECODE_OK);
}

static TmEcode DecodeLIPCThreadInit(ThreadVars *, const void *, void **);
static TmEcode DecodeLIPCThreadDeinit(ThreadVars *tv, void *data);
static TmEcode DecodeLIPC(ThreadVars *, Packet *, void *);


/**
 * \brief Registration Function for DecodeLIPC.
 */
void TmModuleDecodeAFPRegister (void)
{
    tmm_modules[TMM_DECODELIPC].name = "DecodeLIPC";
    tmm_modules[TMM_DECODELIPC].ThreadInit = DecodeLIPCThreadInit;
    tmm_modules[TMM_DECODELIPC].Func = NULL;
    tmm_modules[TMM_DECODELIPC].ThreadExitPrintStats = NULL;
    tmm_modules[TMM_DECODELIPC].ThreadDeinit = NULL;
    tmm_modules[TMM_DECODELIPC].cap_flags = 0;
    tmm_modules[TMM_DECODELIPC].flags = TM_FLAG_DECODE_TM;
}

