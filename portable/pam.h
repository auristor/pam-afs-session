/*
 * Portability wrapper around PAM header files.
 *
 * This header file includes the various PAM headers, wherever they may be
 * found on the system, and defines replacements for PAM functions that may
 * not be available on the local system.
 *
 * The canonical version of this file is maintained in the rra-c-util package,
 * which can be found at <http://www.eyrie.org/~eagle/software/rra-c-util/>.
 *
 * Written by Russ Allbery <rra@stanford.edu>
 *
 * The authors hereby relinquish any claim to any copyright that they may have
 * in this work, whether granted under contract or by operation of law or
 * international treaty, and hereby commit to the public, at large, that they
 * shall not, at any time in the future, seek to enforce any copyright in this
 * work against any person or entity, or prevent any person or entity from
 * copying, publishing, distributing or creating derivative works of this
 * work.
 */

#ifndef PORTABLE_PAM_H
#define PORTABLE_PAM_H 1

#include <config.h>
#include <portable/macros.h>

/* Linux PAM 1.1.0 requires sys/types.h before security/pam_modutil.h. */
#include <sys/types.h>

#ifndef HAVE_PAM_MODUTIL_GETPWNAM
# include <pwd.h>
#endif
#if defined(HAVE_SECURITY_PAM_APPL_H)
# include <security/pam_appl.h>
# include <security/pam_modules.h>
#elif defined(HAVE_PAM_PAM_APPL_H)
# include <pam/pam_appl.h>
# include <pam/pam_modules.h>
#endif
#if defined(HAVE_SECURITY_PAM_EXT_H)
# include <security/pam_ext.h>
#elif defined(HAVE_PAM_PAM_EXT_H)
# include <pam/pam_ext.h>
#endif
#if defined(HAVE_SECURITY_PAM_MODUTIL_H)
# include <security/pam_modutil.h>
#elif defined(HAVE_PAM_PAM_MODUTIL_H)
# include <pam/pam_modutil.h>
#endif
#include <stdarg.h>

BEGIN_DECLS

/* Default to a hidden visibility for all portability functions. */
#pragma GCC visibility push(hidden)

/*
 * If pam_modutil_getpwnam is missing, ideally we should roll our own using
 * getpwnam_r.  However, this is a fair bit of work, since we have to stash
 * the allocated memory in the PAM data so that it will be freed properly.
 * Bail for right now.
 */
#if !HAVE_PAM_MODUTIL_GETPWNAM
# define pam_modutil_getpwnam(h, u) getpwnam(u)
#endif

/* Prototype missing optional PAM functions. */
#if !HAVE_PAM_SYSLOG
void pam_syslog(const pam_handle_t *, int, const char *, ...);
#endif
#if !HAVE_PAM_VSYSLOG
void pam_vsyslog(const pam_handle_t *, int, const char *, va_list);
#endif

/* Undo default visibility change. */
#pragma GCC visibility pop

#endif /* !PORTABLE_PAM_H */
