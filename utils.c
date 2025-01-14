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
 *
 * Portions Copyright 2008 Denis Cheng
 */

#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <fcntl.h>

#include <errno.h>
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#include "filebench.h"
#include "utils.h"
#include "parsertypes.h"

//set one of these to 1
#define isDedup 0
#define isCompress 1



long Duplicity = 10;
fbint_t DuplicationBlockSize = 4096;

long compress_ratio = .1;
fbint_t CompressionBlockSize=  4096;

void fillBufferWithCompressibility(char * buffer, fbint_t size , double compress_ratio)
{

	if(compress_ratio < 0 || compress_ratio > 1)
		return;

	
	
	int urandom_fd = open("/dev/urandom", O_RDONLY);
        size_t bytesRead = read(urandom_fd, buffer, size);


	fbint_t elements_to_fix = (1.0-compress_ratio) * (double)size;

	srand((unsigned int)time(NULL));
	char a = rand()%256;
	memset(buffer, a, elements_to_fix);

	
}

/*For dedup workloads, use duplicity in buffer generation*/
// Function to fill the buffer with duplicity or random data
void fillBufferWithDuplicity(char *buffer, fbint_t size, long duplicity) {
    if (duplicity < 0 || duplicity > 100) {
        printf("Duplicity percentage must be between 0 and 100.\n");
        return;
    }

    int urandom_fd = open("/dev/urandom", O_RDONLY);
    srand((unsigned int)time(NULL));

    long randomValue = rand() % 100; // Generate a random number between 0 and 99

    if (randomValue >= duplicity) {
        // If the random value is greater than or equal to duplicity, fill the buffer with random data
    	size_t bytesRead = read(urandom_fd, buffer, size);

        if (bytesRead == -1) {
            perror("Failed to read from /dev/urandom");
            close(urandom_fd);
            exit(EXIT_FAILURE);
        }

        if (bytesRead < size) {
            fprintf(stderr, "Insufficient data read from /dev/urandom\n");
            close(urandom_fd);
            exit(EXIT_FAILURE);
        }

    }



    close(urandom_fd);
}

// Function to split the buffer and fill it with duplicity or random data
void splitter(char *buffer, fbint_t size, long duplicity, fbint_t duplicationBlockSize) {
   fbint_t bs;
#ifdef isDedup
	bs = duplicationBlockSize;
#endif
#ifdef isCompress
	bs = CompressionBlockSize;
#endif

if (size < bs) {
#ifdef isDedup
        fillBufferWithDuplicity(buffer, size, duplicity);
#endif
#ifdef isCompress
        fillBufferWithCompressibility(buffer, size, compress_ratio);
#endif
    } else {
        // Fill the buffer in blocks of DuplicationBlockSize bytes
        fbint_t remainingSize = size;
        caddr_t currentBuffer = buffer;

        while (remainingSize >= bs) {
#ifdef isDedup
            fillBufferWithDuplicity(currentBuffer, bs, duplicity);
#endif
#ifdef isCompress
        fillBufferWithCompressibility(currentBuffer, bs, compress_ratio);
#endif
            currentBuffer += bs;
            remainingSize -= bs;
        }

        // Fill the remaining portion
        if (remainingSize > 0) {
#ifdef isDedup
            fillBufferWithDuplicity(currentBuffer, remainingSize, duplicity);
#endif
#ifdef isCompress
        fillBufferWithCompressibility(currentBuffer, remainingSize, compress_ratio);
#endif
        }
    } 
}



/*
 * For now, just three routines: one to allocate a string in shared
 * memory, one to emulate a strlcpy() function and one to emulate a
 * strlcat() function, both the second and third only used in non
 * Solaris environments,
 *
 */


/*
 * Allocates space for a new string of the same length as
 * the supplied string "str". Copies the old string into
 * the new string and returns a pointer to the new string.
 * Returns NULL if memory allocation for the new string fails.
 */
char *
fb_stralloc(char *str)
{
	char *newstr;

	if ((newstr = malloc(strlen(str) + 1)) == NULL)
		return (NULL);
	(void) strcpy(newstr, str);
	return (newstr);
}

