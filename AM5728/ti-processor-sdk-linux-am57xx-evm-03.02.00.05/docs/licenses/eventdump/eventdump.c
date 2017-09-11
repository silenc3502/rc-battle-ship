/*
 * eventdump.c
 *
 * Application for pasrsing AVRCP event messages
 *
 * Copyright(c) 2010 Texas Instruments. All rights reserved.      
 * All rights reserved.                                                  
 *                                                                       
 * Redistribution and use in source and binary forms, with or without    
 * modification, are permitted provided that the following conditions    
 * are met:                                                              
 *                                                                       
 *  * Redistributions of source code must retain the above copyright     
 *    notice, this list of conditions and the following disclaimer.      
 *  * Redistributions in binary form must reproduce the above copyright  
 *    notice, this list of conditions and the following disclaimer in    
 *    the documentation and/or other materials provided with the         
 *    distribution.                                                      
 *  * Neither the name Texas Instruments nor the names of its            
 *    contributors may be used to endorse or promote products derived    
 *    from this software without specific prior written permission.      
 *                                                                       
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS   
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT     
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT  
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT      
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT   
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <linux/input.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#define KEY_PLAYCD              200
#define KEY_STOPCD              166
#define KEY_PAUSECD             201
#define KEY_NEXTSONG            163
#define KEY_PREVIOUSSONG        165
#define KEY_REWIND              168
#define KEY_FASTFORWARD         208

static struct {
        const char *name;
        __u16 uinput;
} key_map[] = {
        { "PLAY",               KEY_PLAYCD },
        { "STOP",               KEY_STOPCD },
        { "PAUSE",              KEY_PAUSECD },
        { "FORWARD",            KEY_NEXTSONG },
        { "BACKWARD",           KEY_PREVIOUSSONG },
        { "REWIND",             KEY_REWIND },
        { "FAST FORWARD",       KEY_FASTFORWARD },
        { NULL,			0 },
};

int main(int argc, char **argv)
{
	int fd,i;
	if(argc < 2) {
		printf("usage: %s <device>\n", argv[0]);
		return 1;
	}
	fd = open(argv[1], O_RDONLY);
	struct input_event ev;

	while (1) {
		read(fd, &ev, sizeof(struct input_event));
		
		if ( ev.type == 1 && ev.value == 1 ) {
			for (i=0; (key_map[i].name != NULL) && (key_map[i].uinput != ev.code); i++);
			
			if (key_map[i].name != NULL) {
				printf("%s\n",key_map[i].name);
			}
			else {
				printf("Key %i State %i\n", ev.code, ev.value);
			}	
		}
	}
}

