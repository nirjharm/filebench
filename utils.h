/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#ifndef	_FB_UTILS_H
#define	_FB_UTILS_H

#include "filebench.h"

extern void splitter(char *buffer, fbint_t size, long duplicity, fbint_t DuplicationBlockSize);

extern char *fb_stralloc(char *str);

#ifdef HAVE_STRLCAT
#define	fb_strlcat	strlcat
#else
extern size_t fb_strlcat(char *dst, const char *src, size_t dstsize);
#endif /* HAVE_STRLCAT */

#ifdef HAVE_STRLCPY
#define	fb_strlcpy	strlcpy
#else
extern size_t fb_strlcpy(char *dst, const char *src, size_t dstsize);
#endif /* HAVE_STRLCPY */

extern void fb_set_shmmax(void);
void fb_set_rlimit(void);


#endif	/* _FB_UTILS_H */
