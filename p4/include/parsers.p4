/*************************************************************************
*********************** P A R S E R  *******************************
*************************************************************************/

parser MyParser(packet_in packet,
                out headers hdr,
                inout metadata meta,
                inout standard_metadata_t standard_metadata) {

    state start {

        transition parse_ethernet;

    }

    state parse_ethernet {

        packet.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType){
            TYPE_SR: parse_sr;
            TYPE_IPV4: parse_ipv4;
            default: accept;
        }
    }

    state parse_sr {
        packet.extract(hdr.sr.next);
        transition select(hdr.sr.last.s) {
            1: parse_ipv4;
            default: parse_sr;
        }
    }

    state parse_ipv4 {
        packet.extract(hdr.ipv4);
        transition select(hdr.ipv4.protocol){
            6 : parse_tcp;
            default: accept;
        }
    }

    state parse_tcp {
        packet.extract(hdr.tcp);
        transition accept;
    }
}


/*************************************************************************
***********************  D E P A R S E R  *******************************
*************************************************************************/

control MyDeparser(packet_out packet, in headers hdr) {
    apply {

        //parsed headers have to be added again into the packet.
        packet.emit(hdr.ethernet);
        // to remove
        packet.emit(hdr.cpu);
        packet.emit(hdr.sr);
        packet.emit(hdr.ipv4);

        //Only emited if valid
        packet.emit(hdr.tcp);

    }
}