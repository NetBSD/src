/*	$NetBSD: preference.c,v 1.4 2000/08/29 15:10:20 takemura Exp $	*/

/*-
 * Copyright (c) 1999 Shin Takemura.
 * All rights reserved.
 *
 * This software is part of the PocketBSD.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the PocketBSD project
 *	and its contributors.
 * 4. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
#include <pbsdboot.h>
#include <commctrl.h>
#include <res/resource.h>


struct preference_s pref;
TCHAR* where_pref_load_from = NULL;
static TCHAR filenamebuf[1024];


#define SETTING_IDX 1
#define FB_TYPE 2
#define FB_WIDTH 3
#define FB_HEIGHT 4
#define FB_LINEBYTES 5
#define BOOT_TIME 6
#define FB_ADDR 7
#define PLATID_CPU 8
#define PLATID_MACHINE 9
#define SETTING_NAME 10
#define KERNEL_NAME 11
#define OPTIONS 12
#define CHECK_LAST_CHANCE 13
#define LOAD_DEBUG_INFO 14
#define SERIAL_PORT 15
#define REVERSE_VIDEO 16
#define AUTOBOOT 17

#define NOFID 18


TCHAR *id_table[] = {
0,
TEXT("setting_idx"),
TEXT("fb_type"),
TEXT("fb_width"),
TEXT("fb_height"),
TEXT("fb_linebytes"),
TEXT("boot_time"),
TEXT("fb_addr"),
TEXT("platid_cpu"),
TEXT("platid_machine"),
TEXT("setting_name"),
TEXT("kernel_name"),
TEXT("options"),
TEXT("check_last_chance"),
TEXT("load_debug_info"),
TEXT("serial_port"),
TEXT("reverse_video"),
TEXT("autoboot"),
};



void
pref_init(struct preference_s* pref)
{
	memset(pref, 0, sizeof(*pref));
}


/*
 argument file is handle that was already opened .
 if read is faile , this function will "not" close this handle .
 return 0 if error . if end of file , return -1
*/
int read1byte(HANDLE file,char *c){
	DWORD n;
	if(!ReadFile(file,c,sizeof(char),&n,NULL)){
		msg_printf(MSG_ERROR, TEXT("pref_load()"),
			   TEXT("ReadFile(): error=%d"), GetLastError());
		debug_printf(TEXT("ReadFile(): error=%d\r"), GetLastError());
		
		return 0;
	}
	if (n != sizeof(char)) {
		if( n == 0){
			return (-1);
		}
		msg_printf(MSG_ERROR, TEXT("pref_load()"),
			   TEXT("ReadFile(): read %d bytes"), n);
		debug_printf(TEXT("ReadFile(): read %d bytes\r"), n);
		
		return 0;
	}
	return 1;
}

/*
 argument file is handle that was already opened .
 if write is faile, this function will "not" close this handle .
 return 0 if error . write one line of string .
*/
int write1string(HANDLE file,char *string){
	DWORD n;
	if(!WriteFile(file,string,sizeof(char)*strlen(string),&n,NULL)){
	    msg_printf(MSG_ERROR, TEXT("pref_write()"),
			   TEXT("WriteFile(): error=%d"), GetLastError());
		debug_printf(TEXT("WriteFile(): error=%d\n"), GetLastError());

		return 0;
	}
	if (n != sizeof(char)*strlen(string)) {
		msg_printf(MSG_ERROR, TEXT("pref_write()"),
			   TEXT("WriteFile(): write %d bytes"), n);
		debug_printf(TEXT("WriteFile(): write %d bytes\n"), n);

		return (0);
	}

	return 1;
}

void
pref_dump(struct preference_s* pref)
{
	debug_printf(TEXT("    kernel_name: %s\n"), pref->kernel_name);
	debug_printf(TEXT("        options: %s\n"), pref->options);
	debug_printf(TEXT("  user def name: %s\n"), pref->setting_name);
	debug_printf(TEXT("  setting index: %d\n"), pref->setting_idx);
	debug_printf(TEXT("           type: %d\n"), pref->fb_type);
	debug_printf(TEXT("          width: %d\n"), pref->fb_width);
	debug_printf(TEXT("         height: %d\n"), pref->fb_height);
	debug_printf(TEXT("     bytes/line: %d\n"), pref->fb_linebytes);
	debug_printf(TEXT("           addr: %d\n"), pref->fb_addr);
	debug_printf(TEXT("            cpu: %08lx\n"), pref->platid_cpu);
	debug_printf(TEXT("        machine: %08lx\n"), pref->platid_machine);
	debug_printf(TEXT("    last chance: %S\n"), pref->check_last_chance ?
		     "TRUE" : "FALSE");
	debug_printf(TEXT("load debug info: %S\n"), pref->load_debug_info ?
		     "TRUE" : "FALSE");
	debug_printf(TEXT("    serial port: %S\n"), pref->serial_port ?
		     "ON" : "OFF");
}



