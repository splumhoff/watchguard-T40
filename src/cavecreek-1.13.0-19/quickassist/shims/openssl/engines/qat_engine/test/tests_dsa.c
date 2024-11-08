
/* Copyright (C) 1995-1998 Eric Young (eay@cryptsoft.com)
 * All rights reserved.
 *
 * This package is an SSL implementation written
 * by Eric Young (eay@cryptsoft.com).
 * The implementation was written so as to conform with Netscapes SSL.
 * 
 * This library is free for commercial and non-commercial use as long as
 * the following conditions are aheared to.  The following conditions
 * apply to all code found in this distribution, be it the RC4, RSA,
 * lhash, DES, etc., code; not just the SSL code.  The SSL documentation
 * included with this distribution is covered by the same copyright terms
 * except that the holder is Tim Hudson (tjh@cryptsoft.com).
 * 
 * Copyright remains Eric Young's, and as such any Copyright notices in
 * the code are not to be removed.
 * If this package is used in a product, Eric Young should be given attribution
 * as the author of the parts of the library used.
 * This can be in the form of a textual message at program startup or
 * in documentation (online or textual) provided with the package.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    "This product includes cryptographic software written by
 *     Eric Young (eay@cryptsoft.com)"
 *    The word 'cryptographic' can be left out if the rouines from the library
 *    being used are not cryptographic related :-).
 * 4. If you include any Windows specific code (or a derivative thereof) from 
 *    the apps directory (application code) you must include an acknowledgement:
 *    "This product includes software written by Tim Hudson (tjh@cryptsoft.com)"
 * 
 * THIS SOFTWARE IS PROVIDED BY ERIC YOUNG ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * 
 * The licence and distribution terms for any publically available version or
 * derivative of this code cannot be changed.  i.e. this code cannot simply be
 * copied and put under another distribution licence
 * [including the GNU Public Licence.]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <openssl/crypto.h>
#include <openssl/rand.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/bn.h>
#include <openssl/dsa.h>

#include "tests.h"


static unsigned char BN_p[] =
{ 
        0xD7, 0x57, 0x26, 0x2C, 0x45, 0x84, 0xC4, 0x4C,
        0x21, 0x1F, 0x18, 0xBD, 0x96, 0xE5, 0xF0, 0x61,
        0xC4, 0xF0, 0xA4, 0x23, 0xF7, 0xFE, 0x6B, 0x6B,
        0x85, 0xB3, 0x4C, 0xEF, 0x72, 0xCE, 0x14, 0xA0,
        0xD3, 0xA5, 0x22, 0x2F, 0xE0, 0x8C, 0xEC, 0xE6,
        0x5B, 0xE6, 0xC2, 0x65, 0x85, 0x48, 0x89, 0xDC,
        0x1E, 0xDB, 0xD1, 0x3E, 0xC8, 0xB2, 0x74, 0xDA,
        0x9F, 0x75, 0xBA, 0x26, 0xCC, 0xB9, 0x87, 0x72,
        0x36, 0x02, 0x78, 0x7E, 0x92, 0x2B, 0xA8, 0x44,
        0x21, 0xF2, 0x2C, 0x3C, 0x89, 0xCB, 0x9B, 0x06,
        0xFD, 0x60, 0xFE, 0x01, 0x94, 0x1D, 0xDD, 0x77,
        0xFE, 0x6B, 0x12, 0x89, 0x3D, 0xA7, 0x6E, 0xEB,
        0xC1, 0xD1, 0x28, 0xD9, 0x7F, 0x06, 0x78, 0xD7,
        0x72, 0x2B, 0x53, 0x41, 0xC8, 0x50, 0x6F, 0x35,
        0x82, 0x14, 0xB1, 0x6A, 0x2F, 0xAC, 0x4B, 0x36,
        0x89, 0x50, 0x38, 0x78, 0x11, 0xC7, 0xDA, 0x33,
};

static unsigned char BN_q[] =
{ 
        0xC7, 0x73, 0x21, 0x8C, 0x73, 0x7E, 0xC8, 0xEE,
        0x99, 0x3B, 0x4F, 0x2D, 0xED, 0x30, 0xF4, 0x8E,
        0xDA, 0xCE, 0x91, 0x5F,

};

static unsigned char BN_g[] =
{ 
        0x82, 0x26, 0x90, 0x09, 0xE1, 0x4E, 0xC4, 0x74,
        0xBA, 0xF2, 0x93, 0x2E, 0x69, 0xD3, 0xB1, 0xF1,
        0x85, 0x17, 0xAD, 0x95, 0x94, 0x18, 0x4C, 0xCD,
        0xFC, 0xEA, 0xE9, 0x6E, 0xC4, 0xD5, 0xEF, 0x93,
        0x13, 0x3E, 0x84, 0xB4, 0x70, 0x93, 0xC5, 0x2B,
        0x20, 0xCD, 0x35, 0xD0, 0x24, 0x92, 0xB3, 0x95,
        0x9E, 0xC6, 0x49, 0x96, 0x25, 0xBC, 0x4F, 0xA5,
        0x08, 0x2E, 0x22, 0xC5, 0xB3, 0x74, 0xE1, 0x6D,
        0xD0, 0x01, 0x32, 0xCE, 0x71, 0xB0, 0x20, 0x21,
        0x70, 0x91, 0xAC, 0x71, 0x7B, 0x61, 0x23, 0x91,
        0xC7, 0x6C, 0x1F, 0xB2, 0xE8, 0x83, 0x17, 0xC1,
        0xBD, 0x81, 0x71, 0xD4, 0x1E, 0xCB, 0x83, 0xE2,
        0x10, 0xC0, 0x3C, 0xC9, 0xB3, 0x2E, 0x81, 0x05,
        0x61, 0xC2, 0x16, 0x21, 0xC7, 0x3D, 0x6D, 0xAA,
        0xC0, 0x28, 0xF4, 0xB1, 0x58, 0x5D, 0xA7, 0xF4,
        0x25, 0x19, 0x71, 0x8C, 0xC9, 0xB0, 0x9E, 0xEF,
};


static unsigned char Pub_key[] = 
{
        0x52, 0x3C, 0x3E, 0x53, 0x41, 0xC3, 0xC8, 0xDF,
        0x22, 0x4E, 0x07, 0x0C, 0x99, 0x76, 0xFC, 0x7D,
        0xF1, 0x95, 0xD3, 0xC5, 0x1D, 0x67, 0x1A, 0xF2,
        0xC9, 0x68, 0xA2, 0xA1, 0x41, 0x35, 0x1F, 0xFC,
        0x64, 0x47, 0x65, 0xAD, 0xEB, 0xC4, 0x71, 0xD5,
        0x1B, 0xC2, 0xEF, 0x76, 0x21, 0xE9, 0xED, 0x6A,
        0xA6, 0xD9, 0xDB, 0x5B, 0xBB, 0x81, 0x43, 0x8F,
        0xC0, 0x1E, 0xE6, 0x49, 0x2A, 0xB7, 0xEA, 0x8F,
        0xCB, 0x6B, 0x93, 0x1E, 0x94, 0x8C, 0x24, 0x48,
        0xE3, 0x80, 0xD3, 0x9B, 0x4F, 0xDC, 0xBF, 0x45,
        0xE3, 0xC0, 0xFC, 0xDA, 0x1C, 0x06, 0x74, 0xF9,
        0x02, 0x2A, 0x5A, 0xAA, 0x18, 0x58, 0x1D, 0x12,
        0x4D, 0x07, 0x0F, 0x00, 0xB4, 0x5E, 0xA3, 0x62,
        0xB1, 0xFA, 0xEE, 0x05, 0x04, 0x89, 0x27, 0x5A,
        0x68, 0xBF, 0x8B, 0x19, 0x90, 0x84, 0x21, 0xB8,
        0xFB, 0x3B, 0x37, 0x04, 0x4D, 0x6B, 0xB6, 0xD2,

};

static unsigned char Priv_key[] = 
{ 
        0x6E, 0xB5, 0x08, 0x61, 0xE5, 0x25, 0xD7, 0xEF,
        0x6C, 0x53, 0xBD, 0x77, 0x72, 0x0F, 0x98, 0xE1,
        0x60, 0x3D, 0x67, 0x67,

};

/******************************************************************************
* function:
*           run_dsa(int count,
*                   ENGINE *e
*                   int print_output
*                   int verify)
*
* @param count [IN] - number of iterations
* @param e [IN] - OpenSLL engine pointer
* @param print_output [IN] - print hex output flag
* @param verify [IN] - verify flag
*
* description:
*   The function is design to test DSA sign and verify using qat engine. 
*
*******************************************************************************/
void run_dsa(int count, ENGINE * e, int print_output, int verify, int len)
{
    DSA *dsa = NULL;
    int i = 0;
    int ret = 0;
    unsigned char sig[256] = {0,};
    unsigned int siglen = 0;
    
    unsigned char *DgstData = OPENSSL_malloc(len);

    if (DgstData == NULL)
    {
        printf("DgstData Initial malloc failed ! \n");
        exit(EXIT_FAILURE);
    }

    /* setup input hash message */
    for (i = 0; i < len; i++)
        DgstData[i] = i % 16;
   
    if ((dsa = DSA_new()) == NULL)
        goto end;

    /* iteration count to find h, in this case 105 is expect */
    dsa->q = BN_bin2bn(BN_q, sizeof(BN_q), dsa->q);
    dsa->p = BN_bin2bn(BN_p, sizeof(BN_p), dsa->p);
    dsa->g = BN_bin2bn(BN_g, sizeof(BN_g), dsa->g);
    dsa->pub_key = BN_bin2bn(Pub_key, sizeof(Pub_key), dsa->pub_key);
    dsa->priv_key = BN_bin2bn(Priv_key, sizeof(Priv_key), dsa->priv_key);
    
    for (i = 0; i < count; i++)
    {

        /* DSA_sign() computes a digital signature on the len byte message
           digest dgst using the private key dsa and places its ASN.1 DER
           encoding at sig. The length of the signature is placed in *siglen.
           sig must point to DSA_size(dsa) bytes of memory. */
        DEBUG("%s starting DSA sign \n", __func__);
        ret = DSA_sign(0, DgstData, len, sig, &siglen, dsa);
        if (print_output)

        {
            tests_hexdump(" DSA Signature:", sig, siglen);
        }
        if (verify)

        {

            /* DSA_verify() verifies that the signature sig of size siglen
               matches a given message digest dgst of size len. dsa is the
               signer's public key. Returns * 1: correct signature * 0:
               incorrect signature * -1: error */
            DEBUG("%s starting DSA verify! \n", __func__);
            ret = DSA_verify(0, DgstData, len, sig, siglen, dsa);
            if (!ret)
                printf("# DSA signature verify failed! \n");
        }

        /* the built-in DSA implementation uses constant time modular
           exponentiation for secret exponents by default. The
           DSA_FLAG_NO_EXP_CONSTTIME flag use the faster variable sliding
           window method to be used for all exponents. */
    }
    if (ret)
        printf("# PASS verify for DSA.\n");

    else
        printf("# FAIL verify for DSA.\n");
end:
    DEBUG("%s start to clean up! \n", __func__);
    OPENSSL_free(DgstData);
    
    if (dsa != NULL)
        DSA_free(dsa);
}

/******************************************************************************
* function:
*     tests_run_dsa  (int count,
*                     int size,
*		   engine *e	
*                     int print_output
*                     int verify)
*
* @param count [IN] - number of iterations
* @param size [IN] - the length of input message
* @param e [IN] - OpenSLL engine pointer
* @param print_output [IN] - print hex output flag
* @param verify [IN] - verify flag
*
* description:
*	specify a test case
*
******************************************************************************/
void
tests_run_dsa(int count, int size, ENGINE * e, int print_output, int verify)
{
    run_dsa(count, e, print_output, verify, size);
} 

void
tests_run_dsa_sign_only(int count, int size, ENGINE * e, int print_output,
                        int verify)
{
    run_dsa(count, e, print_output, 0, size);
} 

void
tests_run_dsa_sign_verify(int count, int size, ENGINE * e, int print_output,
                          int verify)
{
    run_dsa(count, e, print_output, 1, size);
}
