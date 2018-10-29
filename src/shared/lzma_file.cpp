#include "lzma_file.h"

#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>

lzma_FILE *lzma_open(lzma_ret *lzma_error, lzma_filter *filters, int fd, uint64_t memlimit)
{
	lzma_ret *ret = lzma_error;
	bool encoding = filters[0].options ? true : false;
	lzma_FILE *lzma_file;
    
	lzma_file = (lzma_FILE*)calloc(1, sizeof(*lzma_file));

	if (!lzma_file) {
		close(fd);
		return NULL;
	}

	lzma_file->fd = fd;
	lzma_file->encoding = encoding;
	lzma_file->eof = false;
	lzma_stream tmp = LZMA_STREAM_INIT;
	lzma_file->strm = tmp;

	if (encoding) {
		if(filters[0].id == LZMA_FILTER_LZMA1)
			*ret = lzma_alone_encoder(&lzma_file->strm, (const lzma_options_lzma*)filters[0].options);
		else
			*ret = lzma_stream_encoder(&lzma_file->strm, filters, (lzma_check)filters[LZMA_FILTERS_MAX + 1].id);
	} else
		*ret = lzma_auto_decoder(&lzma_file->strm, memlimit, 0);

	if (*ret != LZMA_OK) {
		close(fd);
		memset(lzma_file, 0, sizeof(*lzma_file));
		free(lzma_file);
		return NULL;
	}
	return lzma_file;
}

static int lzma_close_real(lzma_ret *lzma_error, lzma_FILE *lzma_file)
{
	lzma_ret *ret = lzma_error;	
	int retval = 0;
	ssize_t n;

	if (!lzma_file)
		return -1;
	if (lzma_file->encoding) {
		for (;;) {
			lzma_file->strm.avail_out = kBufferSize;
			lzma_file->strm.next_out = (uint8_t *)lzma_file->buf;
			*ret = lzma_code(&lzma_file->strm, LZMA_FINISH);
			if (*ret != LZMA_OK && *ret != LZMA_STREAM_END)
			{
				retval = -1;
				break;
			}
			n = kBufferSize - lzma_file->strm.avail_out;
			if (n && write(lzma_file->fd, lzma_file->buf, n) != n)
			{
				retval = -1;
				break;
			}
			if (*ret == LZMA_STREAM_END)
				break;
		}
	} else
		*ret = LZMA_OK;

	lzma_end(&lzma_file->strm);
	return retval;
}

int lzma_close(lzma_ret *lzma_error, lzma_FILE *lzma_file)
{
	int rc;
	rc = lzma_close_real(lzma_error, lzma_file);
	if(rc)
		return rc;
	rc = close(lzma_file->fd);
	return rc;
}

ssize_t lzma_read(lzma_ret *lzma_error, lzma_FILE *lzma_file, void *buf, size_t len)
{
	lzma_ret *ret = lzma_error;
	bool eof = false;
    
	if (!lzma_file || lzma_file->encoding)
		return -1;
	if (lzma_file->eof)
		return 0;

	lzma_file->strm.next_out = (uint8_t*)buf;
	lzma_file->strm.avail_out = len;
	for (;;) {
		if (!lzma_file->strm.avail_in) {
			lzma_file->strm.next_in = (uint8_t *)lzma_file->buf;
			lzma_file->strm.avail_in = read(lzma_file->fd, lzma_file->buf, kBufferSize);
			if (!lzma_file->strm.avail_in)
				eof = true;
		}
		*ret = lzma_code(&lzma_file->strm, LZMA_RUN);
		if (*ret == LZMA_STREAM_END) {
			lzma_file->eof = true;
			return len - lzma_file->strm.avail_out;
		}
		if (*ret != LZMA_OK)
			return -1;
		if (!lzma_file->strm.avail_out)
			return len;
		if (eof)
			return -1;
	}
}

ssize_t lzma_write(lzma_ret *lzma_error, lzma_FILE *lzma_file, void *buf, size_t len)
{
	lzma_ret *ret = lzma_error;
	ssize_t n;

	if (!lzma_file || !lzma_file->encoding)
		return -1;
	if (!len)
		return 0;

	lzma_file->strm.next_in = (uint8_t*)buf;
	lzma_file->strm.avail_in = len;
	for (;;) {
		lzma_file->strm.next_out = (uint8_t *)lzma_file->buf;
		lzma_file->strm.avail_out = kBufferSize;
		*ret = lzma_code(&lzma_file->strm, LZMA_RUN);
		if (*ret != LZMA_OK)
			return -1;
		n = kBufferSize - lzma_file->strm.avail_out;
		if (n && write(lzma_file->fd, lzma_file->buf, n) != n)
			return -1;
		if (!lzma_file->strm.avail_in)
			return len;
	}
}
