/*
 * CS3600, Spring 2013
 * Project 4 Starter Code
 * (c) 2013 Alan Mislove
 *
 */

#ifndef __3600SENDRECV_H__
#define __3600SENDRECV_H__

#include <stdio.h>
#include <stdarg.h>

#define WINDOW_SIZE 40
#define SENDER_TIMEOUT_SEC 0
#define SENDER_TIMEOUT_MICRO 65000

typedef struct header_t {
  unsigned int magic:14;
  unsigned int ack:1;
  unsigned int eof:1;
  unsigned short length;
  unsigned short sequence;
  unsigned short checksum;
} header;

unsigned int MAGIC;

unsigned short get_checksum(unsigned short *buf);
void dump_packet(unsigned char *data, int size);
header *make_header(short sequence, int length, int eof, int ack);
header *get_header(void *data);
char *get_data(void *data);
char *timestamp();
void mylog(char *fmt, ...);

#endif

