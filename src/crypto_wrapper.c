#include <openssl/aes.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "crypto_scrypt.h"
#include "crypto_aesctr.h"
#include "sha256.h"

/* TODO: zero out sensitive data */

typedef struct{
  AES_KEY* aes_key;
  struct crypto_aesctr* aes_state;
  HMAC_SHA256_CTX hctx;
  size_t buflen;
  unsigned char* buf;
} c_state;

int c_keyderiv(const unsigned char * passwd, size_t passwdlen, 
	       const unsigned char * salt, size_t saltlen,
	       unsigned char * buf, size_t buflen){
  return (crypto_scrypt(passwd, passwdlen,
			salt, saltlen,
			65536, 8, 5,
			buf, buflen));
}

/* key must be 64 bytes long */
c_state* c_initialize(const unsigned char* key, size_t sec_size){
  FILE* random;
  c_state* state;
  uint64_t iv;

  if((random = fopen("/dev/random","r")) == NULL)
    goto err0;

  if(fread(&iv, sizeof(iv), 1, random) != sizeof(iv))
    goto err1;
  else
    fclose(random);

  if((state = malloc(sizeof(c_state))) == NULL)
    goto err1;
  if((state->aes_key = malloc(sizeof(AES_KEY))) == NULL)
    goto err2;

  if(!AES_set_encrypt_key(key, 256, state->aes_key))
    goto err3;
  
  if((state->aes_state = crypto_aesctr_init(state->aes_key, iv)) == NULL)
    goto err3;

  HMAC_SHA256_Init(&(state->hctx), &key[32], 32);
  
  state->buflen = 0;
  state->buf = NULL;
  
  return(state);
  
 err3:
  free(state->aes_key);
 err2:
  free(state);
 err1:
  fclose(random);
 err0:
  return(NULL);
}

size_t c_encrypt(c_state* state, unsigned char* outbuf, size_t outbuflen, const unsigned char* inbuf, size_t inbuflen){
  if(state->buflen > 0){ 
    /* we have unprocessed data */
    unsigned char* buf = state->buf;
    state->buf = NULL;
    size_t buflen = state->buflen;
    state->buflen = 0;
    size_t enc_count = c_encrypt(state, outbuf, outbuflen, buf, buflen);

    size_t newlen = buflen - enc_count;
    if(newlen != 0){
      /* didn't process all unprocessed data */
      free(state->buf);
      state->buf = NULL;
      state->buflen = 0;

      unsigned char* newbuf;
      if((newbuf = malloc(sizeof(unsigned char)*(newlen + inbuflen))) == NULL)
	return(0);

      memcpy(newbuf, &(buf[enc_count]), newlen);
      if(inbuflen > 0)
	memcpy(&newbuf[newlen], inbuf, inbuflen);

      free(buf);
      buf = NULL;
      state->buf = newbuf;
      state->buflen = (newlen + inbuflen);
      return(enc_count);
    }else{
      /* processed all unprocessed data, now try to process freshly supplied data */
      free(buf);
      buf = NULL;
      return(c_encrypt(state,&outbuf[enc_count],newlen,inbuf,inbuflen));
    }
  }else{
    /* no unprocessed data */
    size_t minlen = (outbuflen < inbuflen) ? outbuflen : inbuflen;
    if(inbuflen > minlen){
      unsigned char* newbuf;
      if((newbuf = malloc(sizeof(unsigned char) * (inbuflen - minlen))) == NULL)
	return(0);
      memcpy(newbuf,&inbuf[minlen],inbuflen-minlen);
      state->buf = newbuf;
      state->buflen = inbuflen-minlen;
    }

    crypto_aesctr_stream(state->aes_state, inbuf, outbuf, minlen);
    HMAC_SHA256_Update(&(state->hctx), outbuf, minlen);
    return(minlen);
  }
}

size_t c_decrypt(c_state* state, unsigned char* outbuf, size_t outbuflen, const unsigned char* inbuf, size_t inbuflen){
  if(state->buflen > 0){ 
    /* we have unprocessed data */
    unsigned char* buf = state->buf;
    state->buf = NULL;
    size_t buflen = state->buflen;
    state->buflen = 0;
    size_t dec_count = c_decrypt(state, outbuf, outbuflen, buf, buflen);

    size_t newlen = buflen - dec_count;
    if(newlen != 0){
      /* didn't process all unprocessed data */
      free(state->buf);
      state->buf = NULL;
      state->buflen = 0;

      unsigned char* newbuf;
      if((newbuf = malloc(sizeof(unsigned char)*(newlen + inbuflen))) == NULL)
	return(0);

      memcpy(newbuf, &(buf[dec_count]), newlen);
      if(inbuflen > 0)
	memcpy(&newbuf[newlen], inbuf, inbuflen);

      free(buf);
      buf = NULL;
      state->buf = newbuf;
      state->buflen = (newlen + inbuflen);
      return(dec_count);
    }else{
      /* processed all unprocessed data, now try to process freshly supplied data */
      free(buf);
      buf = NULL;
      return(c_decrypt(state,&outbuf[dec_count],newlen,inbuf,inbuflen));
    }
  }else{
    /* no unprocessed data */
    size_t minlen = (outbuflen < inbuflen) ? outbuflen : inbuflen;
    if(inbuflen > minlen){
      unsigned char* newbuf;
      if((newbuf = malloc(sizeof(unsigned char) * (inbuflen - minlen))) == NULL)
	return(0);
      memcpy(newbuf,&inbuf[minlen],inbuflen-minlen);
      state->buf = newbuf;
      state->buflen = inbuflen-minlen;
    }

    HMAC_SHA256_Update(&(state->hctx), inbuf, minlen);
    crypto_aesctr_stream(state->aes_state, inbuf, outbuf, minlen);
    return(minlen);
  }
}

signed char c_encrypt_finalize(c_state* state, unsigned char* outbuf, size_t outbuflen){
  if(state->buflen > 0){
    size_t offset = c_encrypt(state, outbuf, outbuflen, NULL, 0);
    if(state->buflen > 0){
      return(-1);
    }else{
      return(c_encrypt_finalize(state, &outbuf[offset], outbuflen-offset));
    }
  }
  if(outbuflen > 32){
    HMAC_SHA256_Final(outbuf,&(state->hctx));
    return(0);
  }else{
    return(-2);
  }
}

signed char c_decrypt_finalize(c_state* state, unsigned char* inbuf, size_t inbuflen){
  if(state->buflen > 0){
    HMAC_SHA256_Update(&(state->hctx), state->buf, state->buflen);
    free(state->buf);
    state->buf = NULL;
    state->buflen = 0;
  }
  unsigned char hbuf[32];
  if(inbuflen > 32){
    HMAC_SHA256_Update(&(state->hctx), inbuf, inbuflen - 32);
    HMAC_SHA256_Final(hbuf, &(state->hctx));
    if(memcmp(hbuf,&inbuf[32],32))
      return (-1);
  }else if(inbuflen == 32){
    HMAC_SHA256_Final(hbuf, &(state->hctx));
    if(memcmp(hbuf,&inbuf[32],32))
      return (-1);
  }else{
    return(-2);
  }
}

void c_free(c_state* state){
  crypto_aesctr_free(state->aes_state);
  free(state->aes_key);
  free(state);
}
