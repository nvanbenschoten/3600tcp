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

#define WINDOW_SIZE 50
#define SENDER_TIMEOUT_SEC 0
#define SENDER_TIMEOUT_MICRO 2000000

typedef struct header_t {
  unsigned int magic:14;
  unsigned int ack:1;
  unsigned int eof:1;
  unsigned short length;
  unsigned short sequence;
  unsigned char checksum;
} header;

unsigned int MAGIC;

unsigned char get_checksum(char *buf, char *data, int length);
void dump_packet(unsigned char *data, int size);
header *make_header(short sequence, int length, char *data, int eof, int ack);
header *get_header(void *data);
char *get_data(void *data);
char *timestamp();
void mylog(char *fmt, ...);

#endif

