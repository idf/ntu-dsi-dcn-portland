
#include "openflow/openflow.h"
#include "openflow/nicira-ext.h"
#include "openflow/ericsson-ext.h"

extern "C"
{
#define private _private
#define delete _delete
#define list List

#include "openflow/private/csum.h"
#include "openflow/private/poll-loop.h"
#include "openflow/private/rconn.h"
#include "openflow/private/stp.h"
#include "openflow/private/vconn.h"
#include "openflow/private/xtoxll.h"

#include "openflow/private/chain.h"
#include "openflow/private/table.h"
#include "openflow/private/datapath.h" // The functions below are defined in datapath.c
uint32_t save_buffer (ofpbuf *);
ofpbuf * retrieve_buffer (uint32_t id);
void discard_buffer (uint32_t id);
#include "openflow/private/dp_act.h" // The functions below are defined in dp_act.c
void set_vlan_vid (ofpbuf *buffer, sw_flow_key *key, const ofp_action_header *ah);
void set_vlan_pcp (ofpbuf *buffer, sw_flow_key *key, const ofp_action_header *ah);
void strip_vlan (ofpbuf *buffer, sw_flow_key *key, const ofp_action_header *ah);
void set_dl_addr (ofpbuf *buffer, sw_flow_key *key, const ofp_action_header *ah);
void set_nw_addr (ofpbuf *buffer, sw_flow_key *key, const ofp_action_header *ah);
void set_tp_port (ofpbuf *buffer, sw_flow_key *key, const ofp_action_header *ah);
void set_mpls_label (ofpbuf *buffer, sw_flow_key *key, const ofp_action_header *ah);
void set_mpls_exp (ofpbuf *buffer, sw_flow_key *key, const ofp_action_header *ah);
#include "openflow/private/pt_act.h" // The function below is defined in pt_act.c
void update_checksums (ofpbuf *buffer, const sw_flow_key *key, uint32_t old_word, uint32_t new_word);

#undef list
#undef private
#undef delete
}

int main()
{
  return 0;
}
