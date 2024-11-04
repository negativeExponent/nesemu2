/***************************************************************************
 *   Copyright (C) 2013 by James Holodnak                                  *
 *   jamesholodnak@gmail.com                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifdef WIN32
	#include <direct.h>
	#include <io.h>
#else
	#include <sys/stat.h>
//	#include <sys/types.h>
	#include <unistd.h>
#endif
#include <stdlib.h>
#include <string.h>
#include "misc/config.h"
#include "misc/vars.h"
#include "misc/memutil.h"
#include "misc/log.h"
#include "misc/paths.h"
#include "system/main.h"
#include "system/system.h"
#include "version.h"

static vars_t *configvars = 0;

//this path stuff needs to be moved
static void mkdirr(char *path)
{
	char *tmp = mem_strdup(path);
	char *p = tmp;
	int num = 0;

	//normalize the path string
	paths_normalize(tmp);

	log_printf("mkdirr:  creating directory '%s'\n",path);

	//make all the directories between the root and the desired directory
	for(num=0,p=tmp;(p = strchr(p,PATH_SEPERATOR));num++) {
		if(num == 0) {
			p++;
			continue;
		}
		*p = 0;
#ifdef _WIN32
		mkdir(tmp);
#else
		mkdir(tmp,0755);
#endif
		chmod(tmp,0755);
		*p = PATH_SEPERATOR;
		p++;
	}

	//now create the directory we want
#ifdef _WIN32
	mkdir(tmp);
#else
	mkdir(tmp,0755);
#endif
	chmod(path,0755);

	//free tmp string
	mem_free(tmp);
}

static void makepath(char *str)
{
	//test if the path exists already, if not create it
	if(access(str,0) != 0)
		mkdirr(str);
}

//this function looks around for a configuration file.  it checks:
//  1. current working directory
//  2. $HOME directory
//  3. same directory the executable is in
//if it isnt found, it defaults the executable directory
static int findconfig(char *dest)
{
	char *cwd = system_getcwd();
	char *home = getenv("HOME");

	//first see if the configfilename string isnt empty
	if(strcmp(dest,"") != 0) {
		return(0);
	}

	//now look in the current working directory
	sprintf(dest,"%s%c%s",cwd,PATH_SEPERATOR,CONFIG_FILENAME);
	log_printf("looking for configuration at '%s'\n",dest);
	if(access(dest,06) == 0) {
		return(0);
	}

	//check the users home directory
	if(home) {
		sprintf(dest,"%s%c.nesemu2%c%s",home,PATH_SEPERATOR,PATH_SEPERATOR,CONFIG_FILENAME);
		log_printf("looking for configuration at '%s'\n",dest);
		if(access(dest,06) == 0) {
			return(0);
		}
	}

#ifdef WIN32
	//win32 it is ok to store in the same directory as executable
	//now check the executable directory
	sprintf(dest,"%s%c%s",exepath,PATH_SEPERATOR,CONFIG_FILENAME);
	log_printf("looking for configuration at '%s'\n",dest);
	if(access(dest,06) == 0) {
		return(0);
	}

	//set default configuration filename
	sprintf(dest,"%s%c%s",exepath,PATH_SEPERATOR,CONFIG_FILENAME);

#else
	//linux it is not ok to store in the same directory as executable (/usr/bin or something)
	if(home) {
		sprintf(dest,"%s%c.nesemu2%c%s",home,PATH_SEPERATOR,PATH_SEPERATOR,CONFIG_FILENAME);
	}
	else {
		sprintf(dest,"%s%c%s",cwd,PATH_SEPERATOR,CONFIG_FILENAME);
		log_printf("system_findconfig:  HOME environment var not set!  using current directory.\n");
	}
#endif

	return(1);
}

//initialize the configuration defaults
static vars_t *config_get_defaults()
{
	vars_t *ret = vars_create();
	char *str;

	vars_set_int   (ret,F_CONFIG,"video.framelimit",			1);
	vars_set_int   (ret,F_CONFIG,"video.fullscreen",			0);
	vars_set_int   (ret,F_CONFIG,"video.scale",					1);
	vars_set_string(ret,F_CONFIG,"video.filter",					"none");

	vars_set_string(ret,F_CONFIG,"input.port0",					"joypad0");
	vars_set_string(ret,F_CONFIG,"input.port1",					"joypad1");
	vars_set_string(ret,F_CONFIG,"input.expansion",				"none");

#ifdef SDL2
	vars_set_int   (ret,F_CONFIG,"input.joypad0.a",				27 /* 'x' */);
	vars_set_int   (ret,F_CONFIG,"input.joypad0.b",				29 /*'z'*/);
	vars_set_int   (ret,F_CONFIG,"input.joypad0.select",		 4 /*'a' */);
	vars_set_int   (ret,F_CONFIG,"input.joypad0.start",		    22 /*'s'*/);
	vars_set_int   (ret,F_CONFIG,"input.joypad0.up",			82);
	vars_set_int   (ret,F_CONFIG,"input.joypad0.down",			81);
	vars_set_int   (ret,F_CONFIG,"input.joypad0.left",			80);
	vars_set_int   (ret,F_CONFIG,"input.joypad0.right",			82);

	vars_set_int   (ret,F_CONFIG,"input.joypad1.a",				11 /*'h'*/);
	vars_set_int   (ret,F_CONFIG,"input.joypad1.b",				10 /*'g'*/);
	vars_set_int   (ret,F_CONFIG,"input.joypad1.select",		23 /*'t'*/);
	vars_set_int   (ret,F_CONFIG,"input.joypad1.start",			28 /*'y'*/);
	vars_set_int   (ret,F_CONFIG,"input.joypad1.up",			12 /*'i'*/);
	vars_set_int   (ret,F_CONFIG,"input.joypad1.down",			14 /*'k'*/);
	vars_set_int   (ret,F_CONFIG,"input.joypad1.left",			13 /*'j'*/);
	vars_set_int   (ret,F_CONFIG,"input.joypad1.right",			15 /*'l'*/);
