#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include "flv_parse.h"

#pragma pack(1)

#define TAG_TYPE_SCRIPT 18
#define TAG_TYPE_AUDIO  8
#define TAG_TYPE_VIDEO  9


typedef struct {
    uint8_t   signature[3];
    uint8_t   version;
    uint8_t   flag;
    uint32_t  offset;
} flv_header_t;

typedef struct {
	uint8_t  type;
	uint8_t  datasize[3];
    uint8_t  timestamp[3];
    uint8_t  extTime;
	uint8_t  streamID[3];
} flv_tag_header_t;

const char *sound_formats[] = {
    "Linear PCM, platform endian",
    "ADPCM",
    "MP3",
    "Linear PCM, little endian",
    "Nellymoser 16-kHz mono",
    "Nellymoser 8-kHz mono",
    "Nellymoser",
    "G.711 A-law logarithmic PCM",
    "G.711 mu-law logarithmic PCM",
    "not defined by standard",
    "AAC",
    "Speex",
    "not defined by standard",
    "not defined by standard",
    "MP3 8-Khz",
    "Device-specific sound"
};

const char *sound_rates[] = {
    "5.5-Khz",
    "11-Khz",
    "22-Khz",
    "44-Khz"
};

const char *sound_sizes[] = {
    "8 bit",
    "16 bit"
};

const char *sound_types[] = {
    "Mono",
    "Stereo"
};

static void flv_header_parse(flv_header_t *header) {
    int i;
    uint32_t  offset = 0;
    uint8_t  *p = &header->offset;

    offset = p[0] << 24 | p[1] << 16 | p[2] << 8 | p[3];

    header->offset = offset;

	fprintf(stdout, "============== FLV Header ==============\n");
	fprintf(stdout, "signature:  0x %c %c %c\n", flv.signature[0], flv.signature[1], flv.signature[2]);
	fprintf(stdout, "version:    0x %X\n", flv.version);
    fprintf(stdout, "flags  :    0x %X\n", flv.flags); 
	fprintf(stdout, "headerSize: 0x %X\n", offset);
	fprintf(stdout, "========================================\n");
}

static void parse_audio_tag(int fd, int datasize) {
    uint8_t first;
    int     i;
    char    buf[2048];

    fprintf(stdout, "| ");

    read(fd, &first, sizeof(first));

    fprintf(stdout, "%s", sound_formats[(first & 0xf0) >> 4]);
    fprintf(stdout, "| ");
    fprintf(stdout, "%s", sound_rates[(first & 0x0c) >> 2]);
    fprintf(stdout, "| ");
    fprintf(stdout, "%s", sound_sizes[(first & 0x02) >> 1]);
    fprintf(stdout, "| ");
    fprintf(stdout, "%s", sound_types[first & 0x01]);

    for (i = 0; i < datasize / 2048; i++) {
        read(fd, buf, sizeof(buf));
    }

    datasize = datasize - i * 2048;
    read(fd, buf, datasize);
}

static void parse_video_tag(int fd) {

}

static void parse_script_tag(int fd) {

}

static int flv_body_parse(int fd) {
    int ret;
    uint32_t presize;
    flv_tag_header_t tag_header;

    for (;;) {
        ret = read(fd, &presize, sizeof(presize));
        ret = read(fd, &tag_header, sizeof(tag_header));
        if (ret == 0 || ret < 0) {
            return ret == 0 ? PARSE_SUCCESS : PARSE_FAILED; 
        }

        int datasize = tag_header.datasize[0] << 16 + tag_header.datasize[1] << 8 + tag_header.datasize[2];
        int timestamp = tag_header.extTime << 24 + tag_header.timestamp[0] << 16 + tag_header.timestamp[1] << 8 + tag_header.timestamp[2]; 

        switch (tag_header.type) {
        case TAG_TYPE_AUDIO:
            fprintf(stdout, "AUDIO %6d %6d |", datasize, timestamp);
            parse_audio_tag(fd, datasize);
            break;
        case TAG_TYPE_VIDEO:
            fprintf(stdout, "VIDEO %6d %6d |", datasize, timestamp);
            parse_video_tag(fd, datasize);
            break;
        case TAG_TYPE_SCRIPT:
            fprintf(stdout, "SCRIPT %6d %6d |", datasize, timestamp);
            parse_script_tag(fd);
            break;
        }
    }

    return PARSE_SUCCESS;
}

int flv_parse(const char *filename) {
    int fd;
    int ret;
    flv_header_t header;

    fd = open(filename, O_RDONLY);
    if (fd == -1) {
        return PARSE_FAILED;
    }

    ret = read(fd, &header, sizeof(header));

    flv_header_parse(&header);

    lseek(fd, header.offset, SEEK_SET);

    return flv_body_parse(fd);
}