/* To Do . modify this function*/
int
pref_read(TCHAR* filename, struct preference_s* pref)
{
	HANDLE file;
	DWORD length;
	static struct preference_s buf;
	char tempbuf[1024];
	TCHAR unidata[1024];
	TCHAR identif[1024];
	char c;
	int i,flag,d;
	int result;/* count of loading pref item */
	
	
	file = CreateFile(
		filename,      	/* file name */
		GENERIC_READ,	/* access (read-write) mode */
		FILE_SHARE_READ,/* share mode */
		NULL,		/* pointer to security attributes */
		OPEN_EXISTING,	/* how to create */
		FILE_ATTRIBUTE_NORMAL,	/* file attributes*/
		NULL		/* handle to file with attributes to */
		);
	
	if (file == INVALID_HANDLE_VALUE) {
		return (-1);
	}
	
	
	
	
	flag = 1;
	result = 0;
	while(flag){
		i = 0;
		d = read1byte(file,&c);
		if( d <=0){
			if(d == -1){
				flag = 0;
				break;
			}
			else
			{
				CloseHandle(file);
				return (-1);
			}	
		}
	

		while(c != ':' && c != '\r' && c!= '\n'){
			tempbuf[i] = c;
			d = read1byte(file,&c);
			if( d <=0){
				if(d == -1){
					flag = 0;
					break;
				}
				else
				{
					CloseHandle(file);
					return (-1);
				}
			}
			i++;
		}
		if(c == ':'){
			

			tempbuf[i] = '\0';
			length = MultiByteToWideChar(CP_ACP,0,tempbuf,-1,identif,0);
			MultiByteToWideChar(CP_ACP,0,tempbuf,-1,identif,length);
			
			i = 0;
			d = read1byte(file,&c);
			flag = 1;
			while(c != '\r' && c != '\n' && flag){/* get unidata */
				if(d <= 0){
					if(d == -1){
						flag = 0;/* though needless ... */
						break;
					}
					else{
						CloseHandle(file);
						return -1;
					}
				}
				tempbuf[i] = c;
				d = read1byte(file,&c);
				i++;
				
			}
			if(c == '\r'){/* skip \n */
				read1byte(file,&c);
			}
			tempbuf[i] = '\0';
			length = MultiByteToWideChar(CP_ACP,0,tempbuf,-1,unidata,0);
			MultiByteToWideChar(CP_ACP,0,tempbuf,-1,unidata,length);
			
			for(i = 1; i < NOFID;i++){
				if(wcscmp(identif,id_table[i])==0){
					break;
				}
			}
			switch(i){
			case SETTING_IDX:
				d = _wtoi(unidata);
				buf.setting_idx = d;
				result++;
				break;
			case FB_TYPE:
				d = _wtoi(unidata);
				buf.fb_type = d;
				result++;
				break;
			case FB_WIDTH:
				d = _wtoi(unidata);
				buf.fb_width = d;
				result++;
				break;
			case FB_HEIGHT:
				d = _wtoi(unidata);
				buf.fb_height = d;
				result++;
				break;
			case FB_LINEBYTES:
				d = _wtoi(unidata);
				buf.fb_linebytes = d;
				result++;
				break;
			case BOOT_TIME:
				d = _wtoi(unidata);
				buf.boot_time = d;
				result++;
				break;
			case FB_ADDR:
				d = _wtoi(unidata);
				buf.fb_addr = d;
				result++;
				break;
			case PLATID_CPU:
				d = _wtoi(unidata);
				buf.platid_cpu = d;
				result++;
				break;
			case PLATID_MACHINE:
				d = _wtoi(unidata);
				buf.platid_machine = d;
				result++;
				break;
			case SETTING_NAME:
				wcscpy(buf.setting_name,unidata);
				result++;
				break;
			case KERNEL_NAME:
				wcscpy(buf.kernel_name,unidata);
				result++;
				break;
			case OPTIONS:
				wcscpy(buf.options,unidata);
				result++;
				break;
			case CHECK_LAST_CHANCE:
				if(wcscmp(unidata,TEXT("t")) == 0){
					buf.check_last_chance = TRUE;
				}
				else{
					buf.check_last_chance = FALSE;
				}
				result++;
				break;
			case LOAD_DEBUG_INFO:
				if(wcscmp(unidata,TEXT("t")) == 0){
					buf.load_debug_info = TRUE;
				}
				else{
					buf.load_debug_info = FALSE;
				}
				result++;
				break;
			case SERIAL_PORT:
				if(wcscmp(unidata,TEXT("t")) == 0){
					buf.serial_port = TRUE;
				}
				else{
					buf.serial_port = FALSE;
				}
				result++;
				break;
			case REVERSE_VIDEO:
				if(wcscmp(unidata,TEXT("t")) == 0){
					buf.reverse_video = TRUE;
				}
				else{
					buf.reverse_video = FALSE;
				}
				result++;
				break;
			case AUTOBOOT:
				if(wcscmp(unidata,TEXT("t")) == 0){
					buf.autoboot = TRUE;
				}
				else{
					buf.autoboot = FALSE;
				}
				result++;
				break;
			default:
				break;
			}
		}
	}
	
	

	
	CloseHandle(file);

#if 0
	/* shortage of item is not error */
	if(result != NOFID -1){
		return -1;/* data is shortage */
	}
#endif
	
	*pref = buf;

	return (0);
}



