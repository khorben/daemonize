/* $Id$ */
/* Copyright (c) 2020 Pierre Pronchery <khorben@defora.org> */
/* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>. */



#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "daemonize.h"

#ifndef PROGNAME_DAEMONIZE
# define PROGNAME_DAEMONIZE	"daemonize"
#endif


/* main */
/* private */
/* prototypes */
static int _usage(void);


/* public */
/* functions */
/* main */
int main(int argc, char * argv[])
{
	DaemonizePrefs prefs;
	int o;

	memset(&prefs, 0, sizeof(prefs));
	prefs.daemon = 1;
	while((o = getopt(argc, argv, "Fg:p:u:")) != -1)
		switch(o)
		{
			case 'F':
				prefs.daemon = 0;
				break;
			case 'g':
				prefs.groupname = optarg;
				break;
			case 'p':
				prefs.pidfile = optarg;
				break;
			case 'u':
				prefs.username = optarg;
				break;
			default:
				return _usage();
		}
	if(optind + 1 > argc)
		return _usage();
	return daemonize(&prefs, argv[optind],
			argc - optind - 1, &argv[optind + 1]);
}


/* private */
/* functions */
/* usage */
static int _usage(void)
{
	fputs("Usage: " PROGNAME_DAEMONIZE " [-F][-p filename] program [args...]\n"
			"  -F	Run in foreground\n"
			"  -p	Set the PID file\n", stderr);
	return 1;
}
