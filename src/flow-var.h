/* Copyright (c) 2008 Victor Julien <victor@inliniac.net> */
#ifndef __FLOW_VAR_H__
#define __FLOW_VAR_H__

#include "flow.h"
#include "util-var.h"

typedef struct FlowVar_ {
    uint8_t type; /* type, DETECT_FLOWVAR in this case */
    uint16_t idx; /* name idx */
    GenericVar *next; /* right now just implement this as a list,
                       * in the long run we have think of something
                       * faster. */
    uint8_t *value;
    uint16_t value_len;
} FlowVar;

void FlowVarAdd(Flow *, uint8_t, uint8_t *, uint16_t);
FlowVar *FlowVarGet(Flow *, uint8_t);
void FlowVarFree(FlowVar *);
void FlowVarPrint(GenericVar *);

#endif /* __FLOW_VAR_H__ */

