#ifndef _LZMA_FILE_
#define _LZMA_FILE_

#include <stdint.h>
#include <stdlib.h>
#include <lzma.h>

#define kBufferSize (1 << 15)

typedef struct lzma_file {
	uint8_t buf[kBufferSize];
	lzma_stream strm;
	int fd;
	bool encoding;
	bool eof;
} lzma_FILE;

lzma_FILE *lzma_open(lzma_ret *lzma_error, lzma_filter *filters, int fd, uint64_t memlimit);

int lzma_close(lzma_ret *lzma_error, lzma_FILE *lzma_file);

ssize_t lzma_read(lzma_ret *lzma_error, lzma_FILE *lzma_file, void *buf, size_t len);

ssize_t lzma_write(lzma_ret *lzma_error, lzma_FILE *lzma_file, void *buf, size_t len);

#endif