#else
	vars_set_int   (ret,F_CONFIG,"input.joypad0.a",				'x');
	vars_set_int   (ret,F_CONFIG,"input.joypad0.b",				'z');
	vars_set_int   (ret,F_CONFIG,"input.joypad0.select",		'a');
	vars_set_int   (ret,F_CONFIG,"input.joypad0.start",		's');
	vars_set_int   (ret,F_CONFIG,"input.joypad0.up",			273);
	vars_set_int   (ret,F_CONFIG,"input.joypad0.down",			274);
	vars_set_int   (ret,F_CONFIG,"input.joypad0.left",			276);
	vars_set_int   (ret,F_CONFIG,"input.joypad0.right",		275);	

	vars_set_int   (ret,F_CONFIG,"input.joypad1.a",				'h');
	vars_set_int   (ret,F_CONFIG,"input.joypad1.b",				'g');
	vars_set_int   (ret,F_CONFIG,"input.joypad1.select",		't');
	vars_set_int   (ret,F_CONFIG,"input.joypad1.start",		'y');
	vars_set_int   (ret,F_CONFIG,"input.joypad1.up",			'i');
	vars_set_int   (ret,F_CONFIG,"input.joypad1.down",			'k');
	vars_set_int   (ret,F_CONFIG,"input.joypad1.left",			'j');
	vars_set_int   (ret,F_CONFIG,"input.joypad1.right",		'l');
#endif

	vars_set_string(ret,F_CONFIG,"input.zapper.trigger",		"mb0");

	vars_set_int   (ret,F_CONFIG,"sound.enabled",				1);

#ifdef WIN32
	vars_set_string(ret,F_CONFIG,"path.data",						"%exepath%/data");
	vars_set_string(ret,F_CONFIG,"path.user",						"%path.data%");
#else
	vars_set_string(ret,F_CONFIG,"path.data",						"/usr/share/nesemu2");
	vars_set_string(ret,F_CONFIG,"path.user",						"%home%/.nesemu2");
#endif

	vars_set_string(ret,F_CONFIG,"path.bios",						"%path.data%/bios");
	vars_set_string(ret,F_CONFIG,"path.patch",					"%path.data%/patch");
	vars_set_string(ret,F_CONFIG,"path.palette",					"%path.data%/palette");
	vars_set_string(ret,F_CONFIG,"path.xml",						"%path.data%/xml");

	vars_set_string(ret,F_CONFIG,"path.save",						"%path.user%/save");
	vars_set_string(ret,F_CONFIG,"path.state",					"%path.user%/state");
	vars_set_string(ret,F_CONFIG,"path.cheat",					"%path.user%/cheat");

	vars_set_string(ret,F_CONFIG,"palette.source",				"generator");
	vars_set_int   (ret,F_CONFIG,"palette.hue",					-15);
	vars_set_int   (ret,F_CONFIG,"palette.saturation",			45);
	vars_set_string(ret,F_CONFIG,"palette.filename",			"roni.pal");

	vars_set_string(ret,F_CONFIG,"nes.gamegenie.bios",			"genie.rom");
	vars_set_int   (ret,F_CONFIG,"nes.gamegenie.enabled",		0);

	vars_set_string(ret,F_CONFIG,"nes.fds.bios",					"hlefds.bin");
	vars_set_int   (ret,F_CONFIG,"nes.fds.hle",					1);

	vars_set_string(ret,F_CONFIG,"nes.nsf.bios",					"nsfbios.bin");

	vars_set_string(ret,F_CONFIG,"nes.region",					"ntsc");
	vars_set_int   (ret,F_CONFIG,"nes.log_unhandled_io",		0);
	vars_set_int   (ret,F_CONFIG,"nes.pause_on_load",			0);

	vars_set_int   (ret,F_CONFIG,"cartdb.enabled",				1);
	vars_set_string(ret,F_CONFIG,"cartdb.filename",				"%path.xml%/NesCarts.xml;%path.xml%/NesCarts2.xml");

	vars_set_string(ret,0,"version",VERSION);
	vars_set_string(ret,0,"exepath",exepath);
	if((str = getenv("HOME")) != 0)
		vars_set_string(ret,0,"home",str);

	return(ret);
}

