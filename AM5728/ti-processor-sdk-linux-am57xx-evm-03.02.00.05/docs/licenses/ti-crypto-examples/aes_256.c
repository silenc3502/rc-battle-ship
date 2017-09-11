/*
 * AES encryption/decryption demo
 *
 * AES encryption/decryption demo program using OpenSSL EVP apis
 *
 * Copyright (C) 2012 Texas Instruments Incorporated - http://www.ti.com/ 
 * 
 * 
 *  Redistribution and use in source and binary forms, with or without 
 *  modification, are permitted provided that the following conditions 
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the 
 *    documentation and/or other materials provided with the   
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
*/

/*
*
* object file name: rnddata
* encryption file name: rnddata_enc
* decryption file name: rnddata_dec
*
*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <openssl/engine.h>

/**
 * Create an 256 bit key and IV using the supplied key_data. salt can be added for taste.
 * Fills in the encryption and decryption ctx objects and returns 0 on success
 **/
int aes_init(unsigned char *key_data, int key_data_len, unsigned char *salt, EVP_CIPHER_CTX *e_ctx, 
             EVP_CIPHER_CTX *d_ctx)
{
  int i, nrounds = 5;
  unsigned char key[32], iv[32];
  

/****************Enabling use of a hardware engine******************/
    ENGINE *e;

	ENGINE_load_builtin_engines();
	if (!(e = ENGINE_by_id("cryptodev")))
		fprintf(stderr, "Error finding specified ENGINE\n");
	else if (!ENGINE_set_default(e, ENGINE_METHOD_ALL))
		fprintf(stderr, "Error using ENGINE\n");
	else
		fprintf(stderr, "Engine successfully enabled\n");
/*******************************************************************/

  /*
   * Gen key & IV for AES 256 CBC mode. A SHA1 digest is used to hash the supplied key material.
   * nrounds is the number of times the we hash the material. More rounds are more secure but
   * slower.
   */
  i = EVP_BytesToKey(EVP_aes_256_cbc(), EVP_sha1(), salt, key_data, key_data_len, nrounds, key, iv);  //derive a key and IV from various parameters
  if (i != 32) {
    printf("Key size is %d bits - should be 256 bits\n", i);
    return -1;
  }

  EVP_CIPHER_CTX_init(e_ctx);
  EVP_EncryptInit_ex(e_ctx, EVP_aes_256_cbc(), e, key, iv);  //sets up cipher context e_ctx for encryption with aes_256_cbc cipher type
  EVP_CIPHER_CTX_init(d_ctx);
  EVP_DecryptInit_ex(d_ctx, EVP_aes_256_cbc(), e, key, iv);  //sets up cipher context d_ctx for decryption with aes_256_cbc cipher type

 /* Release the structural reference from ENGINE_by_id() */
 ENGINE_free(e);

  return 0;
}

/*
*   AES encryption function
*/
unsigned char *aes_encrypt(EVP_CIPHER_CTX *e)
{
  
  int i, c_len, f_len = 0;
  char *filename="/home/root/rnddata"; 
  unsigned char buffer[1024],temp[1024];
  unsigned char buffer_out[1024+ EVP_MAX_IV_LENGTH];
  FILE *fp, *fpout, *fprnd, *fptmp;
  int bytes;
  
  fp=fopen(filename, "rb");  
  
  //generate a random file if it doesn't exsit 
  if (fp == NULL) {
       printf ("%s can't be opened.\n", filename);
       printf ("Creating 10M random data file (rnddata)\n");
       system("dd if=/dev/urandom of=/home/root/rnddata bs=1048576 count=10");
       fp = fopen(filename, "rb");
       if(fp == NULL){
 		printf ("%s Error for file opening!\n", filename);       
 		return 0;
        }
   }  

  fpout = fopen("/home/root/rnddata_enc", "wb");

  EVP_EncryptInit_ex(e, NULL, NULL, NULL, NULL);
 
  while ((bytes = fread (buffer, 1, 1024, fp)) != 0)
  {
	EVP_EncryptUpdate(e, buffer_out, &c_len, buffer, bytes);
	fwrite(buffer_out,c_len, 1, fpout);
  }
 
  EVP_EncryptFinal_ex(e, buffer_out, &c_len);
  fwrite(buffer_out, c_len, 1, fpout);  

  fclose(fp);
  fclose(fpout);
  return 0;

}


/*
*   AES decryption function
*/
unsigned char *aes_decrypt(EVP_CIPHER_CTX *e)
{
  int c_len, f_len = 0;
  char *filename="/home/root/rnddata_enc";
  unsigned char buffer[1024];
  unsigned char buffer_out[1024+ EVP_MAX_IV_LENGTH];
  FILE *fp, *fpout;
  int bytes;
  
  fp=fopen(filename, "rb");
  if (fp == NULL) {
        printf ("%s can't be opened.\n", filename);
        return 0;
	}

  fpout = fopen("/home/root/rnddata_dec", "wb");

  EVP_DecryptInit_ex(e, NULL, NULL, NULL, NULL);

  while ((bytes = fread (buffer, 1, 1024, fp)) != 0)
  {
	EVP_DecryptUpdate(e, buffer_out, &c_len, buffer, bytes);
	fwrite(buffer_out, c_len, 1, fpout);
  }

  EVP_DecryptFinal_ex(e, buffer_out, &c_len);
  fwrite(buffer_out, c_len, 1, fpout);

  fclose(fp);
  fclose(fpout);
  return 0;
}

int main(int argc, char **argv)
{
  
  EVP_CIPHER_CTX en, de;

  /* 8 bytes to salt the key_data during key generation. This is an example of
     compiled in salt. We just read the bit pattern created by these two 4 byte 
     integers on the stack as 64 bits of contigous salt material - 
     of course this only works if sizeof(int) >= 4 */
  unsigned int salt[] = {12345, 54321};
  unsigned char *key_data;
  int key_data_len, i;
  FILE *fr;
  long int flen;

  printf("Running OpenSSL Encryption Decryption\n");
  
  if(!argv[1]) {
        printf("Usage: AES_256 password\nNote: you can use your password when encryption.\nBut it should be used the same password when decryption\n");
        exit(1);
  }
  /* the key_data is read from the argument list */
  key_data = (unsigned char *)argv[1]; 
  printf("Password is:%s\n", key_data);
  key_data_len = strlen(argv[1]);

  /* gen key and iv. init the cipher ctx object */
  if (aes_init(key_data, key_data_len, (unsigned char *)&salt, &en, &de)) {
    printf("Couldn't initialize AES cipher\n");
    return -1;
  }

  /* encrypt and decrypt each input */
    
  aes_encrypt(&en);
  aes_decrypt(&de);
  
  system("ls -l /home/root/rnddata /home/root/rnddata_enc /home/root/rnddata_dec");
  
  i = system("diff /home/root/rnddata /home/root/rnddata_dec");
  if(i == 0)
         printf(" diff of rnddata and rnddata_dec indicates that they are the same\n");
  else
         printf(" diff of rnddata and rnddata_dec indicates that they are the different!!  This should not ever happen!!\n");
  EVP_CIPHER_CTX_cleanup(&en);
  EVP_CIPHER_CTX_cleanup(&de);

  return 0;
}
  