#ifndef HAVE_STRLCPY
/*
 * Implements the strlcpy function when compilied for non Solaris
 * operating systems. On solaris the strlcpy() function is used
 * directly.
 */
size_t
fb_strlcpy(char *dst, const char *src, size_t dstsize)
{
	uint_t i;

	for (i = 0; i < (dstsize - 1); i++) {

		/* quit if at end of source string */
		if (src[i] == '\0')
			break;

		dst[i] = src[i];
	}

	/* set end of dst string to \0 */
	dst[i] = '\0';
	i++;

	return (i);
}
#endif /* HAVE_STRLCPY */

#ifndef HAVE_STRLCAT
/*
 * Implements the strlcat function when compilied for non Solaris
 * operating systems. On solaris the strlcat() function is used
 * directly.
 */
size_t
fb_strlcat(char *dst, const char *src, size_t dstsize)
{
	uint_t i, j;

	/* find the end of the current destination string */
	for (i = 0; i < (dstsize - 1); i++) {
		if (dst[i] == '\0')
			break;
	}

	/* append the source string to the destination string */
	for (j = 0; i < (dstsize - 1); i++) {
		if (src[j] == '\0')
			break;

		dst[i] = src[j];
		j++;
	}

	/* set end of dst string to \0 */
	dst[i] = '\0';
	i++;

	return (i);
}
#endif /* HAVE_STRLCAT */


#ifdef HAVE_PROC_SYS_KERNEL_SHMMAX
/*
 * Increase the maximum shared memory segment size till some large value.  We do
 * not restore it to the old value when the Filebench run is over. If we could
 * not change the value - we continue execution.
 */
void
fb_set_shmmax(void)
{
	FILE *f;
	int ret;

	f = fopen("/proc/sys/kernel/shmmax", "r+");
	if (!f) {
		filebench_log(LOG_FATAL, "WARNING: Could not open "
				"/proc/sys/kernel/shmmax file!\n"
				"It means that you probably ran Filebench not "
				"as a root. Filebench will not increase shared\n"
				"region limits in this case, which can lead "
				"to the failures on certain workloads.");
		return;
	}

	/* writing new value */
#define SOME_LARGE_SHMAX "268435456" /* 256 MB */
	ret = fwrite(SOME_LARGE_SHMAX, sizeof(SOME_LARGE_SHMAX), 1, f);
	if (ret != 1)
		filebench_log(LOG_ERROR, "Coud not write to "
				"/proc/sys/kernel/shmmax file!");
#undef SOME_LARGE_SHMAX

	fclose(f);

	return;
}
#else /* HAVE_PROC_SYS_KERNEL_SHMMAX */
void
fb_set_shmmax(void)
{
	return;
}
#endif /* HAVE_PROC_SYS_KERNEL_SHMMAX */

#ifdef HAVE_SETRLIMIT
/*
 * Increase the limit of opened files.
 *
 * We first set the limit to the hardlimit reported by the kernel; this call
 * will always succeed.  Then we try to set the limit to some large number of
 * files (unfortunately we can't set this ulimit to infinity), this will only
 * succeed if the process is ran by root.  Therefore, we always set the maximum
 * possible value for the limit for this given process (well, only if hardlimit
 * is greater then the large number of files defined by us, it is not true).
 *
 * Increasing this limit is especially important when we use thread model,
 * because opened files are accounted per-process, not per-thread.
 */
void
fb_set_rlimit(void)
{
	struct rlimit rlp;

	(void)getrlimit(RLIMIT_NOFILE, &rlp);
	rlp.rlim_cur = rlp.rlim_max;
	(void)setrlimit(RLIMIT_NOFILE, &rlp);
#define SOME_LARGE_NUMBER_OF_FILES 50000
	rlp.rlim_cur = rlp.rlim_max = SOME_LARGE_NUMBER_OF_FILES;
#undef SOME_LARGE_NUMBER_OF_FILES
	(void)setrlimit(RLIMIT_NOFILE, &rlp);
	return;
}
#else /* HAVE_SETRLIMIT */
void
fb_set_rlimit(void)
{
	return;
}
#endif /* HAVE_SETRLIMIT */