int config_init()
{
	vars_t *v;
	char tmp[1024];

	//find configuration file
	if(findconfig(configfilename) == 0)
		log_printf("main:  found configuration at '%s'\n",configfilename);
	else
		log_printf("main:  creating new configuration at '%s'\n",configfilename);

	configvars = config_get_defaults();
	if((v = vars_load(configfilename)) == 0) {
		log_printf("config_init:  unable to load file, using defaults\n");
	}
	else {
		//merge the loaded variables with the defaults
		vars_merge(configvars,v);

		//destroy the loaded vars
		vars_destroy(v);
	}

	//make the directories
	makepath(config_get_eval_string(tmp,"path.user"));
	makepath(config_get_eval_string(tmp,"path.save"));
	makepath(config_get_eval_string(tmp,"path.state"));
	makepath(config_get_eval_string(tmp,"path.cheat"));

	return(0);
}

void config_kill()
{
	vars_t *v = configvars;

	if(v == 0) {
		log_printf("config_kill:  internal error!  configvars = 0.\n");
		return;
	}
	vars_save(v,configfilename);
	vars_destroy(v);
}

//gets config string variable with variables expanded
char *config_get_eval_string(char *dest,char *name)
{
	char *tmp,*p,*p2;
	char varname[64];
	int pos;

	//get string
	if((p = config_get_string(name)) == 0)
		return(0);

	//make a copy of the string
	tmp = mem_strdup(p);

	strcpy(dest,tmp);

	for(pos=0,p=tmp;*p;p++) {

		//see if we find a '%'
		if(*p == '%') {

			//clear area to hold var name
			memset(varname,0,64);

			//see if it is missing the '%'
			if((p2 = strchr(p + 1,'%')) == 0) {
				log_printf("config_get_eval_string:  missing ending '%', just copying\n");
			}

			//not missing, replace with variable data
			else {
				//skip over the '%'
				p++;

				//terminate the substring
				*p2 = 0;

				//copy substring to varname array
				strcpy(varname,p);

				//set new position in the string we parsing
				p = p2 + 1;

				if(strcmp(varname,name) == 0) {
					log_printf("config_get_eval_string:  variable cannot reference itself (var '%s')\n",varname);
				}
				else {
					char *tmp2 = (char*)mem_alloc(1024);

					p2 = var_get_eval_string(tmp2,varname);
					if(p2 == 0) {
						log_printf("config_get_eval_string:  variable '%s' referenced non-existant variable '%s', using '.'\n",name,varname);
						dest[pos++] = '.';
						dest[pos] = 0;
					}
					else {
						while(*p2) {
							dest[pos++] = *p2++;
							dest[pos] = 0;
						}
					}
					mem_free(tmp2);
				}
			}
		}

		//copy the char
		dest[pos++] = *p;
		dest[pos] = 0;
	}

	//normalize the path
	paths_normalize(dest);

	//free tmp string and return
	mem_free(tmp);
	return(dest);
}

//get config var (wraps the vars_get_*() functions)
char *config_get_string(char *name)		{	return(vars_get_string(configvars,name,""));		}
int config_get_int(char *name)			{	return(vars_get_int   (configvars,name,0));		}
int config_get_bool(char *name)			{	return(vars_get_bool  (configvars,name,0));		}
double config_get_double(char *name)	{	return(vars_get_double(configvars,name,0.0f));	}

//set config var (wraps the vars_get_*() functions)
void config_set_string(char *name,char *data)	{	vars_set_string(configvars,F_CONFIG,name,data);	}
void config_set_int(char *name,int data)			{	vars_set_int   (configvars,F_CONFIG,name,data);	}
void config_set_bool(char *name,int data)			{	vars_set_bool  (configvars,F_CONFIG,name,data);	}
void config_set_double(char *name,double data)	{	vars_set_double(configvars,F_CONFIG,name,data);	}

//set var (wraps the vars_get_*() functions)
void var_set_string(char *name,char *data)	{	vars_set_string(configvars,0,name,data);	}
void var_set_int(char *name,int data)			{	vars_set_int   (configvars,0,name,data);	}
void var_set_bool(char *name,int data)			{	vars_set_bool  (configvars,0,name,data);	}
void var_set_double(char *name,double data)	{	vars_set_double(configvars,0,name,data);	}

void var_unset(char *name)
{
	vars_delete_var(configvars,name);
}

//semi-kludge for the 'set' command
var_t *config_get_head()
{
	return(configvars->vars);
}
