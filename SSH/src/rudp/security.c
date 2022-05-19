#include "security.h"
#include "syslog.h"

static int padding = RSA_PKCS1_PADDING; 

RSA* security_create_RSA(unsigned char* key, int type_side)
{
    if (type_side != PUBLIC_SIDE && type_side != PRIVATE_SIDE) return NULL;

    RSA* rsa    = RSA_new();
    BIO* keybio = NULL;

    keybio = BIO_new_mem_buf(key, -1);
    if (!keybio)
    {
        perror("BIO_new_mem_buf()");
        return NULL;
    }

    if (type_side == PUBLIC_SIDE)
    {
        rsa = PEM_read_bio_RSA_PUBKEY(keybio, &rsa, NULL, NULL);
    }
    else
    {
        rsa = PEM_read_bio_RSAPrivateKey(keybio, &rsa, NULL, NULL);
    }

    if (!rsa)
    {
        perror("PEM_read_bio_RSA()");
    }
    
    return rsa;
}

RSA* security_create_RSA_from_file(char* filename, int type_side)
{
    if (type_side != PUBLIC_SIDE && type_side != PRIVATE_SIDE) return NULL;

    FILE* file = fopen(filename, "rb");
    if (file == NULL)
    {
        perror("open()");
        return NULL;
    }

    RSA* rsa = RSA_new();
    
    if (type_side == PUBLIC_SIDE)
    {
        rsa = PEM_read_RSA_PUBKEY(file, &rsa, NULL, NULL);
    }
    else
    {
        rsa = PEM_read_RSAPrivateKey(file, &rsa, NULL, NULL);
    }

    if (!rsa)
    {
        perror("PEM_read_RSA():");
    }
    
    return rsa;
}

int security_public_encrypt_RSA(unsigned char* data, int data_len, unsigned char *encrypted, unsigned char* key, int mode)
{
    if (mode != RSA_BUFF && mode != RSA_FILE) return -1;

    RSA* rsa = NULL;

    if (mode == RSA_BUFF) rsa = security_create_RSA(key, PUBLIC_SIDE);
    if (mode == RSA_FILE) rsa = security_create_RSA_from_file((char*) key, PUBLIC_SIDE);
    
    if (!rsa) return -1;

    int result = RSA_public_encrypt(data_len, data, encrypted, rsa, padding);
    return result;
}

int security_public_decrypt_RSA(unsigned char* enc_data, int data_len, unsigned char *decrypted, unsigned char* key, int mode)
{
    if (mode != RSA_BUFF && mode != RSA_FILE) return -1;

    RSA* rsa = NULL;

    if (mode == RSA_BUFF) rsa = security_create_RSA(key, PUBLIC_SIDE);
    if (mode == RSA_FILE) rsa = security_create_RSA_from_file((char*) key, PUBLIC_SIDE);
    
    if (!rsa) return -1;
    
    int result = RSA_public_decrypt(data_len, enc_data, decrypted, rsa, padding);
    return result;
}

int security_private_encrypt_RSA(unsigned char* data, int data_len, unsigned char *encrypted, unsigned char* key, int mode)
{
    if (mode != RSA_BUFF && mode != RSA_FILE) return -1;

    RSA* rsa = NULL;

    if (mode == RSA_BUFF) rsa = security_create_RSA(key, PRIVATE_SIDE);
    if (mode == RSA_FILE) rsa = security_create_RSA_from_file((char*) key, PRIVATE_SIDE);
    
    if (!rsa) return -1;

    int result = RSA_private_encrypt(data_len, data, encrypted, rsa, padding);
    return result;
}

int security_private_decrypt_RSA(unsigned char* enc_data, int data_len, unsigned char *decrypted, unsigned char* key, int mode)
{
    if (mode != RSA_BUFF && mode != RSA_FILE) return -1;

    RSA* rsa = NULL;

    if (mode == RSA_BUFF) rsa = security_create_RSA(key, PRIVATE_SIDE);
    if (mode == RSA_FILE) rsa = security_create_RSA_from_file((char*) key, PRIVATE_SIDE);
    
    if (!rsa) return -1;
    
    int result = RSA_private_decrypt(data_len, enc_data, decrypted, rsa, padding);
    return result;
}

