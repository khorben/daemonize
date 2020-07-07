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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <grp.h>
#include <pwd.h>
#include <errno.h>
#include "common.h"
#include "daemonize.h"


/* Daemonize */
/* private */
/* prototypes */
static int _daemonize_error(char const * message);
static int _daemonize_prefs(DaemonizePrefs * prefs);


/* public */
/* functions */
/* daemonize */
int daemonize(DaemonizePrefs * prefs, char * program, int argc, char * argv[])
{
	char ** args;
	int i;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(\"%s\", %d, \"%s\")\n", __func__, program,
			argc, argv[0]);
#endif
	if((args = malloc(sizeof(*args) * (argc + 2))) == NULL)
		return _daemonize_error(NULL);
	if(prefs != NULL && _daemonize_prefs(prefs) != 0)
	{
		free(args);
		return 2;
	}
	args[0] = program;
	for(i = 0; i < argc; i++)
		args[i + 1] = argv[i];
	args[argc + 1] = NULL;
	execv(program, args);
	free(args);
	return _daemonize_error(program);
}


/* private */
/* daemonize_error */
static int _daemonize_error(char const * message)
{
	fprintf(stderr, "%s%s%s: %s\n", PROGNAME_DAEMONIZE,
			(message != NULL) ? ": " : "",
			(message != NULL) ? message : "",
			strerror(errno));
	return 2;
}


/* daemonize_prefs */
static int _daemonize_prefs(DaemonizePrefs * prefs)
{
	struct passwd * pw = NULL;
	struct group * gr = NULL;
	FILE * fp = NULL;

	if(prefs->username != NULL && (pw = getpwnam(prefs->username)) == NULL)
		return _daemonize_error(prefs->username);
	if(prefs->groupname != NULL
			&& (gr = getgrnam(prefs->groupname)) == NULL)
		return _daemonize_error(prefs->groupname);
	if(prefs->pidfile != NULL && (fp = fopen(prefs->pidfile, "w")) == NULL)
		return _daemonize_error(prefs->pidfile);
	if(gr != NULL && (setgid(gr->gr_gid) != 0 || setegid(gr->gr_gid) != 0))
		_daemonize_error("set(e)gid");
	if(pw != NULL && (setuid(pw->pw_uid) != 0 || seteuid(pw->pw_uid) != 0))
		_daemonize_error("set(e)uid");
	if(prefs->daemon && daemon(0, 0) != 0)
	{
		if(fp != NULL)
			fclose(fp);
		return _daemonize_error("daemon");
	}
	if(fp != NULL)
	{
		/* XXX check for errors */
		fprintf(fp, "%u\n", getpid());
		if(fclose(fp) != 0)
			/* XXX should log instead */
			_daemonize_error(prefs->pidfile);
	}
	return 0;
}
