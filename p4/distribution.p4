/* -*- P4_16 -*- */

#include <core.p4>
#include <v1model.p4> 

//My includes
#include "include/headers.p4"
#include "include/parsers.p4"

#define REGISTER_SIZE 8192
#define TIMESTAMP_WIDTH 48
#define ID_WIDTH 16
#define FLOWLET_TIMEOUT 48w200000

typedef bit<TIMESTAMP_WIDTH> timestamp_t;

/*************************************************************************
************   C H E C K S U M    V E R I F I C A T I O N   *************
*************************************************************************/

control MyVerifyChecksum(inout headers hdr, inout metadata meta) {
    apply {  }
}

/*************************************************************************
**************  I N G R E S S   P R O C E S S I N G   *******************
*************************************************************************/

control MyIngress(inout headers hdr,
                  inout metadata meta,
                  inout standard_metadata_t standard_metadata) {

    action drop() {
        mark_to_drop(standard_metadata);
    }

    register<bit<20>>(1) sr_id_register;
    register<bit<ID_WIDTH>>(REGISTER_SIZE) flowlet_to_id;
    register<bit<TIMESTAMP_WIDTH>>(REGISTER_SIZE) flowlet_time_stamp;
    register<bit<TIMESTAMP_WIDTH>>(REGISTER_SIZE) flowlet_timeout;

    action read_flowlet_registers(){
        //compute register index
        hash(meta.flowlet_register_index, HashAlgorithm.crc16,
            (bit<16>)0,
            { hdr.ipv4.srcAddr, hdr.ipv4.dstAddr, hdr.tcp.srcPort, hdr.tcp.dstPort,hdr.ipv4.protocol},
            (bit<14>)REGISTER_SIZE);
        // Read previous time stamp
        flowlet_time_stamp.read(meta.flowlet_last_stamp, (bit<32>)meta.flowlet_register_index);
        // Read previous flowlet id
        flowlet_to_id.read(meta.flowlet_id, (bit<32>)meta.flowlet_register_index);
        // Update timestamp
        flowlet_time_stamp.write((bit<32>)meta.flowlet_register_index, standard_metadata.ingress_global_timestamp);

        // Get the timeout value for this particular flowlet 
        flowlet_timeout.read(meta.flowlet_timeout_value, (bit<32>)meta.flowlet_register_index);
    }

    action update_flowlet_id(){
        bit<32> random_t;
        random(random_t, (bit<32>)0, (bit<32>)65000);
        meta.flowlet_id = (bit<16>)random_t;
        flowlet_to_id.write((bit<32>)meta.flowlet_register_index, (bit<16>)meta.flowlet_id);
    }


    action ipv4_forward(macAddr_t dstAddr, egressSpec_t port) {
        hdr.ethernet.srcAddr = hdr.ethernet.dstAddr;
        hdr.ethernet.dstAddr = dstAddr;

        standard_metadata.egress_spec = port;
        hdr.ipv4.ttl = hdr.ipv4.ttl - 1;
    }

    // DST @ -> CLASSIFICATION OF FLOWLETS//////////////////////////////////
    action set_sr_group(bit<14> sr_group_id, bit<16> num_shops){
        hash(meta.packet_hash,
	    HashAlgorithm.crc16,
	    (bit<1>)0,
	    { hdr.ipv4.srcAddr,
	      hdr.ipv4.dstAddr,
          hdr.tcp.srcPort,
          hdr.tcp.dstPort,
          hdr.ipv4.protocol,
          meta.flowlet_id},
	    num_shops);

	    meta.sr_group_id = sr_group_id;
    }

    table ipv4_lpm {
        key = {
            hdr.ipv4.dstAddr: lpm;
        }
        actions = {
            ipv4_forward;
            set_sr_group;
            NoAction;
        }
        size = 1024;
        default_action = NoAction();
    }


    // GROUPS OF FLOWLETS -> LISTS OF SEGMENTS ////////////////////////////// 
    action sr_ingress_1_hop(label_t label_1, timestamp_t flowlet_timeout_value) {

        hdr.ethernet.etherType = TYPE_SR;

        hdr.sr.push_front(1);
        hdr.sr[0].setValid();
        hdr.sr[0].label = label_1;
        hdr.sr[0].ttl = hdr.ipv4.ttl - 1;
        hdr.sr[0].s = 1;

        flowlet_timeout.write((bit<32>)meta.flowlet_register_index, flowlet_timeout_value);
    }

    action sr_ingress_2_hop(label_t label_1, label_t label_2, timestamp_t flowlet_timeout_value) {

        hdr.ethernet.etherType = TYPE_SR;

        hdr.sr.push_front(1);
        hdr.sr[0].setValid();
        hdr.sr[0].label = label_1;
        hdr.sr[0].ttl = hdr.ipv4.ttl - 1;
        hdr.sr[0].s = 1;

        hdr.sr.push_front(1);
        hdr.sr[0].setValid();
        hdr.sr[0].label = label_2;
        hdr.sr[0].ttl = hdr.ipv4.ttl - 1;
        hdr.sr[0].s = 0;

        flowlet_timeout.write((bit<32>)meta.flowlet_register_index, flowlet_timeout_value);
    }

    action sr_ingress_3_hop(label_t label_1, label_t label_2, label_t label_3, timestamp_t flowlet_timeout_value) {

        hdr.ethernet.etherType = TYPE_SR;

        hdr.sr.push_front(1);
        hdr.sr[0].setValid();
        hdr.sr[0].label = label_1;
        hdr.sr[0].ttl = hdr.ipv4.ttl - 1;
        hdr.sr[0].s = 1;

        hdr.sr.push_front(1);
        hdr.sr[0].setValid();
        hdr.sr[0].label = label_2;
        hdr.sr[0].ttl = hdr.ipv4.ttl - 1;
        hdr.sr[0].s = 0;

        hdr.sr.push_front(1);
        hdr.sr[0].setValid();
        hdr.sr[0].label = label_3;
        hdr.sr[0].ttl = hdr.ipv4.ttl - 1;
        hdr.sr[0].s = 0;

        flowlet_timeout.write((bit<32>)meta.flowlet_register_index, flowlet_timeout_value);
    }

    action sr_ingress_4_hop(label_t label_1, label_t label_2, label_t label_3, label_t label_4, timestamp_t flowlet_timeout_value) {

        hdr.ethernet.etherType = TYPE_SR;

        hdr.sr.push_front(1);
        hdr.sr[0].setValid();
        hdr.sr[0].label = label_1;
        hdr.sr[0].ttl = hdr.ipv4.ttl - 1;
        hdr.sr[0].s = 1;

        hdr.sr.push_front(1);
        hdr.sr[0].setValid();
        hdr.sr[0].label = label_2;
        hdr.sr[0].ttl = hdr.ipv4.ttl - 1;
        hdr.sr[0].s = 0;

        hdr.sr.push_front(1);
        hdr.sr[0].setValid();
        hdr.sr[0].label = label_3;
        hdr.sr[0].ttl = hdr.ipv4.ttl - 1;
        hdr.sr[0].s = 0;

        hdr.sr.push_front(1);
        hdr.sr[0].setValid();
        hdr.sr[0].label = label_4;
        hdr.sr[0].ttl = hdr.ipv4.ttl - 1;
        hdr.sr[0].s = 0;

        flowlet_timeout.write((bit<32>)meta.flowlet_register_index, flowlet_timeout_value);
    }

    action sr_ingress_5_hop(label_t label_1, label_t label_2, label_t label_3, label_t label_4, label_t label_5, timestamp_t flowlet_timeout_value) {
        hdr.ethernet.etherType = TYPE_SR;

        hdr.sr.push_front(1);
        hdr.sr[0].setValid();
        hdr.sr[0].label = label_1;
        hdr.sr[0].ttl = hdr.ipv4.ttl - 1;
        hdr.sr[0].s = 1;

        hdr.sr.push_front(1);
        hdr.sr[0].setValid();
        hdr.sr[0].label = label_2;
        hdr.sr[0].ttl = hdr.ipv4.ttl - 1;
        hdr.sr[0].s = 0;

        hdr.sr.push_front(1);
        hdr.sr[0].setValid();
        hdr.sr[0].label = label_3;
        hdr.sr[0].ttl = hdr.ipv4.ttl - 1;
        hdr.sr[0].s = 0;

        hdr.sr.push_front(1);
        hdr.sr[0].setValid();
        hdr.sr[0].label = label_4;
        hdr.sr[0].ttl = hdr.ipv4.ttl - 1;
        hdr.sr[0].s = 0;

        hdr.sr.push_front(1);
        hdr.sr[0].setValid();
        hdr.sr[0].label = label_5;
        hdr.sr[0].ttl = hdr.ipv4.ttl - 1;
        hdr.sr[0].s = 0;

        flowlet_timeout.write((bit<32>)meta.flowlet_register_index, flowlet_timeout_value);
    }

    table FEC_tbl {
        key = {
            meta.sr_group_id: exact;
            meta.packet_hash: exact;
        }
        actions = {
            sr_ingress_1_hop;
            sr_ingress_2_hop;
            sr_ingress_3_hop;
            sr_ingress_4_hop;
            sr_ingress_5_hop;
            NoAction;
        }
        default_action = NoAction();
        size = 256;
    }


    // LIST OF SEGMENTS -> FORWARDING //////////////////////////////
    action sr_forward(macAddr_t dstAddr, egressSpec_t port) {
        hdr.ethernet.srcAddr = hdr.ethernet.dstAddr;
        hdr.ethernet.dstAddr = dstAddr;

        standard_metadata.egress_spec = port;
        hdr.sr[1].ttl = hdr.sr[0].ttl - 1;
    }

    action set_ecmp_group(bit<14> ecmp_group_id, bit<16> num_nhops){
        hash(meta.packet_hash,
	    HashAlgorithm.crc16,
	    (bit<1>)0,
	    { hdr.ipv4.srcAddr,
	      hdr.ipv4.dstAddr,
          hdr.tcp.srcPort,
          hdr.tcp.dstPort,
          hdr.ipv4.protocol,
          meta.flowlet_id},
	    num_nhops);

	    meta.ecmp_group_id = ecmp_group_id;
    }

    table sr_tbl {
        key = {
            hdr.sr[0].label: exact;
        }
        actions = {
            sr_forward;
            set_ecmp_group;
            ipv4_forward;
            NoAction;
        }
        default_action = NoAction();
        size = CONST_MAX_LABELS;
    }

    table ecmp_group_to_nhop {
        key = {
            meta.ecmp_group_id:    exact;
            meta.packet_hash: exact;
        }
        actions = {
            drop;
            sr_forward;
        }
        size = 1024;
    }

    apply {
        // Pop segment if the segment represents the current switch 
        if(hdr.sr[0].isValid()){
            sr_id_register.read(meta.sr_id, 0);
            if(hdr.sr[0].label == meta.sr_id)
            {
                if(hdr.sr[0].s == 1)
                {
                    hdr.ethernet.etherType = TYPE_IPV4;
                }
                hdr.sr.pop_front(1);
            }
        }

        // Ingress router stuff
        if (hdr.ipv4.isValid() && !(hdr.sr[0].isValid())){
            @atomic {
                read_flowlet_registers();
                meta.flowlet_time_diff = standard_metadata.ingress_global_timestamp - meta.flowlet_last_stamp;

                //check if inter-packet gap is > timeout value
                if (meta.flowlet_time_diff > meta.flowlet_timeout_value){
                    update_flowlet_id();
                }
            }
            switch (ipv4_lpm.apply().action_run){
                set_sr_group: {
                    FEC_tbl.apply();
                }
            }
        }

        // SR-Forwarding
        if(hdr.sr[0].isValid()){
            switch (sr_tbl.apply().action_run){
                set_ecmp_group: {
                    ecmp_group_to_nhop.apply();
                }
            }

            // Pop adjacency segment
            if((hdr.ethernet.etherType == TYPE_SR) && (hdr.sr[0].label & SR_ADJ_SEGMENT_MASK) != 0)
            {
                if(hdr.sr[0].s == 1)
                {
                    hdr.ethernet.etherType = TYPE_IPV4;
                }
                hdr.sr.pop_front(1);
            }
        }
    }
}

