/* tinycap.c
**
** Copyright 2011, The Android Open Source Project
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**     * Redistributions of source code must retain the above copyright
**       notice, this list of conditions and the following disclaimer.
**     * Redistributions in binary form must reproduce the above copyright
**       notice, this list of conditions and the following disclaimer in the
**       documentation and/or other materials provided with the distribution.
**     * Neither the name of The Android Open Source Project nor the names of
**       its contributors may be used to endorse or promote products derived
**       from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY The Android Open Source Project ``AS IS'' AND
** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED. IN NO EVENT SHALL The Android Open Source Project BE LIABLE
** FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
** DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
** SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
** CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
** LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
** OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
** DAMAGE.
*/
 
#include "tinyalsa/asoundlib.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <string.h>
 
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>

#include "ringbuffer.h"
 
#define ID_RIFF 0x46464952
#define ID_WAVE 0x45564157
#define ID_FMT  0x20746d66
#define ID_DATA 0x61746164
 
#define FORMAT_PCM 1
 


RingBuffer *gringbuffer;


int capturing = 1;
static int closed = 0;
char *buffer;
pthread_t thread;
unsigned int card = 0;
unsigned int device = 0;
unsigned int channels = 1;
unsigned int rate = 8000;
unsigned int bits = 16;
unsigned int frames;
unsigned int period_size = 1024;
unsigned int period_count = 4;
 
unsigned int size;
 
unsigned int capture_sample(unsigned int card, unsigned int device,
                            unsigned int channels, unsigned int rate,
                            enum pcm_format format, unsigned int period_size,
                            unsigned int period_count);
 
void play_sample(unsigned int card, unsigned int device, unsigned int channels,
                 unsigned int rate, unsigned int bits, unsigned int period_size,
                 unsigned int period_count);                            
 
void sigint_handler(int sig)
{
    capturing = 0;
    closed = 1;
}
 
void thread1(void)
{
    sleep(2);
    printf("I am thread1\n");
 
    play_sample(card, device, channels, rate, bits, period_size, period_count);
}
 
int main(int argc, char **argv)
{
    gringbuffer = RingBuffer_Malloc(8192);
    if(gringbuffer == NULL)
    {
        printf("fail to malloc mem!\r\n");
    }
    enum pcm_format format;
    signal(SIGINT, sigint_handler);
    
    switch (bits) {
    case 32:
        format = PCM_FORMAT_S32_LE;
        break;
    case 24:
        format = PCM_FORMAT_S24_LE;
        break;
    case 16:
        format = PCM_FORMAT_S16_LE;
        break;
    default:
        printf("%d bits is not supported.\n", bits);
        return 1;
    }
 
     if((pthread_create(&thread, NULL, (void *)thread1, NULL)) == 0)  //comment2     
    {
        printf("Create pthread ok!\n");
    }else{
        printf("Create pthread failed!\n");
    }
        
    /* install signal handler and begin capturing */
    
    frames = capture_sample(card, device, channels,
                            rate, format,
                            period_size, period_count);
 
    
    printf("Captured %d frames\n", frames);                            
    RingBuffer_Free(gringbuffer);
    return 0;
}
 


unsigned int capture_sample(unsigned int card, unsigned int device,
                            unsigned int channels, unsigned int rate,
                            enum pcm_format format, unsigned int period_size,
                            unsigned int period_count)
{
    
    struct pcm_config config;
    struct pcm *pcm;
    char *buffer;
    unsigned int frames_read;
    unsigned int total_frames_read;
    unsigned int bytes_per_frame;

    memset(&config, 0, sizeof(config));
    config.channels = channels;
    config.rate = rate;
    config.period_size = period_size;
    config.period_count = period_count;
    config.format = format;
    config.start_threshold = 0;
    config.stop_threshold = 0;
    config.silence_threshold = 0;

    pcm = pcm_open(card, device, PCM_IN, &config);
    if (!pcm || !pcm_is_ready(pcm)) {
        fprintf(stderr, "Unable to open PCM device (%s)\n",
                pcm_get_error(pcm));
        return 0;
    }

    size = pcm_frames_to_bytes(pcm, pcm_get_buffer_size(pcm));
    printf("what the size is %d\r\n", size);
    buffer = malloc(size);
    if (!buffer) {
        fprintf(stderr, "Unable to allocate %u bytes\n", size);
        pcm_close(pcm);
        return 0;
    }

    if (1) {
        printf("Capturing sample: %u ch, %u hz, %u bit\n", channels, rate,
           pcm_format_to_bits(format));
    }

    bytes_per_frame = pcm_frames_to_bytes(pcm, 1);
    total_frames_read = 0;
    frames_read = 0;
    while (capturing) 
    {
        frames_read = pcm_readi(pcm, buffer, pcm_get_buffer_size(pcm));
        total_frames_read += frames_read;
        printf("data in %d\r\n", bytes_per_frame * frames_read);
        printf("data into ringbuffer %d\r\n", RingBuffer_In(gringbuffer, buffer, bytes_per_frame * frames_read));
    }


    free(buffer);
    pcm_close(pcm);


    return total_frames_read;
}



 
void play_sample(unsigned int card, unsigned int device, unsigned int channels,
                 unsigned int rate, unsigned int bits, unsigned int period_size,
                 unsigned int period_count)
{
    struct pcm_config config;
    struct pcm *pcm;
    
    memset(&config, 0, sizeof(config));
    config.channels = channels;
    config.rate = rate;
    config.period_size = period_size;
    config.period_count = period_count;
    config.format = PCM_FORMAT_S16_LE;
    config.start_threshold = 0;
    config.stop_threshold = 0;
    config.silence_threshold = 0;
 
 
    pcm = pcm_open(card, device, PCM_OUT, &config);
    if (!pcm || !pcm_is_ready(pcm)) {
        printf("Unable to open PCM device %u (%s)\n",
                device, pcm_get_error(pcm));
        return;
    }
 
 
    printf("Playing sample: %u ch, %u hz, %u bit\n", channels, rate, bits);
 
    char *readbuffer = malloc(8192);
    if(readbuffer == NULL) 
    {
        printf("unable to malloc mem!!\r\n");
    }

    printf("size len is%d\r\n", size);

    do {
        printf("data out is %d\r\n", RingBuffer_Out(gringbuffer, readbuffer, 4096));
        if (pcm_writei(pcm, readbuffer, pcm_bytes_to_frames(pcm, 4096)) < 0 ) 
        {
            printf("Error playing sample\n");
            break;
        }
    } while (!closed);
    free(readbuffer);
    pcm_wait(pcm, -1);
    
    pcm_close(pcm);
}
