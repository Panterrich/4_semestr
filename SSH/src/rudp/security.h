#ifndef SECURITY_H
#define SECURITY_H

#include "rudp.h"
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/rsa.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/bn.h>
#include <openssl/rc4.h>

enum TYPE_SIDE 
{
    PRIVATE_SIDE = 0,
    PUBLIC_SIDE  = 1,
};


enum RSA_MODE 
{
    RSA_FILE = 0,
    RSA_BUFF = 1,
};


enum SECURITY_ERROR
{
    SECURITY_UNKNOWN_SIDE = -1,
};

#define PRIME_LENGTH 1024
#define RSA_SIZE     256

struct DH_header_private
{
    unsigned char p_buff      [RSA_SIZE];
    unsigned char g_buff      [RSA_SIZE];
    unsigned char pub_key_buff[RSA_SIZE];

    int p_length;
    int g_length;
    int pub_key_length;
};

struct DH_header_public
{
    unsigned char pub_key_buff[RSA_SIZE];
    int pub_key_length;
};


RSA* security_create_RSA(unsigned char* key, int type_side);

RSA* security_create_RSA_from_file(char* filename, int type_side);

int security_public_encrypt_RSA(unsigned char* data, int data_len, unsigned char *encrypted, unsigned char* key, int mode);

int security_public_decrypt_RSA(unsigned char* enc_data, int data_len, unsigned char *decrypted, unsigned char* key, int mode);

int security_private_encrypt_RSA(unsigned char* data, int data_len, unsigned char *encrypted, unsigned char* key, int mode);

int security_private_decrypt_RSA(unsigned char* enc_data, int data_len, unsigned char *decrypted, unsigned char* key, int mode);

int security_get_secret(char* filename, unsigned char* secret, int type_side, int socket, int type_connection, struct sockaddr_in* connected_address, struct rudp_header* control);

#endif // SECURITY_H