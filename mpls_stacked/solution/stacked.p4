/* -*- P4_16 -*- */

#include <core.p4>
#include <v1model.p4>

//My includes
#include "include/headers.p4"
#include "include/parsers.p4"

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


    // DST @ -> CLASSIFICATION OF FLOWS //////////////////////////////////
    action ipv4_forward(macAddr_t dstAddr, egressSpec_t port) {

        hdr.ethernet.srcAddr = hdr.ethernet.dstAddr;
        hdr.ethernet.dstAddr = dstAddr;

        standard_metadata.egress_spec = port;
        hdr.ipv4.ttl = hdr.ipv4.ttl - 1;
    }

    action sr_group(bit<14> sr_group_id, bit<16> num_nhops){
        hash(meta.packet_hash,
	    HashAlgorithm.crc16,
	    (bit<1>)0,
	    { hdr.ipv4.srcAddr,
	      hdr.ipv4.dstAddr,
          hdr.tcp.srcPort,
          hdr.tcp.dstPort,
          hdr.ipv4.protocol},
	    num_nhops);

	    meta.sr_group_id = sr_group_id;
    }

    table ipv4_lpm {
        key = {
            hdr.ipv4.dstAddr: lpm;
        }
        actions = {
            ipv4_forward;
            sr_group;
            NoAction;
        }
        size = 1024;
        default_action = NoAction();
    }


    // GROUPS OF FLOWS -> LISTS OF SEGMENTS //////////////////////////////
    action sr_ingress_1_hop(label_t label_1) {

        hdr.ethernet.etherType = TYPE_SR;

        hdr.sr.push_front(1);
        hdr.sr[0].setValid();
        hdr.sr[0].label = label_1;
        hdr.sr[0].ttl = hdr.ipv4.ttl - 1;
        hdr.sr[0].s = 1;
    }

    action sr_ingress_2_hop(label_t label_1, label_t label_2) {

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
    }

    action sr_ingress_3_hop(label_t label_1, label_t label_2, label_t label_3) {

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
    }

    action sr_ingress_4_hop(label_t label_1, label_t label_2, label_t label_3, label_t label_4) {

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
    }

    action sr_ingress_5_hop(label_t label_1, label_t label_2, label_t label_3, label_t label_4, label_t label_5) {

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

        if(1 == 1)
        {
            hdr.sr.pop_front(1);
        }
    }

    action penultimate(macAddr_t dstAddr, egressSpec_t port) {

        hdr.ethernet.etherType = TYPE_IPV4;

        hdr.ethernet.srcAddr = hdr.ethernet.dstAddr;
        hdr.ethernet.dstAddr = dstAddr;

        hdr.ipv4.ttl = hdr.sr[0].ttl - 1;

        standard_metadata.egress_spec = port;
        hdr.sr.pop_front(1);
    }

    table sr_tbl {
        key = {
            hdr.sr[0].label: exact;
            hdr.sr[0].s: exact;
        }
        actions = {
            sr_forward;
            penultimate;
            NoAction;
        }
        default_action = NoAction();
        size = CONST_MAX_LABELS;
    }

    apply {
        // Ingress router
        if (hdr.ipv4.isValid()){
            switch (ipv4_lpm.apply().action_run){
                sr_group: {
                    FEC_tbl.apply();
                }
            }
        }

        // SR-Forwarding
        if(hdr.sr[0].isValid()){
            sr_tbl.apply();
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
