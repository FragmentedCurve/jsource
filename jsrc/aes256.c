#include <stdint.h>
#include <string.h> // CBC mode, for memset

#define AES256 1

// public interface
#define AES_ctx AES_ctx_256
#define AES_init_ctx AES_init_ctx_256
#define AES_init_ctx_iv AES_init_ctx_iv_256
#define AES_ctx_set_iv AES_ctx_set_iv_256
#define AES_ECB_encrypt AES_ECB_encrypt_256
#define AES_ECB_decrypt AES_ECB_decrypt_256
#define AES_CBC_encrypt_buffer AES_CBC_encrypt_buffer_256
#define AES_CBC_decrypt_buffer AES_CBC_decrypt_buffer_256
#define AES_CTR_xcrypt_buffer AES_CTR_xcrypt_buffer_256

#include "aes-c.h"
