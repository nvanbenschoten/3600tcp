/*
 * CS3600, Spring 2013
 * Project 4 Starter Code
 * (c) 2013 Alan Mislove
 *
 */

#include <math.h>
#include <ctype.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "3600sendrecv.h"

static int DATA_SIZE = 1460;

unsigned short p_created = 1;

struct timeval start_time;
struct timeval cur_time;
unsigned int elapsed_time = 0;
unsigned int timeout_sec = SENDER_TIMEOUT_SEC;
unsigned int timeout_usec = SENDER_TIMEOUT_MICRO;

void usage() {
    printf("Usage: 3600send host:port\n");
    exit(1);
}

/**
 * Reads the next block of data from stdin
 */
int get_next_data(char *data, int size) {
    return read(0, data, size);
}

/**
 * Builds and returns the next packet, or NULL
 * if no more data is available.
 */
void *get_next_packet(int sequence, int *len, unsigned int time) {
    char *data = malloc(DATA_SIZE);
    int data_len = get_next_data(data, DATA_SIZE);

    if (data_len == 0) {
        free(data);
        return NULL;
    }

    header *myheader = make_header((short)sequence, data_len, 0, 0, time);
    void *packet = malloc(sizeof(header) + sizeof(char) + data_len);
    memcpy(packet, myheader, sizeof(header));
    
    unsigned char *checksum = malloc(sizeof(char));
    *checksum = get_checksum((char *)myheader, data, data_len);
    memcpy(((char *) packet) +sizeof(header), (char *)checksum, sizeof(unsigned char));
   
    memcpy(((char *) packet) +sizeof(header)+sizeof(char), data, data_len);

    free(data);
    free(checksum);
    free(myheader);

    *len = sizeof(header) + sizeof(char) + data_len;

    return packet;
}

void *get_final_packet() {
    gettimeofday(&cur_time, NULL);
    elapsed_time = (unsigned int)(cur_time.tv_sec*1000000 + cur_time.tv_usec)-(start_time.tv_sec*100000+start_time.tv_usec);
    header *myheader = make_header(p_created, 0, 1, 0, elapsed_time);

    void *packet = malloc(sizeof(header) + sizeof(char));
    memcpy(packet, myheader, sizeof(header));

    unsigned char *checksum = malloc(sizeof(char));
    *checksum = get_checksum((char *)myheader, NULL, 0);
    memcpy(((char *) packet) +sizeof(header), (char *)checksum, sizeof(unsigned char));

    free(myheader);
    free(checksum);

    return packet;
}

void update_timeouts(unsigned int time) {
    time = ntohl(time);
    gettimeofday(&cur_time, NULL);
    elapsed_time = (unsigned int)(cur_time.tv_sec*1000000 + cur_time.tv_usec)-(start_time.tv_sec*100000+start_time.tv_usec);
    unsigned int rtt = elapsed_time - time;
    timeout_sec = timeout_sec*RTT_DECAY + (1-RTT_DECAY)*(rtt)/1000000;
    timeout_usec =  timeout_usec*RTT_DECAY + ((unsigned int)((1.0-RTT_DECAY)*(rtt)))%1000000;
    mylog("time: %u elapsed_time: %u timeout_sec: %u timeout_usec: %u rtt: %u\n", time, elapsed_time, timeout_sec, timeout_usec, rtt);
    //mylog("Set timeout_usec to %u\n", timeout_usec);
}

/*int send_next_packet(int sock, struct sockaddr_in out) {
    int packet_len = 0;
    void *packet = get_next_packet(sequence, &packet_len);

    if (packet == NULL) 
        return 0;

    mylog("[send data] %d (%d)\n", sequence, packet_len - sizeof(header));

    if (sendto(sock, packet, packet_len, 0, (struct sockaddr *) &out, (socklen_t) sizeof(out)) < 0) {
        perror("sendto");
        exit(1);
    }

    return 1;
}*/

/*void send_final_packet(int sock, struct sockaddr_in out) {
    header *myheader = make_header(sequence+1, 0, 1, 0);
    mylog("[send eof]\n");

    if (sendto(sock, myheader, sizeof(header), 0, (struct sockaddr *) &out, (socklen_t) sizeof(out)) < 0) {
        perror("sendto"nt received;
        );
        exit(1);
    }
}*/

