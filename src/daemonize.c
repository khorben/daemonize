/* $Id$ */
/* Copyright (c) 2020-2025 Pierre Pronchery <khorben@defora.org> */
/* This file is part of daemonize */
/* All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */



#include <sys/param.h>
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
static int _daemonize_prefs(DaemonizePrefs const * prefs, char ** env);


/* public */
/* functions */
/* daemonize */
int daemonize(DaemonizePrefs const * prefs, char const * program,
		int argc, char * argv[])
{
	char ** args;
	char * env[2] = { NULL, NULL };
	int i;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(\"%s\", %d, \"%s\")\n", __func__, program,
			argc, argv[0]);
#endif
	if(chdir(prefs->chdir) != 0)
		return _daemonize_error(prefs->chdir);
	if((args = malloc(sizeof(*args) * (argc + 2))) == NULL)
		return _daemonize_error(NULL);
	if(prefs != NULL && _daemonize_prefs(prefs, env) != 0)
	{
		free(args);
		return 2;
	}
	if((args[0] = strdup(program)) == NULL)
	{
		free(args);
		return _daemonize_error("strdup");
	}
	for(i = 0; i < argc; i++)
		args[i + 1] = argv[i];
	args[argc + 1] = NULL;
	execve(program, args, env);
	free(args[0]);
	free(args);
	free(env[0]);
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
static int _prefs_setgroups(char const * username);

static int _daemonize_prefs(DaemonizePrefs const * prefs, char ** env)
{
	struct passwd * pw = NULL;
	struct group * gr = NULL;
	uid_t uid;
	gid_t gid;
	size_t len;
	FILE * fp = NULL;
	const char home[] = "HOME=";

	/* lookup the target user if set */
	if(prefs->username != NULL)
	{
		if((pw = getpwnam(prefs->username)) == NULL)
			return _daemonize_error(prefs->username);
		uid = pw->pw_uid;
		gid = pw->pw_gid;
	}
	/* lookup the target group */
	if(prefs->groupname != NULL)
	{
		if((gr = getgrnam(prefs->groupname)) == NULL)
			return _daemonize_error(prefs->groupname);
		gid = gr->gr_gid;
	}
	else if(pw != NULL && (gr = getgrgid(gid)) == NULL)
		return _daemonize_error("getgrgid");
	/* prepare the environment */
	if(pw == NULL && (pw = getpwuid(getuid())) == NULL)
		return _daemonize_error("getpwuid");
	if(env != NULL && pw->pw_dir != NULL && (len = strlen(pw->pw_dir)) > 0)
	{
		/* set $HOME */
		if((env[0] = malloc(sizeof(home) + len)) == NULL)
			return _daemonize_error("malloc");
		snprintf(env[0], sizeof(home) + len, "%s%s", home, pw->pw_dir);
	}
	/* open the PID file before dropping permissions */
	if(prefs->pidfile != NULL && (fp = fopen(prefs->pidfile, "w")) == NULL)
	{
		free(env[0]);
		return _daemonize_error(prefs->pidfile);
	}
	/* set the groups and user if set */
	if(gr != NULL)
	{
		if(setgid(gid) != 0)
			_daemonize_error("setgid");
		if(setegid(gid) != 0)
			_daemonize_error("setegid");
	}
	if(prefs->username != NULL)
	{
		_prefs_setgroups(prefs->username);
		if(setuid(uid) != 0)
			_daemonize_error("setuid");
		if(seteuid(uid) != 0)
			_daemonize_error("seteuid");
	}
	/* actually daemonize */
	if(prefs->daemon && daemon(0, 0) != 0)
	{
		if(fp != NULL)
			fclose(fp);
		free(env[0]);
		return _daemonize_error("daemon");
	}
	/* write the PID file */
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

static int _prefs_setgroups(char const * username)
{
	int ret;
	struct group * gr;
	int i;
	int n = 0;
	gid_t * groups = NULL;
	gid_t * p;

	setgroupent(1);
	while((gr = getgrent()) != NULL)
		for(i = 0; gr->gr_mem[i] != NULL; i++)
		{
			if(strcmp(gr->gr_mem[i], username) != 0)
				continue;
			if((p = realloc(groups, sizeof(*groups) * (n + 1)))
					== NULL)
			{
				free(groups);
				return _daemonize_error("realloc");
			}
			groups = p;
			groups[n++] = gr->gr_gid;
		}
	endgrent();
	ret = setgroups(n, groups);
	free(groups);
	if(ret != 0)
		_daemonize_error("setgroups");
	return ret;
}
