/*************************************************************************
*********************** H E A D E R S  ***********************************
*************************************************************************/

#define CONST_MAX_LABELS 	255
#define CONST_MAX_SR_HOPS 8
// might be wrong, feels wrong at least should be << 19, change after it all works
#define SR_ADJ_SEGMENT_MASK 1<<18 

const bit<16> TYPE_IPV4 = 0x800;
const bit<16> TYPE_SR   = 0x8847;

typedef bit<9>  egressSpec_t;
typedef bit<48> macAddr_t;
typedef bit<32> ip4Addr_t;
typedef bit<20> label_t;

header ethernet_t {
    macAddr_t dstAddr;
    macAddr_t srcAddr;
    bit<16>   etherType;
}

header sr_t {
    bit<20>   label;
    bit<3>    exp;
    bit<1>    s;
    bit<8>    ttl;
}

header ipv4_t {
    bit<4>    version;
    bit<4>    ihl;
    bit<8>    diffserv;
    bit<16>   totalLen;
    bit<16>   identification;
    bit<3>    flags;
    bit<13>   fragOffset;
    bit<8>    ttl;
    bit<8>    protocol;
    bit<16>   hdrChecksum;
    ip4Addr_t srcAddr;
    ip4Addr_t dstAddr;
}

header tcp_t{
    bit<16> srcPort;
    bit<16> dstPort;
    bit<32> seqNo;
    bit<32> ackNo;
    bit<4>  dataOffset;
    bit<4>  res;
    bit<1>  cwr;
    bit<1>  ece;
    bit<1>  urg;
    bit<1>  ack;
    bit<1>  psh;
    bit<1>  rst;
    bit<1>  syn;
    bit<1>  fin;
    bit<16> window;
    bit<16> checksum;
    bit<16> urgentPtr;
}

struct metadata {
    bit<20> sr_id;
    
    bit<20> current_seg;
    bit<20> sr_dest;
    bit<14> packet_hash;
    bit<14> meta_path_differentiator;
    bit<14> sr_group_id;
    bit<14> ecmp_group_id;

    bit<13> flowlet_register_index;
    bit<16> flowlet_id;
    bit<13> flowlet_timeout_index;
    bit<48> flowlet_timeout_value;
    bit<48> flowlet_last_stamp;
    bit<48> flowlet_time_diff;
}

struct headers {
    ethernet_t                      ethernet;
    sr_t[CONST_MAX_SR_HOPS]         sr;
    ipv4_t                          ipv4;
    tcp_t                           tcp;
}