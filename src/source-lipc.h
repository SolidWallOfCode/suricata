/* Copyright (C) 2023 Aviatrix
 *
 */

/**
 * \file
 *
 * \author Alan M. Carroll (amc@apache.org)
 */

#ifndef __SOURCE_LIPC_H__
#define __SOURCE_LIPC_H__

#include <unistd.h>
#include <stdint.h>

/* per packet LIPC vars */
typedef struct LIPCPacketVars_
{
    uint32_t tenant_id;
} LIPCPacketVars;

/** needs to be able to contain Windows adapter id's, so
 *  must be quite long. */
#define LIPC_IFACE_NAME_LENGTH 128

typedef struct LIPCIfaceConfig_
{
    /* number of threads */
    int threads;
    /* Request buffer size */
    int req_buffer_size;

    // Standard config reference couting.
    SC_ATOMIC_DECLARE(unsigned int, ref);
    void (*DerefFunc)(void *);

} LIPCIfaceConfig;

/// Named used to identify the socket.
extern char const * const SOCKET_NAME;
/// Length of the name in the socket address structure.
extern size_t const SOCKET_NAME_LEN;
/// Length of the overall socket address structure.
extern size_t const SOCKET_ADDR_LEN;
/// Buffer size for packets
extern size_t const SOCKET_BUFFER_LEN;

/// Open a server socket to listen for connections.
/// @return The socket file descriptor or -1 on failure.
int av_open(char const ** err_text);

/// Connect to the server.
/// @return The socket file descriptor or -1 on failure.
int av_connect(char const ** err_text);

extern uint16_t const AV_VERSION;

struct av_request {
    uint16_t version; ///< Protocol version.
    uint16_t length; ///< Length of request.
    uint32_t id; ///< Transaction identifier.
};

struct av_response {
    uint16_t version; ///< Protocol version.
    uint16_t length; ///< Length of request.
    uint16_t result; ///< Result of inspection.
    uint16_t pad; ///< Future expansion.
    uint32_t id; ///< Transaction identifier.
};

#endif /* __SOURCE_LIPC_H__ */