/*************************************************************************
****************  E G R E S S   P R O C E S S I N G   *******************
*************************************************************************/

control MyEgress(inout headers hdr,
                 inout metadata meta,
                 inout standard_metadata_t standard_metadata) {

    apply {
    }
}

/*************************************************************************
*************   C H E C K S U M    C O M P U T A T I O N   **************
*************************************************************************/

control MyComputeChecksum(inout headers  hdr, inout metadata meta) {
    apply {

        // We have modified the ip ttl, so we have to compute the new checksum
        update_checksum(
                hdr.ipv4.isValid(),
                { hdr.ipv4.version,
                hdr.ipv4.ihl,
                hdr.ipv4.diffserv,
                hdr.ipv4.totalLen,
                hdr.ipv4.identification,
                hdr.ipv4.flags,
                hdr.ipv4.fragOffset,
                hdr.ipv4.ttl,
                hdr.ipv4.protocol,
                hdr.ipv4.srcAddr,
                hdr.ipv4.dstAddr },
                hdr.ipv4.hdrChecksum,
                HashAlgorithm.csum16);
    }
}

/*************************************************************************
***********************  S W I T C H  *******************************
*************************************************************************/

V1Switch(
MyParser(),
MyVerifyChecksum(),
MyIngress(),
MyEgress(),
MyComputeChecksum(),
MyDeparser()
) main;
