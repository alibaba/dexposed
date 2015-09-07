#ifndef _UAPI_LINUX_COMPILER_H
#define _UAPI_LINUX_COMPILER_H

/*
 * This file is not currently in the Linux kernel tree.
 * Upstream uapi headers refer to <linux/compiler.h> but there is
 * no such uapi file. We've sent this upstream, and are optimistically
 * adding it to bionic in the meantime. This should be replaced by
 * a scrubbed header from external/kernel-headers when possible.
 *
 * An alternative to this file is to check in a symbolic link to the
 * non-uapi <linux/compiler.h>. That's fine for building bionic too.
 */

#define __user
#define __force

#endif /* _UAPI_LINUX_COMPILER_H */
