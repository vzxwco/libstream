/*  Copyright (c) 2011, Philip Busch <vzxwco@gmail.com>
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *   - Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   - Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */


#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "list.h"
#include "stream.h"


#define CHUNKSIZE 4096


/* Creates a new stream object. */
stream *stream_create()
{
	return calloc(sizeof(stream), 1);
}


/* Writes the byte c to the stream s. */
int stream_write_byte(stream *s, char c)
{
	assert (s != NULL);

	char *chunk;

	// do we need a new chunk node?
	if (s->buffer == NULL || s->end == CHUNKSIZE) {
		chunk = malloc(CHUNKSIZE);
		if (chunk == NULL)
			return -1;

		if (list_append(&(s->buffer), chunk)) {
			free (chunk);
			return -1;
		}

		s->end = 0;
	}

	chunk = list_get_last(&(s->buffer));
	chunk[s->end++] = c;

	return 0;
}


/* Writes len bytes of data to stream s. */
int stream_write(stream *s, char *data, size_t len)
{
	size_t i;

	if (s == NULL)
		return -1;

	if (data == NULL)
		return -1;

	for (i = 0; i < len; i++)
		if (stream_write_byte(s, data[i]))
			return (i==0)?-1:(int)i;

	return 0;
}


/* Reads a byte from stream s and stores it in c. */
int stream_read_byte(stream *s, char *c)
{
	char *data;
	size_t size, end;
	node_l *n;

	assert(s != NULL);

	size = list_size(&(s->buffer));

	switch (size) {
		case 0:
			return -1;
		case 1:
			end = s->end;
			break;
		default:
			end = CHUNKSIZE;
			break;
	}

	data = list_get_first(&(s->buffer));
	*c = data[s->start++];

	if (s->start == end) {
		free(data);
		n = list_pop_first_node(&(s->buffer));
		free(n);

		s->start = 0;
		if (size == 1)
			s->end = 0;
	}

	return 0;
}


/* Reads up to len bytes from the first list node and stores the result in buf. */
static int stream_read_chunk(stream *s, char *buf, size_t len)
{
	size_t end, listsz, chunksz;
	char *data;
	node_l *n;

	assert(buf != NULL);

	listsz = list_size(&(s->buffer));

	switch (listsz) {
		case 0:
			return 0;
		case 1:
			end = s->end;
			break;
		default:
			end = CHUNKSIZE;
			break;
	}

	data = list_get_first(&(s->buffer));

	chunksz = end - s->start;

	if (len < chunksz) {
		memcpy(buf, data + s->start, len);
		s->start += len;
		return len;
	} else {
		memcpy(buf, data + s->start, chunksz);

		n = list_pop_first_node(&(s->buffer));
		free(data);
		free(n);

		s->start = 0;
		if (listsz == 1)
			s->end = 0;
		return chunksz;
	}
}


/* Reads up to len bytes of data from stream s and stores the result in buf. */
int stream_read(stream *s, char *buf, size_t len)
{
	size_t cnt, offset = 0;

	while (len) {
		cnt = stream_read_chunk(s, buf+offset, len);
		if (cnt == 0)
			return offset;

		offset += cnt;
		len -= cnt;
	}

	return offset;
}

/*
// Simple but slow. Left as an example of how not to do it.
int stream_read(stream *s, char *buf, size_t len)
{
	size_t i;
	char c;

	for (i = 0; i < len; i++) {
		if (stream_read_byte(s, &c) == 0)
			buf[i] = c;
		else
			return i;
	}

	return i;
}
*/


/* Reads up to len bytes of data from stream s, storing the result in buf.
   Does NOT modify stream position indicators or the buffer itself. */
int stream_peak(stream *s, char *buf, size_t len)
{
	node_l *n = s->buffer;
	char *data;
	size_t i = 0, offset = 0, chunksz, start, end;

	assert(buf != NULL);

	if (len == 0)
		return 0;

	if (s->buffer == NULL)
		return 0;

	do {
		data = list_get_first(&n);

		start = (i==0)?s->start:0;
		end = (n->next == s->buffer)?s->end:CHUNKSIZE;

		chunksz = end - start;

		if (chunksz < len) {
			memcpy(buf + offset, data + start, chunksz);
			offset += chunksz;
			len -= chunksz;
		} else {
			memcpy(buf + offset, data + start, len);
			offset += len;
			break;
		}

		n = n->next;
	} while (n != s->buffer);

	return offset;
}


/* Flushes the stream s. Afterwards, s looks like it had just been created. */
void stream_flush(stream *s)
{
	list_destroy(&(s->buffer));
	s->start = s->end = 0;
}


/* Frees all resources that are associated with the stream s, including the
   stream object itself. Same as stream_flush(s) followed by free(s). */
void stream_destroy(stream *s)
{
	stream_flush(s);
	free(s);
}

/* Computes the size of the buffer of stream s, in bytes. */
size_t stream_size(stream *s)
{
	size_t len = list_size(&(s->buffer));

	switch (len) {
		case 0:
			return 0;
		case 1:
			return s->end - s->start;
		default:
			return (CHUNKSIZE - s->start) + (s->end) + \
			        (len - 2) * CHUNKSIZE;
	}
}



// The following is just for testing and debugging
/*
#define BUFLEN 4096

int main(int argc, char *argv[])
{
	stream *s;
	FILE *in, *out;
	char buf[BUFLEN];
	size_t cnt;

	if (argc != 3) {
		fprintf(stderr, "USAGE: %s <infile> <outfile>\n", argv[0]);
		fprintf(stderr, "       Reads from infile to stream, then writes stream to outfile.\n");
		return -1;
	}

	s = stream_create();

	if((in = fopen(argv[1], "r")) == NULL)
		return -1;

	if((out = fopen(argv[2], "w")) == NULL)
		return -1;

	// read from infile, write to stream
	while((cnt = fread(buf, sizeof(char), BUFLEN, in)) != 0)
		stream_write(s, buf, cnt);

	// read from stream, write to outfile
	while((cnt = stream_read(s, buf, BUFLEN)) != 0) {
		fwrite(buf, sizeof(char), cnt, out);
	}

	fclose(in);
	fclose(out);

	// useless since we read the whole stream anyway
	stream_flush(s);

	stream_destroy(s);

	return 0;
}
*/
