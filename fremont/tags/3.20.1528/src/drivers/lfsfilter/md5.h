/*
 * This is the header file for the MD5 message-digest algorithm.
 * The algorithm is due to Ron Rivest.  This code was
 * written by Colin Plumb in 1993, no copyright is claimed.
 * This code is in the public domain; do with it what you wish.
 *
 * Equivalent code is available from RSA Data Security, Inc.
 * This code has been tested against that, and is equivalent,
 * except that you don't need to include two pages of legalese
 * with every copy.
 *
 * To compute the message digest of a chunk of bytes, declare an
 * MD5_CTX structure, pass it to MD5Init, call MD5Update as
 * needed on buffers full of bytes, and then call MD5Final, which
 * will fill a supplied 16-byte array with the digest.
 *
 * Changed so as no longer to depend on Colin Plumb's `usual.h'
 * header definitions; now uses stuff from dpkg's config.h
 *  - Ian Jackson <ijackson@nyx.cs.du.edu>.
 * Still in the public domain.
 */

#ifndef MD5_H
#define MD5_H

/* for uint32_t, WORDS_BIGENDIAN */
//#include "iscsi-platform.h"


typedef _U32 UWORD32;

#ifdef __cplusplus
extern "C" {
#endif


#define md5byte unsigned char

typedef struct _MD5_CTX {
	UWORD32 buf[4];
	UWORD32 bytes[2];
	UWORD32 in[16];
    unsigned char digest[16];            /* actual digest after MD5Final call */
} MD5_CTX;

void MD5Init(struct _MD5_CTX *context);
void MD5Update(struct _MD5_CTX *context, md5byte const *buf, unsigned len);
void MD5Final(unsigned char digest[16], struct _MD5_CTX *context);
void MD5Transform(UWORD32 buf[4], UWORD32 const in[16]);


#ifdef __cplusplus
}
#endif

#endif /* !MD5_H */