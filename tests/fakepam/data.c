/*
 * Data manipulation functions for the fake PAM library, used for testing.
 *
 * This file contains the implementation of pam_get_* and pam_set_* for the
 * various data items supported by the PAM library, plus the PAM environment
 * manipulation functions.
 *
 * Written by Russ Allbery <rra@stanford.edu>
 * Copyright 2010 Board of Trustees, Leland Stanford Jr. University
 *
 * See LICENSE for licensing terms.
 */

#include <config.h>
#include <portable/pam.h>
#include <portable/system.h>

#include <tests/fakepam/testing.h>

/* Used for unused parameters to silence gcc warnings. */
#define UNUSED __attribute__((__unused__))


/*
 * Return a stored PAM data element in the provided data variable.
 */
int
pam_get_data(const pam_handle_t *pamh, const char *name, const void **data)
{
    struct fakepam_data *item;

    for (item = pamh->data; item != NULL; item = item->next)
        if (strcmp(item->name, name) == 0) {
            *data = item->data;
            return PAM_SUCCESS;
        }
    return PAM_NO_MODULE_DATA;
}


/*
 * Store a data item.  Replaces the existing data item (calling its cleanup)
 * if it is already set; otherwise, add a new data item.
 */
int
pam_set_data(pam_handle_t *pamh, const char *item, void *data,
             void (*cleanup)(pam_handle_t *, void *, int))
{
    struct fakepam_data *p;

    for (p = pamh->data; p != NULL; p = p->next)
        if (strcmp(p->name, item) == 0) {
            if (p->cleanup != NULL)
                p->cleanup (pamh, p->data, PAM_DATA_REPLACE);
            p->data = data;
            p->cleanup = cleanup;
            return PAM_SUCCESS;
        }
    p = malloc(sizeof(struct fakepam_data));
    if (p == NULL)
        return PAM_BUF_ERR;
    p->name = strdup(item);
    if (p->name == NULL) {
        free(p);
        return PAM_BUF_ERR;
    }
    p->data = data;
    p->cleanup = cleanup;
    p->next = pamh->data;
    pamh->data = p;
    return PAM_SUCCESS;
}


/*
 * Return the user for the PAM context.
 */
int
pam_get_user(pam_handle_t *pamh, PAM_CONST char **user,
             const char *prompt UNUSED)
{
    if (pamh->user == NULL)
        return PAM_CONV_ERR;
    else {
        *user = (PAM_CONST char *) pamh->user;
        return PAM_SUCCESS;
    }
}


/*
 * Return a setting in the PAM environment.
 */
PAM_CONST char *
pam_getenv(pam_handle_t *pamh, const char *name)
{
    size_t i;

    if (pamh->environ == NULL)
        return NULL;
    for (i = 0; pamh->environ[i] != NULL; i++)
        if (strncmp(name, pamh->environ[i], strlen(name)) == 0
            && pamh->environ[i][strlen(name)] == '=')
            return pamh->environ[i] + strlen(name) + 1;
    return NULL;
}


/*
 * Return a newly malloc'd copy of the complete PAM environment.  This must be
 * freed by the caller.
 */
char **
pam_getenvlist(pam_handle_t *pamh)
{
    char **environ;
    size_t i;

    if (pamh->environ == NULL)
        return NULL;
    for (i = 0; pamh->environ[i] != NULL; i++)
        ;
    environ = malloc((i + 1) * sizeof(char *));
    if (environ == NULL)
        return NULL;
    for (i = 0; pamh->environ[i] != NULL; i++) {
        environ[i] = strdup(pamh->environ[i]);
        if (environ[i] == NULL)
            goto fail;
    }
    return environ;

fail:
    for (i = 0; environ[i] != NULL; i++)
        free(environ[i]);
    free(environ);
    return NULL;
}


/*
 * Add a setting to the PAM environment.  If there is another existing
 * variable with the same value, the value is replaced, unless the setting
 * doesn't end in an equal sign.  If it doesn't end in an equal sign, any
 * existing environment variable of that name is removed.  This follows the
 * Linux PAM semantics.
 */
int
pam_putenv(pam_handle_t *pamh, const char *setting)
{
    const char *equals;
    size_t namelen;
    bool delete = false;
    bool found = false;
    size_t i, j;
    const char **environ;

    if (setting == NULL)
        return PAM_PERM_DENIED;
    equals = strchr(setting, '=');
    if (equals != NULL)
        namelen = equals - setting;
    else {
        delete = true;
        namelen = strlen(setting);
    }

    /* Handle the first call to pam_putenv. */
    if (pamh->environ == NULL) {
        if (delete)
            return PAM_BAD_ITEM;
        pamh->environ = malloc(2 * sizeof(char *));
        if (pamh->environ == NULL)
            return PAM_BUF_ERR;
        pamh->environ[0] = setting;
        pamh->environ[1] = NULL;
        return PAM_SUCCESS;
    }

    /*
     * We have an existing array.  See if we're replacing a value, deleting a
     * value, or adding a new one.  When deleting, waste a bit of memory but
     * save some time by not bothering to reduce the size of the array.
     */
    for (i = 0; pamh->environ[i] != NULL; i++)
        if (strncmp(setting, pamh->environ[i], namelen) == 0
            && pamh->environ[i][namelen] == '=') {
            if (delete) {
                for (j = i + 1; pamh->environ[j] != NULL; i++, j++)
                    pamh->environ[i] = pamh->environ[j];
                pamh->environ[i] = NULL;
            } else {
                pamh->environ[i] = setting;
            }
            found = true;
            break;
        }
    if (!found) {
        if (delete)
            return PAM_BAD_ITEM;
        environ = realloc(pamh->environ, (i + 2) * sizeof(char *));
        if (environ == NULL)
            return PAM_BUF_ERR;
        pamh->environ = environ;
        pamh->environ[i] = setting;
        pamh->environ[i + 1] = NULL;
    }
    return PAM_SUCCESS;
}
