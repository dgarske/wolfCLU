/* clu_certgen_rsa.c
 *
 * Copyright (C) 2006-2020 wolfSSL Inc.
 *
 * This file is part of wolfSSL. (formerly known as CyaSSL)
 *
 * wolfSSL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * wolfSSL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <wolfclu/clu_header_main.h>
#include <wolfclu/clu_log.h>
#include <wolfclu/certgen/clu_certgen.h>

void free_things_rsa(byte** a, byte** b, byte** c, RsaKey* d, RsaKey* e,
                                                                     WC_RNG* f);

int make_self_signed_rsa_certificate(char* keyPath, char* certOut, int oid)
{
    int ret = 0;
    word32 index = 0;

    Cert newCert;
    RsaKey key;
    WC_RNG rng;

    int keyFileSz;
    XFILE keyFile;
    XFILE file;
    XFILE pemFile;
    byte* keyBuf;
    int certBufSz;
    byte* certBuf;
    int pemBufSz;
    byte* pemBuf;

    keyFile = XFOPEN(keyPath, "rb");
    if (keyFile == NULL) {
        WOLFCLU_LOG(WOLFCLU_L0, "unable to open key file %s", keyPath);
        return BAD_FUNC_ARG;
    }

    XFSEEK(keyFile, 0, SEEK_END);
    keyFileSz = (int)XFTELL(keyFile);
    keyBuf = (byte*)XMALLOC(keyFileSz, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    if (keyBuf == NULL) {
        XFCLOSE(keyFile);
        return MEMORY_E;
    }
    XFSEEK(keyFile, 0, SEEK_SET);
    XFREAD(keyBuf, 1, keyFileSz, keyFile);
    XFCLOSE(keyFile);

    ret = wc_InitRsaKey(&key, NULL);
    if (ret != 0) {
        WOLFCLU_LOG(WOLFCLU_L0, "Failed to initialize RsaKey\nRET: %d", ret);
        XFREE(keyBuf, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
        return ret;
    }

    ret = wc_InitRng(&rng);
    if (ret != 0) {
        WOLFCLU_LOG(WOLFCLU_L0, "Failed to initialize rng.\nRET: %d", ret);
        XFREE(keyBuf, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
        return ret;
    }

    ret = wc_RsaPrivateKeyDecode(keyBuf, &index, &key, keyFileSz);
    XFREE(keyBuf, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    if (ret != 0 ) {
        WOLFCLU_LOG(WOLFCLU_L0, "Failed to decode private key.\nRET: %d", ret);
        return ret;
    }

    wc_InitCert(&newCert);
    char country[CTC_NAME_SIZE];
    char province[CTC_NAME_SIZE];
    char city[CTC_NAME_SIZE];
    char org[CTC_NAME_SIZE];
    char unit[CTC_NAME_SIZE];
    char commonName[CTC_NAME_SIZE];
    char email[CTC_NAME_SIZE];
    char daysValid[CTC_NAME_SIZE];

    WOLFCLU_LOG(WOLFCLU_L0, "Enter your countries 2 digit code (ex: United States -> US): ");
    XFGETS(country,CTC_NAME_SIZE, stdin);
    country[CTC_NAME_SIZE-1] = '\0';
    WOLFCLU_LOG(WOLFCLU_L0, "Enter the name of the province you are located at: ");
    XFGETS(province,CTC_NAME_SIZE, stdin);
    WOLFCLU_LOG(WOLFCLU_L0, "Enter the name of the city you are located at: ");
    XFGETS(city,CTC_NAME_SIZE, stdin);
    WOLFCLU_LOG(WOLFCLU_L0, "Enter the name of your orginization: ");
    XFGETS(org,CTC_NAME_SIZE, stdin);
    WOLFCLU_LOG(WOLFCLU_L0, "Enter the name of your unit: ");
    XFGETS(unit,CTC_NAME_SIZE, stdin);
    WOLFCLU_LOG(WOLFCLU_L0, "Enter the common name of your domain: ");
    XFGETS(commonName,CTC_NAME_SIZE, stdin);
    WOLFCLU_LOG(WOLFCLU_L0, "Enter your email address: ");
    XFGETS(email,CTC_NAME_SIZE, stdin);
    WOLFCLU_LOG(WOLFCLU_L0, "Enter the number of days this certificate should be valid: ");
    XFGETS(daysValid,CTC_NAME_SIZE, stdin);

    XSTRNCPY(newCert.subject.country, country, CTC_NAME_SIZE);
    XSTRNCPY(newCert.subject.state, province, CTC_NAME_SIZE);
    XSTRNCPY(newCert.subject.locality, city, CTC_NAME_SIZE);
    XSTRNCPY(newCert.subject.org, org, CTC_NAME_SIZE);
    XSTRNCPY(newCert.subject.unit, unit, CTC_NAME_SIZE);
    XSTRNCPY(newCert.subject.commonName, commonName, CTC_NAME_SIZE);
    XSTRNCPY(newCert.subject.email, email, CTC_NAME_SIZE);
    newCert.daysValid = XATOI(daysValid);
    newCert.isCA    = 0;

    switch(oid) {
        case SHA_HASH:
            newCert.sigType = CTC_SHAwRSA;
            break;
        case SHA_HASH224:
            newCert.sigType = CTC_SHA224wRSA;
            break;
        case SHA_HASH256:
            newCert.sigType = CTC_SHA256wRSA;
            break;
        case SHA_HASH384:
            newCert.sigType = CTC_SHA384wRSA;
            break;
        case SHA_HASH512:
            newCert.sigType = CTC_SHA512wRSA;
            break;
    }

    certBuf = (byte*) XMALLOC(FOURK_SZ, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    if (certBuf == NULL) {
        WOLFCLU_LOG(WOLFCLU_L0, "Failed to initialize buffer to stort certificate.");
        return -1;
    }
    XMEMSET(certBuf, 0, FOURK_SZ);

    ret = wc_MakeCert(&newCert, certBuf, FOURK_SZ, &key, NULL, &rng);
    if (ret < 0) {
        WOLFCLU_LOG(WOLFCLU_L0, "Failed to make certificate.");
        return ret;
    }
    WOLFCLU_LOG(WOLFCLU_L0, "MakeCert returned %d", ret);

    ret = wc_SignCert(newCert.bodySz, newCert.sigType, certBuf, FOURK_SZ, &key, 
                                                                   NULL, &rng);
    if (ret < 0) {
        WOLFCLU_LOG(WOLFCLU_L0, "Failed to sign certificate.");
        return ret;
    }
    WOLFCLU_LOG(WOLFCLU_L0, "SignCert returned %d", ret);

    certBufSz = ret;

    WOLFCLU_LOG(WOLFCLU_L0, "Successfully created new certificate");
    WOLFCLU_LOG(WOLFCLU_L0, "Writing newly generated certificate to file \"%s\"",
                                                                 certOut);
    file = XFOPEN(certOut, "wb");
    if (!file) {
        WOLFCLU_LOG(WOLFCLU_L0, "failed to open file: %s", certOut);
        return -1;
    }

    ret = (int)XFWRITE(certBuf, 1, certBufSz, file);
    XFCLOSE(file);
    WOLFCLU_LOG(WOLFCLU_L0, "Successfully output %d bytes", ret);

/*---------------------------------------------------------------------------*/
/* convert the der to a pem and write it to a file */
/*---------------------------------------------------------------------------*/

    WOLFCLU_LOG(WOLFCLU_L0, "Convert the der cert to pem formatted cert");

    pemBuf = (byte*)XMALLOC(FOURK_SZ, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    if (pemBuf == NULL) {
        WOLFCLU_LOG(WOLFCLU_L0, "Failed to initialize pem buffer.");
        return -1;
    }
    XMEMSET(pemBuf, 0, FOURK_SZ);

    pemBufSz = wc_DerToPem(certBuf, certBufSz, pemBuf, FOURK_SZ, CERT_TYPE);
    if (pemBufSz < 0) {
        WOLFCLU_LOG(WOLFCLU_L0, "Failed to convert from der to pem.");
        return -1;
    }

    WOLFCLU_LOG(WOLFCLU_L0, "Resulting pem buffer is %d bytes", pemBufSz);

    pemFile = XFOPEN(certOut, "wb");
    if (!pemFile) {
        WOLFCLU_LOG(WOLFCLU_L0, "failed to open file: %s", certOut);
        return -1;
    }
    XFWRITE(pemBuf, 1, pemBufSz, pemFile);
    XFCLOSE(pemFile);
    WOLFCLU_LOG(WOLFCLU_L0, "Successfully converted the der to pem. Result is in:  %s\n",
                                                                 certOut);

    free_things_rsa(&pemBuf, &certBuf, NULL, &key, NULL, &rng);
    return 1;
}

void free_things_rsa(byte** a, byte** b, byte** c, RsaKey* d, RsaKey* e,
                                                                      WC_RNG* f)
{
    if (a != NULL) {
        if (*a != NULL) {
            XFREE(*a, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
            *a = NULL;
        }
    }
    if (b != NULL) {
        if (*b != NULL) {
            XFREE(*b, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
            *b = NULL;
        }
    }
    if (c != NULL) {
        if (*c != NULL) {
            XFREE(*c, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
            *c = NULL;
        }
    }

    wc_FreeRsaKey(d);
    wc_FreeRsaKey(e);
    wc_FreeRng(f);

}

