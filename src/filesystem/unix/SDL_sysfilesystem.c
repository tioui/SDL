/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2013 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/
#include "SDL_config.h"

#ifdef SDL_FILESYSTEM_UNIX

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* System dependent filesystem routines                                */

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "SDL_error.h"
#include "SDL_stdinc.h"
#include "SDL_filesystem.h"

static char *
readSymLink(const char *path)
{
    char *retval = NULL;
    ssize_t len = 64;
    ssize_t rc = -1;

    while (1)
    {
        char *ptr = (char *) SDL_realloc(retval, (size_t) len);
        if (ptr == NULL) {
            SDL_OutOfMemory();
            break;
        }

        retval = ptr;

        rc = readlink(path, retval, len);
        if (rc == -1) {
            break;  /* not a symlink, i/o error, etc. */
        } else if (rc < len) {
            retval[rc] = '\0';  /* readlink doesn't null-terminate. */
            return retval;  /* we're good to go. */
        }

        len *= 2;  /* grow buffer, try again. */
    }

    if (retval != NULL) {
        SDL_free(retval);
    }
    return NULL;
}


char *
SDL_GetBasePath(void)
{
    char *retval = NULL;

    /* is a Linux-style /proc filesystem available? */
    if (access("/proc", F_OK) == 0) {
        retval = readSymLink("/proc/self/exe");
        if (retval == NULL) {
            /* older kernels don't have /proc/self ... try PID version... */
            char path[64];
            const int rc = (int) SDL_snprintf(path, sizeof(path),
                                              "/proc/%llu/exe",
                                              (unsigned long long) getpid());
            if ( (rc > 0) && (rc < sizeof(path)) ) {
                retval = readSymLink(path);
            }
        }
    }

    /* If we had access to argv[0] here, we could check it for a path,
        or troll through $PATH looking for it, too. */

    if (retval != NULL) { /* chop off filename. */
        char *ptr = SDL_strrchr(retval, '/');
        if (ptr != NULL) {
            *(ptr+1) = '\0';
        } else {  /* shouldn't happen, but just in case... */
            SDL_free(retval);
            retval = NULL;
        }
    }

    if (retval != NULL) {
        /* try to shrink buffer... */
        char *ptr = (char *) SDL_realloc(retval, strlen(retval) + 1);
        if (ptr != NULL)
            retval = ptr;  /* oh well if it failed. */
    }

    return retval;
}

char *
SDL_GetPrefPath(const char *org, const char *app)
{
    /*
     * We use XDG's base directory spec, even if you're not on Linux.
     *  This isn't strictly correct, but the results are relatively sane
     *  in any case.
     *
     * http://standards.freedesktop.org/basedir-spec/basedir-spec-latest.html
     */
    const char *envr = SDL_getenv("XDG_DATA_HOME");
    const char *append = "/";
    char *retval = NULL;
    char *ptr = NULL;
    size_t len = 0;

    if (!envr) {
        /* You end up with "$HOME/.local/share/Game Name 2" */
        envr = SDL_getenv("HOME");
        if (!envr) {
            /* we could take heroic measures with /etc/passwd, but oh well. */
            SDL_SetError("neither XDG_DATA_HOME nor HOME environment is set");
            return NULL;
        }
        append = ".local/share/";
    } /* if */

    len = SDL_strlen(envr) + SDL_strlen(append) + SDL_strlen(app) + 2;
    retval = (char *) SDL_malloc(len);
    if (!retval) {
        SDL_OutOfMemory();
        return NULL;
    }

    SDL_snprintf(retval, len, "%s%s%s/", envr, append, app);

    for (ptr = retval+1; *ptr; ptr++) {
        if (*ptr == '/') {
            *ptr = '\0';
            mkdir(retval, 0700);
            *ptr = '/';
        }
    }
    mkdir(retval, 0700);

    return retval;
}

#endif /* SDL_FILESYSTEM_UNIX */

/* vi: set ts=4 sw=4 expandtab: */