int security_get_secret(char* filename, unsigned char* secret, int type_side, int socket, int type_connection, struct sockaddr_in* connected_address, struct rudp_header* control)
{
    RSA* rsa = security_create_RSA_from_file(filename, type_side);
    if (!rsa) return -1;

    DH* dh = DH_new();

    if (type_side == PRIVATE_SIDE)
    {
        if (DH_generate_parameters_ex(dh, PRIME_LENGTH, 2, NULL) != 1)
        {
            perror("DH_generate_parameters_ex()");

            DH_free(dh);
            return -1;
        }
        
        int errflags = 0;
        if (DH_check(dh, &errflags) != 1)
        {
            perror("DH_check()");

            DH_free(dh);
            return -1;
        }

        if (DH_generate_key(dh) != 1)
        {
            perror("DH_generate_key()");

            DH_free(dh);
            return -1;
        }

        const BIGNUM* p = NULL;
        const BIGNUM* q = NULL;
        const BIGNUM* g = NULL;

        DH_get0_pqg(dh, &p, &q, &g);
        
        const BIGNUM* pub_key = DH_get0_pub_key(dh);

        struct DH_header_private enc_priv_header = {};
        struct DH_header_private     priv_header = {};

        BN_bn2bin(p,       priv_header.p_buff);
        BN_bn2bin(g,       priv_header.g_buff);
        BN_bn2bin(pub_key, priv_header.pub_key_buff);
        
        int result = RSA_private_encrypt(PRIME_LENGTH, priv_header.p_buff, enc_priv_header.p_buff, rsa, padding);
        if (result == -1)
        {
            perror("RSA_private_encrypt()");

            DH_free(dh);
            return -1;
        }

        result = RSA_private_encrypt(PRIME_LENGTH, priv_header.g_buff, enc_priv_header.g_buff, rsa, padding);
        if (result == -1)
        {
            perror("RSA_private_encrypt()");

            DH_free(dh);
            return -1;
        }

        result = RSA_private_encrypt(PRIME_LENGTH, priv_header.pub_key_buff, enc_priv_header.pub_key_buff, rsa, padding);
        if (result == -1)
        {
            perror("RSA_private_encrypt()");

            DH_free(dh);
            return -1;
        }

        result = rudp_send(socket, &enc_priv_header, sizeof(struct DH_header_private), connected_address, type_connection, control);
        if (result < 0)
        {
            perror("rudp_send()");

            DH_free(dh);
            return -1;
        }

        struct DH_header_public enc_pub_header = {};
        struct DH_header_public     pub_header = {};


        result = rudp_recv(socket, &enc_pub_header, sizeof(struct DH_header_public), connected_address, type_connection, control);
        if (result <= 0)
        {
            perror("rudp_recv()");

            DH_free(dh);
            return -1;
        }

        result = RSA_private_decrypt(RSA_SIZE, enc_pub_header.pub_key_buff, pub_header.pub_key_buff, rsa, padding);
        if (result == -1)
        {
            perror("RSA_private_deccrypt()");

            DH_free(dh);
            return -1;
        }

        BIGNUM* second_pub_key_bn = BN_new();
        second_pub_key_bn = BN_bin2bn(pub_header.pub_key_buff, PRIME_LENGTH, second_pub_key_bn);

        if (DH_compute_key(secret, second_pub_key_bn, dh) != 1)
        {
            perror("DH_compute_key()");

            DH_free(dh);
            BN_free(second_pub_key_bn);
            return -1;
        }

        DH_free(dh);
        BN_free(second_pub_key_bn);
        return 0;
    }
    else if (type_side == PUBLIC_SIDE)
    {      
        struct DH_header_private enc_priv_header = {};
        struct DH_header_private     priv_header = {};

        int result = rudp_recv(socket, &enc_priv_header, sizeof(struct DH_header_private), connected_address, type_connection, control);
        if (result <= 0)
        {
            perror("rudp_recv()");

            DH_free(dh);
            return -1;
        }

        result = RSA_public_decrypt(RSA_SIZE, enc_priv_header.p_buff, priv_header.p_buff, rsa, padding);
        if (result == -1)
        {
            perror("RSA_public_deccrypt()");

            DH_free(dh);
            return -1;
        }

        result = RSA_public_decrypt(RSA_SIZE, enc_priv_header.g_buff, priv_header.g_buff, rsa, padding);
        if (result == -1)
        {
            perror("RSA_public_deccrypt()");

            DH_free(dh);
            return -1;
        }

        result = RSA_public_decrypt(RSA_SIZE, enc_priv_header.pub_key_buff, priv_header.pub_key_buff, rsa, padding);
        if (result == -1)
        {
            perror("RSA_public_deccrypt()");

            DH_free(dh);
            return -1;
        }

        BIGNUM* p   = BN_new();
        BIGNUM* g   = BN_new();
        BIGNUM* key = BN_new();

        p   = BN_bin2bn(priv_header.p_buff,       PRIME_LENGTH, p);
        g   = BN_bin2bn(priv_header.g_buff,       PRIME_LENGTH, g);
        key = BN_bin2bn(priv_header.pub_key_buff, PRIME_LENGTH, key);
        
        if (DH_set0_pqg(dh, p, NULL, g) != 1)
        {
            perror("DH_set0_pqg()");

            DH_free(dh);
            BN_free(key);
            return -1;
        }

        syslog(LOG_NOTICE, "[SECURITY] WHO %s\n", ERR_reason_error_string(ERR_get_error()));
        int errflags = 0;
        if (DH_check(dh, &errflags) != 1)
        {
            syslog(LOG_NOTICE, "[SECURITY] @@@@@@ %s", ERR_reason_error_string(ERR_get_error()));
            perror("DH_check()");

            DH_free(dh);
            BN_free(key);
            return -1;
        }

        syslog(LOG_NOTICE, "[SECURITY] ZZZZZZZ %s", ERR_reason_error_string(ERR_get_error()));

        if (DH_generate_key(dh) != 1)
        {   
            syslog(LOG_ERR, "[SECURITY] WHERE!!! %s", ERR_reason_error_string(ERR_get_error()));
            perror("DH_generate_key()");

            DH_free(dh);
            BN_free(key);
            return -1;
        }

        syslog(LOG_NOTICE, "[SECURITY] MMMMMMMMM %s", ERR_reason_error_string(ERR_get_error()));


        const BIGNUM* pub_key = DH_get0_pub_key(dh);

        struct DH_header_public enc_pub_header = {};
        struct DH_header_public     pub_header = {};

        BN_bn2bin(pub_key, pub_header.pub_key_buff);
        
        result = RSA_public_encrypt(sizeof(struct DH_header_public), (unsigned char*) &pub_header, (unsigned char*) &enc_pub_header, rsa, padding);
        if (result == -1)
        {
            perror("RSA_public_encrypt()");

            DH_free(dh);
            BN_free(key);
            return -1;
        }

        syslog(LOG_NOTICE, "[SECURITY] 4 \n");

        result = rudp_send(socket, &enc_pub_header, sizeof(struct DH_header_public), connected_address, type_connection, control);
        if (result < 0)
        {
            perror("rudp_send()");

            DH_free(dh);
            BN_free(key);
            return -1;
        }

        if (DH_compute_key(secret, key, dh) != 1)
        {
            perror("DH_compute_key()");

            DH_free(dh);
            BN_free(key);
            return -1;
        }

        DH_free(dh);
        BN_free(key);
        return 0;
    }
    else
    {
        DH_free(dh);
        return SECURITY_UNKNOWN_SIDE;
    }
}
