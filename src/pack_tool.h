#ifndef PACK_TOOL_H
#define PACK_TOOL_H

unsigned int
pack(unsigned char *buf, char *format, ...);

void
unpack(unsigned char *buf, char *format, ...);

#endif