int
pref_load(struct path_s load_path[], int pathlen)
{
	int i;

	where_pref_load_from = NULL;
	for (i = 0; i < pathlen; i++) {
		wsprintf(filenamebuf, TEXT("%s%s"),
			 load_path[i].name, PREFNAME);
		debug_printf(TEXT("pref_load: try to '%s'\n"), filenamebuf);

		if (pref_read(filenamebuf, &pref) == 0) {
			debug_printf(TEXT("pref_load: succeded, '%s'.\n"),
				     filenamebuf);
			pref_dump(&pref);
			where_pref_load_from = filenamebuf;
			return (0);
		}
	}

	return (-1);
}


int
pref_save(struct path_s load_path[], int pathlen)
{
	int i;

	if (where_pref_load_from) {
		if (pref_write(where_pref_load_from, &pref) != 0) {
			msg_printf(MSG_ERROR, TEXT("Error()"),
			    TEXT("Can't write %s"), where_pref_load_from);
			return -1;
		}
		return 0;
	}
	for (i = 0; i < pathlen; i++) {
		if (!(load_path[i].flags & PATH_SAVE)) {
			continue;
		}
		wsprintf(filenamebuf, TEXT("%s%s"),
		    load_path[i].name, PREFNAME);
		debug_printf(TEXT("pref_save: try to '%s'\n"), filenamebuf);
		if (pref_write(filenamebuf, &pref) == 0) {
			debug_printf(TEXT("pref_write: succeded, '%s'.\n"),
			    filenamebuf);
			return (0);
		}
	}

	msg_printf(MSG_ERROR, TEXT("Error()"),
	    TEXT("Can't write %s"), PREFNAME);

	return (-1);
}