int main(int argc, char *argv[]) {

    // extract the host IP and port
    if ((argc != 2) || (strstr(argv[1], ":") == NULL)) {
        usage();
    }

    char *tmp = (char *) malloc(strlen(argv[1])+1);
    strcpy(tmp, argv[1]);

    char *ip_s = strtok(tmp, ":");
    char *port_s = strtok(NULL, ":");

    // first, open a UDP socket  
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    // next, construct the local port
    struct sockaddr_in out;
    out.sin_family = AF_INET;
    out.sin_port = htons(atoi(port_s));
    out.sin_addr.s_addr = inet_addr(ip_s);

    // socket for received packets
    struct sockaddr_in in;
    socklen_t in_len = sizeof(in);

    // construct the socket set
    fd_set socks;

    // construct the timeout
    struct timeval t;
    t.tv_sec = SENDER_TIMEOUT_SEC;
    t.tv_usec = SENDER_TIMEOUT_MICRO; // 10 ms

    // packet tracking vars
    unsigned short p_ack = 0;
    //unsigned short p_created = 1;
    //int p_sent = 0;
    //void *packet;
    //int p_off = 0;
    void * packets[WINDOW_SIZE] = {0};
    int p_len[WINDOW_SIZE] = {0};
    int more_packets = 1;
    int i = 0;
    int repeated_acks = 0;
    int retransmitted = 0;
    //

    // allocate memory buffers for packets
    for (i = 0; i < WINDOW_SIZE; i++) {
        packets[i] = calloc(1, 1500);
    }
    
    //time_t ltime;
    //ltime = time(NULL);
    //struct tm *tm;
    //tm = localtime(&ltime);
    gettimeofday(&start_time, NULL);
    gettimeofday(&cur_time, NULL);
    mylog("start_time.tv_sec: %d cur_time.tv_sec: %d\n", start_time.tv_sec, cur_time.tv_sec);
    //`
    int starting = 1;
    int cur_window = 1;
    //float window_size_exp = 0;
    int backoff = 1;
    int round_acked = 0;

    while (1) {
        /*if (starting) { // if we are still exponentially increasing the window size
            cur_window = (int) pow(2.0, window_size_exp);
            mylog("Increased cur_window to %d\n", cur_window);
            window_size_exp++;
        }*/

        // get the next packets to send if necessary in window
        while (more_packets && p_created-p_ack <= cur_window) {
            // get a time for our new packet
            gettimeofday(&cur_time, NULL);
            elapsed_time = (unsigned int)(cur_time.tv_sec*1000000 + cur_time.tv_usec)-(start_time.tv_sec*100000+start_time.tv_usec);
            // create the packet and add it to the array
            packets[p_created%WINDOW_SIZE] = get_next_packet(p_created, &(p_len[p_created%WINDOW_SIZE]), elapsed_time); // 
            //memcpy(&(packets[p_created%10]), get_next_packet(p_created, &(p_len[p_created%10])), p_len[p_created%10]);
             
            if (packets[p_created%WINDOW_SIZE] == NULL) { 
                //mylog("no more data that needs packets\n");
                //p_created--;
                more_packets = 0;
            }
            //else {
                //p_off++;
            //}
            else {
                p_created++;
                //mylog("p_created: %d\n", p_created);
            }
        }

        // send non-acked packets in window
        for (i = p_ack+1; i < p_created && i <= p_ack+cur_window; i++) {
            mylog("i: %d p_created: %d cur_window: %d\n", i, p_created, cur_window);
            if (sendto(sock, packets[i%WINDOW_SIZE], p_len[i%WINDOW_SIZE], 0, (struct sockaddr *) &out, (socklen_t) sizeof(out)) < 0) {
                perror("sendto");
                exit(1);
            }
            mylog("[send data] %d (%d)\n", i, p_len[i%WINDOW_SIZE] - sizeof(header)); // 
            if (retransmitted) {
                break;
            }
        }
        
        
        // check for received packets
        FD_ZERO(&socks);
        FD_SET(sock, &socks);
        t.tv_sec = timeout_sec * RTT_MULT;
        t.tv_usec = timeout_usec * RTT_MULT;

        // wait to receive, or for a timeout
        // loop while still receiving packets/data every 10 ms
        while (select(sock + 1, &socks, NULL, NULL, &t)) {
            unsigned char buf[10000];
            int buf_len = sizeof(buf);
            int received;
            if ((received = recvfrom(sock, &buf, buf_len, 0, (struct sockaddr *) &in, (socklen_t *) &in_len)) < 0) {
                perror("recvfrom");
                exit(1);
            }

            header *myheader = get_header(buf);

            if (retransmitted && myheader->sequence == p_ack) { // if we just retransmitted and are still dealing with a packet backlog
                // update timeout
                update_timeouts(myheader->time);
                // go to process next parcket and ignore this one
                continue;
            }

            if ((myheader->magic == MAGIC) && (myheader->sequence >= p_ack) && (myheader->ack == 1)) {
                mylog("[recv ack] %d\n", myheader->sequence);

                if (p_ack == myheader->sequence) { // if we received a repeated ack
                    repeated_acks++;
                }
                else { // otherwise reset the repeated_ack count and mark retransmission as unnecessary
                    if (starting) {
                        cur_window++;
                        mylog("Increased cur_window to %d\n", cur_window);
                        //round_acked++;
                    }
                    else {
                        round_acked++;
                    }
                    retransmitted = 0;
                    repeated_acks = 0;
                }

                if (repeated_acks >= DUPLICATE_ACKS) { // if we need to fast retransmit
                    //p_ack = myheader->sequence; // TODO: see if necessary
                    retransmitted = 1;
                    repeated_acks = 0;
                    starting = 0;
                    if (cur_window != 1) {
                        //cur_window = cur_window*backoff/(backoff+1); // TODO: maybe need + 1
                    }
                    backoff++;
                    update_timeouts(myheader->time);
                    break;
                }

                p_ack = myheader->sequence;
                // update timeout values estimates based on RTT
                update_timeouts(myheader->time);
                if (p_ack+1 == p_created) {
                    break;
                }
                //done = 1;
            } else {
                // update timeout values estimates based on RTT
                // TODO: determine this helps or hurts estimate
                update_timeouts(myheader->time);
                // log old ACK
                mylog("[recv corrupted ack] %x %d\n", MAGIC, p_created);
            }

            //FD_ZERO(&socks);
            //FD_SET(sock, &socks);
        } /*else {
            mylog("[error] timeout occurred\n");
        }*/
        // increase window for congestion avoidence if necessary
        cur_window += (int) (CWD_SCALE_FACTOR*(float)round_acked/(float)cur_window);
        round_acked = 0;

        if (t.tv_sec <= 0 && t.tv_usec <= 0) { // if we timed out waiting for packets
            starting = 0;
            if (cur_window != 1) {
                cur_window = cur_window*backoff/(backoff+1);
            }
            //timeout_usec = timeout_usec*2;
            //timeout_sec =  (1-RTT_DECAY)*(rtt)/1000000;
            //timeout_usec =  ((unsigned int)((1.0-RTT_DECAY)*(rtt)))%1000000;
            timeout_sec = (timeout_usec*2)/1000000;
            timeout_usec = (timeout_usec*2)%1000000;
            mylog("timeout: backing off, backoff = %d cur_window = %d\n", backoff, cur_window);
            //backoff++;
        } //else { // otherwise we completed a round successfully so we incread window slightly
            
        //}

        if (!more_packets && p_ack+1 == p_created) {
            //mylog("done, all packets acked\n");
            break;
        }
    }

    //send_final_packet(sock, out);
    char *packet = get_final_packet();
    mylog("[send eof]\n");

    if (sendto(sock, packet, sizeof(header)+sizeof(char), 0, (struct sockaddr *) &out, (socklen_t) sizeof(out)) < 0) {
        perror("sendto");
        exit(1);
    }
    if (sendto(sock, packet, sizeof(header)+sizeof(char), 0, (struct sockaddr *) &out, (socklen_t) sizeof(out)) < 0) {
        perror("sendto");
        exit(1);
    }
    if (sendto(sock, packet, sizeof(header)+sizeof(char), 0, (struct sockaddr *) &out, (socklen_t) sizeof(out)) < 0) {
        perror("sendto");
        exit(1);
    }

    free(packet);

    mylog("[completed]\n");
    return 0;
}