int
pref_write(TCHAR* filename, struct preference_s* buf)
{
	HANDLE file;
	char tempbuf[1024];
	TCHAR unibuf[1024];
	DWORD length;


	debug_printf(TEXT("pref_write('%s').\n"), filename);
	pref_dump(&pref);

       	file = CreateFile(
	       	filename,      	/* file name */
	       	GENERIC_WRITE,	/* access (read-write) mode */
	       	FILE_SHARE_WRITE,/* share mode */
	       	NULL,		/* pointer to security attributes */
	       	CREATE_ALWAYS,	/* how to create */
	       	FILE_ATTRIBUTE_NORMAL,	/* file attributes*/
	       	NULL		/* handle to file with attributes to */
	       	);

	if (file == INVALID_HANDLE_VALUE) {
		debug_printf(TEXT("CreateFile(): error=%d\n"), GetLastError());
		return (-1);
	}


	wsprintf(unibuf,TEXT("setting_idx:%d\r\n"),buf->setting_idx);
	debug_printf(TEXT("setting_idx,tempbuf=%s"),unibuf);
		length = WideCharToMultiByte(CP_ACP,0,unibuf,-1,NULL,0,NULL,NULL);
	WideCharToMultiByte(CP_ACP,0,unibuf,-1,tempbuf,length,NULL,NULL);
	if(!write1string(file,tempbuf)){
		CloseHandle(file);
		return (-1);
	}


	wsprintf(unibuf,TEXT("fb_type:%d\r\n"),buf->fb_type);
	debug_printf(TEXT("fb_type,tempbuf=%s,"),unibuf);
	length = WideCharToMultiByte(CP_ACP,0,unibuf,-1,NULL,0,NULL,NULL);
	WideCharToMultiByte(CP_ACP,0,unibuf,-1,tempbuf,length,NULL,NULL);
	if(!write1string(file,tempbuf)){
		CloseHandle(file);
		return (-1);
	}


	wsprintf(unibuf,TEXT("fb_width:%d\r\n"),buf->fb_width);
	debug_printf(TEXT("fb_width,tempbuf=%s"),unibuf);
	length = WideCharToMultiByte(CP_ACP,0,unibuf,-1,NULL,0,NULL,NULL);
	WideCharToMultiByte(CP_ACP,0,unibuf,-1,tempbuf,length,NULL,NULL);
	if(!write1string(file,tempbuf)){
		CloseHandle(file);
		return (-1);
	}



	wsprintf(unibuf,TEXT("fb_height:%d\r\n"),buf->fb_height);
	debug_printf(TEXT("fb_height,tempbuf=%s"),unibuf);
	length = WideCharToMultiByte(CP_ACP,0,unibuf,-1,NULL,0,NULL,NULL);
	WideCharToMultiByte(CP_ACP,0,unibuf,-1,tempbuf,length,NULL,NULL);
	if(!write1string(file,tempbuf)){
		CloseHandle(file);
		return (-1);
	}	



	wsprintf(unibuf,TEXT("fb_linebytes:%d\r\n"),buf->fb_linebytes);
	debug_printf(TEXT("fb_linebytes,tempbuf=%s"),unibuf);
	length = WideCharToMultiByte(CP_ACP,0,unibuf,-1,NULL,0,NULL,NULL);
	WideCharToMultiByte(CP_ACP,0,unibuf,-1,tempbuf,length,NULL,NULL);
	if(!write1string(file,tempbuf)){
		CloseHandle(file);
		return (-1);
	}	


	wsprintf(unibuf,TEXT("boot_time:%d\r\n"),buf->boot_time);
	debug_printf(TEXT("boot_time,tempbuf=%s\n"),unibuf);
	length = WideCharToMultiByte(CP_ACP,0,unibuf,-1,NULL,0,NULL,NULL);
	WideCharToMultiByte(CP_ACP,0,unibuf,-1,tempbuf,length,NULL,NULL);
	if(!write1string(file,tempbuf)){
		CloseHandle(file);
		return (-1);
	}	

	wsprintf(unibuf,TEXT("fb_addr:%d\r\n"),buf->fb_addr);
	debug_printf(TEXT("fb_addr,tempbuf=%s"),unibuf);
	length = WideCharToMultiByte(CP_ACP,0,unibuf,-1,NULL,0,NULL,NULL);
	WideCharToMultiByte(CP_ACP,0,unibuf,-1,tempbuf,length,NULL,NULL);
	if(!write1string(file,tempbuf)){
		CloseHandle(file);
		return (-1);
	}	



    wsprintf(unibuf,TEXT("platid_cpu:%d\r\n"),buf->platid_cpu);
	debug_printf(TEXT("platid_cpu,tempbuf=%s"),unibuf);
	length = WideCharToMultiByte(CP_ACP,0,unibuf,-1,NULL,0,NULL,NULL);
	WideCharToMultiByte(CP_ACP,0,unibuf,-1,tempbuf,length,NULL,NULL);
	if(!write1string(file,tempbuf)){
		CloseHandle(file);
		return (-1);
	}	


	wsprintf(unibuf,TEXT("platid_machine:%d\r\n"),buf->platid_machine);
	debug_printf(TEXT("platid_machine,tempbuf=%s"),unibuf);
	length = WideCharToMultiByte(CP_ACP,0,unibuf,-1,NULL,0,NULL,NULL);
	WideCharToMultiByte(CP_ACP,0,unibuf,-1,tempbuf,length,NULL,NULL);
	if(!write1string(file,tempbuf)){
		CloseHandle(file);
		return (-1);
	}	



	wsprintf(unibuf,TEXT("setting_name:%s\r\n"),buf->setting_name);
	debug_printf(TEXT("setting_name,unibuf=%s,wcslen=%d"),unibuf,
		wcslen(unibuf));
	length = WideCharToMultiByte(CP_ACP,0,unibuf,-1,NULL,0,NULL,NULL);
	WideCharToMultiByte(CP_ACP,0,unibuf,-1,tempbuf,length,NULL,NULL);

	if(!write1string(file,tempbuf)){
		CloseHandle(file);
		return (-1);
	}



	wsprintf(unibuf,TEXT("kernel_name:%s\r\n"),buf->kernel_name);
	debug_printf(TEXT("kernel_name,unibuf=%s,wcslen=%d"),unibuf,
		wcslen(unibuf));
	length = WideCharToMultiByte(CP_ACP,0,unibuf,-1,NULL,0,NULL,NULL);
	WideCharToMultiByte(CP_ACP,0,unibuf,-1,tempbuf,length,NULL,NULL);
	if(!write1string(file,tempbuf)){
		CloseHandle(file);
		return (-1);
	}



	wsprintf(unibuf,TEXT("options:%s\r\n"),buf->options);
	debug_printf(TEXT("options,unibuf=%s,wcslen=%d"),unibuf,
		wcslen(unibuf));
	length = WideCharToMultiByte(CP_ACP,0,unibuf,-1,NULL,0,NULL,NULL);
	WideCharToMultiByte(CP_ACP,0,unibuf,-1,tempbuf,length,NULL,NULL);
	if(!write1string(file,tempbuf)){
		CloseHandle(file);
		return (-1);
	}


	if(buf->check_last_chance){
		wsprintf(unibuf,TEXT("check_last_chance:t\r\n"));
	}
	else{
	wsprintf(unibuf,TEXT("check_last_chance:n\r\n"));
	}


	length = WideCharToMultiByte(CP_ACP,0,unibuf,-1,NULL,0,NULL,NULL);
	WideCharToMultiByte(CP_ACP,0,unibuf,-1,tempbuf,length,NULL,NULL);
	if(!write1string(file,tempbuf)){
		CloseHandle(file);
		return (-1);
	}




	if(buf->load_debug_info){
		wsprintf(unibuf,TEXT("load_debug_info:t\r\n"));
	}
	else{
	wsprintf(unibuf,TEXT("load_debug_info:n\r\n"));
	}


	length = WideCharToMultiByte(CP_ACP,0,unibuf,-1,NULL,0,NULL,NULL);
	WideCharToMultiByte(CP_ACP,0,unibuf,-1,tempbuf,length,NULL,NULL);


	if(!write1string(file,tempbuf)){
		CloseHandle(file);
		return (-1);
	}



	if(buf->serial_port){
		wsprintf(unibuf,TEXT("serial_port:t\r\n"));
	}
	else{
	wsprintf(unibuf,TEXT("serial_port:n\r\n"));
	}

	length = WideCharToMultiByte(CP_ACP,0,unibuf,-1,NULL,0,NULL,NULL);
	WideCharToMultiByte(CP_ACP,0,unibuf,-1,tempbuf,length,NULL,NULL);

	if(!write1string(file,tempbuf)){
		CloseHandle(file);
		return (-1);
	}	



	if(buf->reverse_video){
		wsprintf(unibuf,TEXT("reverse_video:t\r\n"));
	}
	else{
	wsprintf(unibuf,TEXT("reverse_video:n\r\n"));
	}

length = WideCharToMultiByte(CP_ACP,0,unibuf,-1,NULL,0,NULL,NULL);
	WideCharToMultiByte(CP_ACP,0,unibuf,-1,tempbuf,length,NULL,NULL);
	if(!write1string(file,tempbuf)){
		CloseHandle(file);
		return (-1);
	}

	if(buf->autoboot){
		wsprintf(unibuf,TEXT("autoboot:t\r\n"));
	}
	else{
		wsprintf(unibuf,TEXT("autoboot:n\r\n"));
	}

	length = WideCharToMultiByte(CP_ACP,0,unibuf,-1,NULL,0,NULL,NULL);
	WideCharToMultiByte(CP_ACP,0,unibuf,-1,tempbuf,length,NULL,NULL);
	if(!write1string(file,tempbuf)){
		CloseHandle(file);
		return (-1);
	}

	CloseHandle(file);
	return (0);
}

